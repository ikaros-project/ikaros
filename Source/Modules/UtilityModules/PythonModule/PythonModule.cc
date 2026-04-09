#include "ikaros.h"

#include <cerrno>
#include <csignal>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace ikaros;

namespace
{
    constexpr size_t kSharedMemoryAlignment = 64;

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

    size_t align_bytes(size_t offset)
    {
        size_t remainder = offset % kSharedMemoryAlignment;
        return remainder == 0 ? offset : offset + (kSharedMemoryAlignment - remainder);
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
    struct SharedBufferView
    {
        std::string name;
        std::vector<int> shape;
        size_t offset_bytes = 0;
        size_t size_bytes = 0;
    };

    struct SharedMemoryRegion
    {
        std::string shm_name;
        int fd = -1;
        size_t total_bytes = 0;
        void * mapped = nullptr;
        std::map<std::string, SharedBufferView> buffers;
    };

    std::map<std::string, matrix> inputs_;
    std::map<std::string, matrix> outputs_;
    std::map<std::string, parameter> parameters_;

    std::string python_script_;
    std::string python_function_;
    std::string python_executable_;
    std::string process_mode_;
    std::string execution_mode_;
    std::string worker_script_;
    std::string on_error_;

    int timeout_ms_ = 1000;
    bool restart_on_crash_ = false;
    bool async_in_flight_ = false;
    bool use_shared_memory_transport_ = true;
    bool use_global_names_ = false;
    std::string stdout_buffer_;

    pid_t worker_pid_ = -1;
    int worker_stdin_fd_ = -1;
    int worker_stdout_fd_ = -1;
    SharedMemoryRegion input_region_;
    SharedMemoryRegion output_region_;

    void DestroySharedRegion(SharedMemoryRegion & region)
    {
        if(region.mapped != nullptr && region.total_bytes > 0)
            munmap(region.mapped, region.total_bytes);
        if(region.fd >= 0)
            close(region.fd);
        if(!region.shm_name.empty())
            shm_unlink(region.shm_name.c_str());

        region.shm_name.clear();
        region.fd = -1;
        region.total_bytes = 0;
        region.mapped = nullptr;
        region.buffers.clear();
    }

    SharedMemoryRegion CreateSharedRegion(const std::string & suffix, const std::map<std::string, matrix> & buffers)
    {
        SharedMemoryRegion region;
        static uint64_t shm_sequence = 0;
        std::ostringstream name;
        name << "/ikp_" << getpid() << "_" << ++shm_sequence << "_" << (suffix == "inputs" ? "i" : "o");
        region.shm_name = name.str();

        size_t offset = 0;
        for(const auto & [buffer_name, buffer] : buffers)
        {
            SharedBufferView view;
            view.name = buffer_name;
            view.shape = buffer.shape();
            view.offset_bytes = align_bytes(offset);
            view.size_bytes = static_cast<size_t>(buffer.size()) * sizeof(float);
            region.buffers[buffer_name] = view;
            offset = view.offset_bytes + view.size_bytes;
        }
        region.total_bytes = std::max<size_t>(offset, 1);

        region.fd = shm_open(region.shm_name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
        if(region.fd < 0)
            throw exception("Could not create shared memory for python worker: " + std::string(std::strerror(errno)), path_);

        if(ftruncate(region.fd, static_cast<off_t>(region.total_bytes)) != 0)
        {
            DestroySharedRegion(region);
            throw exception("Could not resize shared memory for python worker.", path_);
        }

        region.mapped = mmap(nullptr, region.total_bytes, PROT_READ | PROT_WRITE, MAP_SHARED, region.fd, 0);
        if(region.mapped == MAP_FAILED)
        {
            region.mapped = nullptr;
            DestroySharedRegion(region);
            throw exception("Could not map shared memory for python worker.", path_);
        }

        std::memset(region.mapped, 0, region.total_bytes);
        return region;
    }

    dictionary BuildRegionDescription(const SharedMemoryRegion & region)
    {
        dictionary description;
        description["name"] = region.shm_name;
        description["size_bytes"] = double(region.total_bytes);

        list buffers;
        for(const auto & [buffer_name, buffer] : region.buffers)
        {
            dictionary entry;
            entry["name"] = buffer_name;
            list shape;
            for(int dim : buffer.shape)
                shape.push_back(double(dim));
            entry["shape"] = shape;
            entry["offset_bytes"] = double(buffer.offset_bytes);
            entry["size_bytes"] = double(buffer.size_bytes);
            entry["dtype"] = "float32";
            buffers.push_back(entry);
        }
        description["buffers"] = buffers;
        return description;
    }

    void CopyToSharedRegion(const std::map<std::string, matrix> & local_buffers, const SharedMemoryRegion & region)
    {
        if(region.mapped == nullptr)
            throw exception("Python worker shared memory is not mapped.", path_);

        char * base = static_cast<char *>(region.mapped);
        for(const auto & [buffer_name, buffer] : local_buffers)
        {
            const auto region_it = region.buffers.find(buffer_name);
            if(region_it == region.buffers.end())
                continue;

            const SharedBufferView & view = region_it->second;
            std::memcpy(base + view.offset_bytes, buffer.data(), view.size_bytes);
        }
    }

    void CopyFromSharedRegion(std::map<std::string, matrix> & local_buffers, const SharedMemoryRegion & region)
    {
        if(region.mapped == nullptr)
            throw exception("Python worker shared memory is not mapped.", path_);

        const char * base = static_cast<const char *>(region.mapped);
        for(auto & [buffer_name, buffer] : local_buffers)
        {
            const auto region_it = region.buffers.find(buffer_name);
            if(region_it == region.buffers.end())
                continue;

            const SharedBufferView & view = region_it->second;
            std::memcpy(buffer.data(), base + view.offset_bytes, view.size_bytes);
        }
    }

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

    std::string ResolvePythonExecutable()
    {
        if(!python_executable_.empty())
            return python_executable_;

        if(kernel().info_.contains_non_null("python_executable"))
        {
            std::string kernel_python_executable = std::string(kernel().info_["python_executable"]);
            if(!kernel_python_executable.empty())
                return kernel_python_executable;
        }

        return "python3";
    }

    std::optional<std::string> TryReadLine(int timeout_ms)
    {
        if(worker_stdout_fd_ < 0)
            throw exception("Python worker stdout is not connected.", path_);

        while(true)
        {
            auto newline = stdout_buffer_.find('\n');
            if(newline != std::string::npos)
            {
                std::string line = stdout_buffer_.substr(0, newline);
                stdout_buffer_.erase(0, newline + 1);
                return line;
            }

            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(worker_stdout_fd_, &readfds);

            struct timeval timeout;
            timeout.tv_sec = timeout_ms / 1000;
            timeout.tv_usec = (timeout_ms % 1000) * 1000;

            int ready = select(worker_stdout_fd_ + 1, &readfds, nullptr, nullptr, &timeout);
            if(ready == 0)
                return std::nullopt;
            if(ready < 0)
                throw exception("Waiting for python worker failed.", path_);

            char buffer[1024];
            ssize_t bytes_read = read(worker_stdout_fd_, buffer, sizeof(buffer));
            if(bytes_read == 0)
                throw exception("Python worker closed its output unexpectedly.", path_);
            if(bytes_read < 0)
                throw exception("Reading from python worker failed.", path_);
            stdout_buffer_.append(buffer, static_cast<size_t>(bytes_read));
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

    dictionary ReadMessage(int timeout_ms)
    {
        auto line = TryReadLine(timeout_ms);
        if(!line.has_value())
            throw exception("Python worker timed out.", path_);
        if(line->empty())
            throw exception("Python worker returned an empty message.", path_);

        value parsed = parse_json(*line);
        if(!parsed.is_dictionary())
            throw exception("Python worker returned malformed JSON.", path_);
        return make_dictionary(parsed);
    }

    std::optional<dictionary> PollMessage()
    {
        auto line = TryReadLine(0);
        if(!line.has_value())
            return std::nullopt;
        if(line->empty())
            throw exception("Python worker returned an empty message.", path_);

        value parsed = parse_json(*line);
        if(!parsed.is_dictionary())
            throw exception("Python worker returned malformed JSON.", path_);
        return make_dictionary(parsed);
    }

    void ResetWorkerHandles()
    {
        if(worker_stdin_fd_ >= 0)
            close(worker_stdin_fd_);
        if(worker_stdout_fd_ >= 0)
            close(worker_stdout_fd_);
        worker_stdin_fd_ = -1;
        worker_stdout_fd_ = -1;
        worker_pid_ = -1;
        stdout_buffer_.clear();
        async_in_flight_ = false;
    }

    bool WorkerExited(int * exit_status = nullptr)
    {
        if(worker_pid_ <= 0)
            return true;

        int status = 0;
        pid_t result = waitpid(worker_pid_, &status, WNOHANG);
        if(result == 0)
            return false;
        if(result < 0)
            return false;

        if(exit_status)
            *exit_status = status;
        return true;
    }

    std::string DescribeWorkerExitStatus(int status)
    {
        if(WIFEXITED(status))
            return "Python worker exited with status " + std::to_string(WEXITSTATUS(status)) + ".";
        if(WIFSIGNALED(status))
            return "Python worker terminated by signal " + std::to_string(WTERMSIG(status)) + ".";
        return "Python worker terminated unexpectedly.";
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
            std::string level = entry.contains_non_null("level") ? std::string(entry["level"]) : "print";
            std::string text = entry.contains_non_null("message") ? std::string(entry["message"]) : "";

            if(level == "debug")
                kernel().Notify(msg_debug, text, path_);
            else if(level == "warning")
                kernel().Notify(msg_warning, text, path_);
            else if(level == "error")
                kernel().Notify(msg_fatal_error, text, path_);
            else
                kernel().Notify(msg_print, text, path_);
        }
    }

    void ResetOutputs()
    {
        for(auto & [name, buffer] : outputs_)
            buffer.set(0);
    }

    void HandleWorkerFailure(const std::string & failure_message)
    {
        if(on_error_ == "zero")
        {
            Notify(msg_warning, failure_message, path_);
            ResetOutputs();
            return;
        }

        if(on_error_ == "log")
        {
            Notify(msg_warning, failure_message, path_);
            return;
        }

        throw exception(failure_message, path_);
    }

    dictionary BuildTickPayload()
    {
        dictionary payload;
        payload["command"] = "tick";
        payload["tick"] = double(GetTick());
        payload["time"] = GetTime();
        payload["dt"] = GetTickDuration();

        if(!use_shared_memory_transport_)
        {
            dictionary inputs_dict;
            for(auto & [name, input] : inputs_)
                inputs_dict[name] = parse_json(input.json());
            payload["inputs"] = inputs_dict;

            dictionary outputs_dict;
            for(auto & [name, output] : outputs_)
                outputs_dict[name] = parse_json(output.json());
            payload["outputs"] = outputs_dict;
        }

        dictionary parameters_dict;
        for(auto & [name, parameter_value] : parameters_)
            parameters_dict[name] = parse_json(parameter_value.json());
        payload["parameters"] = parameters_dict;

        return payload;
    }

    void PublishOutputs(dictionary message)
    {
        if(use_shared_memory_transport_)
        {
            CopyFromSharedRegion(outputs_, output_region_);
            return;
        }

        if(!message.contains_non_null("outputs"))
            return;

        value & output_values_value = message["outputs"];
        if(!output_values_value.is_dictionary())
            throw exception("Python worker returned malformed outputs payload.", path_);

        dictionary output_values = make_dictionary(output_values_value);
        for(auto & [name, buffer] : outputs_)
        {
            if(!output_values.contains(name))
                continue;

            assign_matrix_from_json(buffer, output_values[name]);
        }
    }

    bool HandleWorkerReply(dictionary reply)
    {
        HandleWorkerLogs(reply);

        std::string status = reply.contains_non_null("status") ? std::string(reply["status"]) : "";
        if(status != "ok")
        {
            std::string error = reply.contains_non_null("error") ? std::string(reply["error"]) : "Python worker reported an unknown error.";
            if(reply.contains_non_null("traceback"))
                error += "\n" + std::string(reply["traceback"]);
            HandleWorkerFailure(error);
            return false;
        }

        try
        {
            PublishOutputs(reply);
        }
        catch(const std::exception & e)
        {
            HandleWorkerFailure(std::string(e.what()));
            return false;
        }
        return true;
    }

    void SpawnWorker()
    {
        ShutdownWorker();
        DestroySharedRegion(input_region_);
        DestroySharedRegion(output_region_);
        use_shared_memory_transport_ = true;
        input_region_ = CreateSharedRegion("inputs", inputs_);
        output_region_ = CreateSharedRegion("outputs", outputs_);

        int to_child[2];
        int from_child[2];
        int exec_status[2];
        if(pipe(to_child) != 0 || pipe(from_child) != 0 || pipe(exec_status) != 0)
            throw exception("Could not create pipes for python worker.", path_);

        if(fcntl(exec_status[1], F_SETFD, FD_CLOEXEC) < 0)
        {
            close(to_child[0]);
            close(to_child[1]);
            close(from_child[0]);
            close(from_child[1]);
            close(exec_status[0]);
            close(exec_status[1]);
            throw exception("Could not configure python worker startup pipe.", path_);
        }

        worker_pid_ = fork();
        if(worker_pid_ < 0)
        {
            close(to_child[0]);
            close(to_child[1]);
            close(from_child[0]);
            close(from_child[1]);
            close(exec_status[0]);
            close(exec_status[1]);
            throw exception("Could not start python worker process.", path_);
        }

        if(worker_pid_ == 0)
        {
            dup2(to_child[0], STDIN_FILENO);
            dup2(from_child[1], STDOUT_FILENO);

            close(to_child[0]);
            close(to_child[1]);
            close(from_child[0]);
            close(from_child[1]);
            close(exec_status[0]);

            execlp(
                python_executable_.c_str(),
                python_executable_.c_str(),
                worker_script_.c_str(),
                python_script_.c_str(),
                python_function_.c_str(),
                path_.c_str(),
                static_cast<char *>(nullptr)
            );

            std::string exec_error = "Failed to execute python interpreter \"" + python_executable_ + "\": " + std::strerror(errno);
            write(exec_status[1], exec_error.c_str(), exec_error.size());
            close(exec_status[1]);
            _exit(127);
        }

        close(to_child[0]);
        close(from_child[1]);
        close(exec_status[1]);
        worker_stdin_fd_ = to_child[1];
        worker_stdout_fd_ = from_child[0];

        char exec_buffer[512];
        ssize_t exec_bytes = read(exec_status[0], exec_buffer, sizeof(exec_buffer) - 1);
        close(exec_status[0]);
        if(exec_bytes < 0)
        {
            ShutdownWorker();
            throw exception("Could not read python worker startup status.", path_);
        }
        if(exec_bytes > 0)
        {
            exec_buffer[exec_bytes] = '\0';
            ShutdownWorker();
            throw exception(std::string(exec_buffer), path_);
        }

        dictionary init_payload;
        init_payload["command"] = "init";
        init_payload["tick"] = 0.0;
        init_payload["time"] = 0.0;
        init_payload["dt"] = 0.0;
        init_payload["inputs_region"] = BuildRegionDescription(input_region_);
        init_payload["outputs_region"] = BuildRegionDescription(output_region_);
        init_payload["use_global_names"] = use_global_names_;

        dictionary parameters_dict;
        for(auto & [name, parameter_value] : parameters_)
            parameters_dict[name] = parse_json(parameter_value.json());
        init_payload["parameters"] = parameters_dict;

        WriteLine(init_payload.json());
        dictionary init_message = ReadMessage(timeout_ms_);
        HandleWorkerLogs(init_message);
        std::string status = init_message.contains_non_null("status") ? std::string(init_message["status"]) : "";
        if(status != "ready")
        {
            std::string error = init_message.contains_non_null("error") ? std::string(init_message["error"]) : "Python worker failed to initialize.";
            if(init_message.contains_non_null("traceback"))
                error += "\n" + std::string(init_message["traceback"]);
            ShutdownWorker();
            throw exception(error, path_);
        }

        use_shared_memory_transport_ = !init_message.contains_non_null("transport") || std::string(init_message["transport"]) != "json";
    }

    void ShutdownWorker()
    {
        pid_t worker_pid = worker_pid_;

        if(worker_stdin_fd_ >= 0)
        {
            try
            {
                dictionary message;
                message["command"] = "shutdown";
                WriteLine(message.json());

                if(worker_stdout_fd_ >= 0)
                {
                    dictionary reply = ReadMessage(std::max(timeout_ms_, 100));
                    HandleWorkerLogs(reply);

                    std::string status = reply.contains_non_null("status") ? std::string(reply["status"]) : "";
                    if(status == "error")
                    {
                        std::string error = reply.contains_non_null("error") ? std::string(reply["error"]) : "Python worker shutdown failed.";
                        if(reply.contains_non_null("traceback"))
                            error += "\n" + std::string(reply["traceback"]);
                        Notify(msg_warning, error, path_);
                    }
                }
            }
            catch(...)
            {
            }
        }

        ResetWorkerHandles();
        DestroySharedRegion(input_region_);
        DestroySharedRegion(output_region_);

        if(worker_pid > 0)
        {
            int status = 0;
            waitpid(worker_pid, &status, 0);
        }
    }

    bool RestartWorker(const std::string & reason)
    {
        try
        {
            Notify(msg_warning, reason + " Restarting python worker.", path_);
            SpawnWorker();
            return true;
        }
        catch(const std::exception & restart_error)
        {
            HandleWorkerFailure("Python worker restart failed: " + std::string(restart_error.what()));
        }
        catch(...)
        {
            HandleWorkerFailure("Python worker restart failed with an unknown error.");
        }
        return false;
    }

    bool EnsureWorkerAvailable()
    {
        int status = 0;
        if(!WorkerExited(&status))
            return true;

        ResetWorkerHandles();
        std::string message = DescribeWorkerExitStatus(status);
        if(restart_on_crash_)
            return RestartWorker(message);

        HandleWorkerFailure(message);
        return false;
    }

    bool LaunchAsyncTick()
    {
        if(async_in_flight_)
            return false;
        if(!EnsureWorkerAvailable())
            return false;

        dictionary payload = BuildTickPayload();
        try
        {
            if(use_shared_memory_transport_)
                CopyToSharedRegion(inputs_, input_region_);
            WriteLine(payload.json());
            async_in_flight_ = true;
            return true;
        }
        catch(const std::exception & e)
        {
            if(restart_on_crash_ && RestartWorker("Python worker communication failed: " + std::string(e.what())))
                HandleWorkerFailure("Python worker failed during async launch; outputs were not updated.");
            else
                HandleWorkerFailure("Python worker communication failed: " + std::string(e.what()));
            return false;
        }
    }

    void PollAsyncTick()
    {
        if(!async_in_flight_)
            return;

        if(!EnsureWorkerAvailable())
        {
            async_in_flight_ = false;
            return;
        }

        try
        {
            auto reply = PollMessage();
            if(!reply.has_value())
                return;

            async_in_flight_ = false;
            HandleWorkerReply(*reply);
        }
        catch(const std::exception & e)
        {
            async_in_flight_ = false;
            if(restart_on_crash_ && RestartWorker("Python worker did not return a valid async response: " + std::string(e.what())))
                HandleWorkerFailure("Python worker failed during async execution; outputs were not updated.");
            else
                HandleWorkerFailure("Python worker did not return a valid async response: " + std::string(e.what()));
        }
    }

public:
    ~PythonModule() override
    {
        ShutdownWorker();
    }

    void Init() override
    {
        python_script_ = info_.contains_non_null("python") ? std::string(info_["python"]) : "";
        python_function_ = info_.contains_non_null("python_function") ? std::string(info_["python_function"]) : "tick";
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
        on_error_ = GetParameter("on_error").as_string();
        restart_on_crash_ = GetParameter("restart_on_crash").as_bool();
        use_global_names_ = GetParameter("use_global_names").as_bool();

        if(process_mode_.empty())
            process_mode_ = "worker";
        if(execution_mode_.empty())
            execution_mode_ = "sync";
        if(on_error_.empty())
            on_error_ = "pause";

        python_executable_ = ResolvePythonExecutable();

        if(process_mode_ != "worker")
            throw exception("Only process_mode=\"worker\" is implemented in this phase.", path_);
        if(execution_mode_ != "sync" && execution_mode_ != "async")
            throw exception("execution_mode must be \"sync\" or \"async\".", path_);

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
        if(execution_mode_ == "async")
        {
            PollAsyncTick();
            if(!async_in_flight_)
                LaunchAsyncTick();
            return;
        }

        if(!EnsureWorkerAvailable())
            return;

        dictionary payload = BuildTickPayload();
        try
        {
            if(use_shared_memory_transport_)
                CopyToSharedRegion(inputs_, input_region_);
            WriteLine(payload.json());
        }
        catch(const std::exception & e)
        {
            if(restart_on_crash_ && RestartWorker("Python worker communication failed: " + std::string(e.what())))
                HandleWorkerFailure("Python worker failed during this tick; outputs were not updated.");
            else
                HandleWorkerFailure("Python worker communication failed: " + std::string(e.what()));
            return;
        }

        dictionary reply;
        try
        {
            reply = ReadMessage(timeout_ms_);
        }
        catch(const std::exception & e)
        {
            if(restart_on_crash_ && RestartWorker("Python worker did not return a valid response: " + std::string(e.what())))
                HandleWorkerFailure("Python worker failed during this tick; outputs were not updated.");
            else
                HandleWorkerFailure("Python worker did not return a valid response: " + std::string(e.what()));
            return;
        }

        HandleWorkerReply(reply);
    }
};

INSTALL_CLASS(PythonModule)
