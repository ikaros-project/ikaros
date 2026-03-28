#include "ikaros.h"

#include <cerrno>
#include <csignal>
#include <filesystem>
#include <map>
#include <stdexcept>
#include <string>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace ikaros;

namespace
{
    dictionary make_dictionary(value v)
    {
        return std::get<dictionary>(v.value_);
    }

    list make_list(value v)
    {
        return std::get<list>(v.value_);
    }

    list shape_to_list(const matrix & m)
    {
        list shape;
        for(int d : m.shape())
            shape.push_back(double(d));
        return shape;
    }

    void flatten_json_matrix(value v, std::vector<float> & values, std::vector<int> & shape, int depth = 0)
    {
        if(v.is_list())
        {
            list current = make_list(v);
            if(shape.size() <= static_cast<size_t>(depth))
                shape.push_back(current.size());
            else if(shape[depth] != current.size())
                throw std::runtime_error("Inconsistent nested list shape in python output.");

            for(auto item : current)
                flatten_json_matrix(item, values, shape, depth + 1);
        }
        else
            values.push_back(v.as_float());
    }

    void assign_matrix_from_json(matrix & target, value v)
    {
        std::vector<float> flattened;
        std::vector<int> actual_shape;
        flatten_json_matrix(v, flattened, actual_shape);

        const std::vector<int> & expected_shape = target.shape();
        if(expected_shape != actual_shape)
            throw std::runtime_error("Python output shape does not match declared output shape.");

        if(static_cast<int>(flattened.size()) != target.size())
            throw std::runtime_error("Python output size does not match declared output size.");

        float * data = target.data();
        for(int i = 0; i < target.size(); ++i)
            data[i] = flattened[i];
    }
}

class PythonModule: public Module
{
    std::map<std::string, matrix> inputs_;
    std::map<std::string, matrix> outputs_;
    std::map<std::string, parameter> parameters_;

    std::string python_script_;
    std::string python_function_;
    std::string python_executable_;
    std::string process_mode_;
    std::string execution_mode_;
    std::string worker_script_;

    int timeout_ms_ = 1000;

    pid_t worker_pid_ = -1;
    int worker_stdin_fd_ = -1;
    int worker_stdout_fd_ = -1;

    void BindDeclaredBuffers(const list & declarations, std::map<std::string, matrix> & bound_buffers)
    {
        for(auto declaration : declarations)
        {
            std::string name = declaration["name"];
            if(name.empty())
                continue;

            matrix buffer;
            Bind(buffer, name);
            bound_buffers[name] = buffer;
        }
    }

    void BindDeclaredParameters(const list & declarations)
    {
        for(auto declaration : declarations)
        {
            std::string name = declaration["name"];
            if(name.empty())
                continue;

            parameter bound;
            Bind(bound, name);
            parameters_[name] = bound;
        }
    }

    std::string ReadLineWithTimeout()
    {
        if(worker_stdout_fd_ < 0)
            throw exception("Python worker stdout is not connected.", path_);

        std::string line;
        while(true)
        {
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(worker_stdout_fd_, &readfds);

            struct timeval timeout;
            timeout.tv_sec = timeout_ms_ / 1000;
            timeout.tv_usec = (timeout_ms_ % 1000) * 1000;

            int ready = select(worker_stdout_fd_ + 1, &readfds, nullptr, nullptr, &timeout);
            if(ready == 0)
                throw exception("Python worker timed out.", path_);
            if(ready < 0)
                throw exception("Waiting for python worker failed.", path_);

            char c = '\0';
            ssize_t bytes_read = read(worker_stdout_fd_, &c, 1);
            if(bytes_read == 0)
                throw exception("Python worker closed its output unexpectedly.", path_);
            if(bytes_read < 0)
                throw exception("Reading from python worker failed.", path_);

            if(c == '\n')
                return line;

            line.push_back(c);
        }
    }

    void WriteLine(const std::string & line)
    {
        if(worker_stdin_fd_ < 0)
            throw exception("Python worker stdin is not connected.", path_);

        std::string payload = line + "\n";
        const char * data = payload.c_str();
        size_t remaining = payload.size();
        while(remaining > 0)
        {
            ssize_t written = write(worker_stdin_fd_, data, remaining);
            if(written < 0)
                throw exception("Writing to python worker failed.", path_);
            remaining -= static_cast<size_t>(written);
            data += written;
        }
    }

    dictionary ReadMessage()
    {
        std::string line = ReadLineWithTimeout();
        if(line.empty())
            throw exception("Python worker returned an empty message.", path_);

        value parsed = parse_json(line);
        if(!parsed.is_dictionary())
            throw exception("Python worker returned malformed JSON.", path_);
        return make_dictionary(parsed);
    }

    void HandleWorkerLogs(dictionary message)
    {
        if(!message.contains("logs"))
            return;

        for(auto entry_value : list(message["logs"]))
        {
            if(!entry_value.is_dictionary())
                continue;

            dictionary entry = make_dictionary(entry_value);
            std::string level = entry.contains("level") ? std::string(entry["level"]) : "print";
            std::string text = entry.contains("message") ? std::string(entry["message"]) : "";

            if(level == "debug")
                Notify(msg_debug, text, path_);
            else if(level == "warning")
                Notify(msg_warning, text, path_);
            else if(level == "error")
                Notify(msg_fatal_error, text, path_);
            else
                Notify(msg_print, text, path_);
        }
    }

    dictionary BuildTickPayload()
    {
        dictionary payload;
        payload["command"] = "tick";
        payload["tick"] = double(GetTick());
        payload["time"] = GetTime();
        payload["dt"] = GetTickDuration();

        dictionary inputs_dict;
        for(auto & [name, input] : inputs_)
            inputs_dict[name] = parse_json(input.json());
        payload["inputs"] = inputs_dict;

        dictionary outputs_dict;
        for(auto & [name, output] : outputs_)
            outputs_dict[name] = parse_json(output.json());
        payload["outputs"] = outputs_dict;

        dictionary parameters_dict;
        for(auto & [name, parameter_value] : parameters_)
            parameters_dict[name] = parse_json(parameter_value.json());
        payload["parameters"] = parameters_dict;

        return payload;
    }

    void PublishOutputs(dictionary message)
    {
        if(!message.contains("outputs"))
            return;

        dictionary output_values = message["outputs"];
        for(auto & [name, buffer] : outputs_)
        {
            if(!output_values.contains(name))
                continue;

            assign_matrix_from_json(buffer, output_values[name]);
        }
    }

    void SpawnWorker()
    {
        int to_child[2];
        int from_child[2];
        if(pipe(to_child) != 0 || pipe(from_child) != 0)
            throw exception("Could not create pipes for python worker.", path_);

        worker_pid_ = fork();
        if(worker_pid_ < 0)
            throw exception("Could not start python worker process.", path_);

        if(worker_pid_ == 0)
        {
            dup2(to_child[0], STDIN_FILENO);
            dup2(from_child[1], STDOUT_FILENO);

            close(to_child[0]);
            close(to_child[1]);
            close(from_child[0]);
            close(from_child[1]);

            execlp(
                python_executable_.c_str(),
                python_executable_.c_str(),
                worker_script_.c_str(),
                python_script_.c_str(),
                python_function_.c_str(),
                path_.c_str(),
                static_cast<char *>(nullptr)
            );
            _exit(127);
        }

        close(to_child[0]);
        close(from_child[1]);
        worker_stdin_fd_ = to_child[1];
        worker_stdout_fd_ = from_child[0];

        dictionary init_message = ReadMessage();
        HandleWorkerLogs(init_message);
        std::string status = init_message.contains("status") ? std::string(init_message["status"]) : "";
        if(status != "ready")
            throw exception(init_message.contains("error") ? std::string(init_message["error"]) : "Python worker failed to initialize.", path_);
    }

    void ShutdownWorker()
    {
        if(worker_stdin_fd_ >= 0)
        {
            try
            {
                dictionary message;
                message["command"] = "shutdown";
                WriteLine(message.json());
            }
            catch(...)
            {
            }
        }

        if(worker_stdin_fd_ >= 0)
            close(worker_stdin_fd_);
        if(worker_stdout_fd_ >= 0)
            close(worker_stdout_fd_);
        worker_stdin_fd_ = -1;
        worker_stdout_fd_ = -1;

        if(worker_pid_ > 0)
        {
            int status = 0;
            waitpid(worker_pid_, &status, 0);
        }
        worker_pid_ = -1;
    }

public:
    ~PythonModule() override
    {
        ShutdownWorker();
    }

    void Init() override
    {
        python_script_ = info_.contains("python") ? std::string(info_["python"]) : "";
        python_function_ = info_.contains("python_function") ? std::string(info_["python_function"]) : "tick";
        if(python_function_.empty())
            python_function_ = "tick";

        if(python_script_.empty())
            throw exception("Python-backed class is missing the \"python\" attribute.", path_);

        if(!std::filesystem::exists(python_script_))
            throw exception("Python script \"" + python_script_ + "\" could not be found.", path_);

        process_mode_ = GetParameter("process_mode").as_string();
        execution_mode_ = GetParameter("execution_mode").as_string();
        python_executable_ = GetParameter("python_executable").as_string();
        timeout_ms_ = GetParameter("timeout_ms").as_int();

        if(process_mode_.empty())
            process_mode_ = "worker";
        if(execution_mode_.empty())
            execution_mode_ = "sync";
        if(python_executable_.empty())
            python_executable_ = "python3";

        if(process_mode_ != "worker")
            throw exception("Only process_mode=\"worker\" is implemented in this phase.", path_);
        if(execution_mode_ != "sync")
            throw exception("Only execution_mode=\"sync\" is implemented in this phase.", path_);

        worker_script_ = (std::filesystem::path(kernel().classes["PythonModule"].path).parent_path() / "python_worker.py").string();
        if(!std::filesystem::exists(worker_script_))
            throw exception("Python worker helper script could not be found.", path_);

        BindDeclaredBuffers(info_["inputs"], inputs_);
        BindDeclaredBuffers(info_["outputs"], outputs_);
        BindDeclaredParameters(info_["parameters"]);

        SpawnWorker();
        Notify(msg_print, "Initialized python worker for class \"" + std::string(info_["class"]) + "\".", path_);
    }

    void Tick() override
    {
        dictionary payload = BuildTickPayload();
        WriteLine(payload.json());

        dictionary reply = ReadMessage();
        HandleWorkerLogs(reply);

        std::string status = reply.contains("status") ? std::string(reply["status"]) : "";
        if(status != "ok")
        {
            std::string error = reply.contains("error") ? std::string(reply["error"]) : "Python worker reported an unknown error.";
            throw exception(error, path_);
        }

        PublishOutputs(reply);
    }
};

INSTALL_CLASS(PythonModule)
