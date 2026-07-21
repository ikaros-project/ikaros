#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <limits>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

#include "ikaros.h"

using namespace ikaros;

namespace
{
    class ThreadPoolTestTask : public Task
    {
    public:
        ThreadPoolTestTask(std::atomic<int> & count, double duration, bool fail = false):
            count_(count),
            duration_(duration),
            fail_(fail)
        {
        }

        void Tick() override
        {
            std::this_thread::sleep_for(std::chrono::duration<double>(duration_));
            ++count_;
            if(fail_)
                throw std::runtime_error("ThreadPoolTestTask failure");
        }

        std::string Info() const override
        {
            return "ThreadPoolTestTask";
        }

    private:
        std::atomic<int> & count_;
        double duration_;
        bool fail_;
    };


    class ThreadPoolDestructionTestTask : public Task
    {
    public:
        explicit ThreadPoolDestructionTestTask(bool & destroyed): destroyed_(destroyed) {}

        ~ThreadPoolDestructionTestTask() override
        {
            destroyed_ = true;
        }

        void Tick() override {}

        std::string Info() const override
        {
            return "ThreadPoolDestructionTestTask";
        }

    private:
        bool & destroyed_;
    };


    class ThreadPoolBlockingTestTask : public Task
    {
    public:
        ThreadPoolBlockingTestTask(std::atomic<bool> & entered, std::atomic<bool> & release):
            entered_(entered),
            release_(release)
        {
        }

        void Tick() override
        {
            entered_ = true;
            while(!release_)
                std::this_thread::yield();
        }

        std::string Info() const override
        {
            return "ThreadPoolBlockingTestTask";
        }

    private:
        std::atomic<bool> & entered_;
        std::atomic<bool> & release_;
    };


    template<typename Exception, typename Function>
    void
    RequireThrows(Function && function, const std::string & description)
    {
        try
        {
            function();
        }
        catch(const Exception &)
        {
            return;
        }
        throw std::runtime_error(description);
    }
}

class TaskLifecycleTestModule : public Module
{
    parameter duration;
    parameter fail;
    parameter failAfterTasks;
    parameter failInit;
    parameter label;
    parameter outputValue;
    matrix output;
    matrix postTaskFailureProbe;
    std::atomic<bool> tickActive = false;

    void Init() override
    {
        Bind(duration, "duration");
        Bind(fail, "fail");
        Bind(failAfterTasks, "fail_after_tasks");
        Bind(failInit, "fail_init");
        Bind(label, "label");
        Bind(outputValue, "output_value");
        Bind(output, "OUTPUT");

        Notify(msg_print, label.as_string() + " INIT");
        if(failInit.as_bool())
            throw std::runtime_error("TaskLifecycleTestModule Init failure " + label.as_string());

        if(failAfterTasks.as_bool())
        {
            postTaskFailureProbe.realloc(1);
            postTaskFailureProbe.last();
        }
    }

    void Tick() override
    {
        tickActive = true;
        const std::string taskLabel = label.as_string();
        Notify(msg_print, taskLabel + " TICK_START");
        std::this_thread::sleep_for(std::chrono::duration<double>(duration.as_double()));
        output(0) = outputValue.as_float();

        if(fail.as_bool())
        {
            tickActive = false;
            Notify(msg_print, taskLabel + " THROW");
            throw std::runtime_error("TaskLifecycleTestModule failure " + taskLabel);
        }

        Notify(msg_print, taskLabel + " TICK_END");
        tickActive = false;

        if(failAfterTasks.as_bool())
        {
            postTaskFailureProbe.last().realloc(2);
            Notify(msg_print, taskLabel + " POST_TASK_FAILURE_ARMED");
        }
    }

    void Stop() override
    {
        Notify(msg_print, label.as_string() + (tickActive.load() ? " STOP_DURING_TICK" : " STOP_AFTER_TICK"));
    }
};

INSTALL_CLASS(TaskLifecycleTestModule)


class ThreadPoolAPITestModule : public Module
{
    void Init() override
    {
        RequireThrows<std::invalid_argument>([]() { ThreadPool invalid_pool(0); },
                                             "ThreadPool accepted zero workers");

        bool destroyed = false;
        Task * destruction_task = new ThreadPoolDestructionTestTask(destroyed);
        delete destruction_task;
        if(!destroyed)
            throw std::runtime_error("Task base destruction was not virtual");

        RequireThrows<std::invalid_argument>([]()
        {
            TaskSequence invalid_sequence(std::vector<Task *>{nullptr});
        }, "TaskSequence accepted a null task");

        ThreadPool pool(1);
        RequireThrows<std::invalid_argument>([&pool]() { pool.submit(nullptr); },
                                             "ThreadPool accepted a null sequence");

        std::atomic<int> count = 0;
        ThreadPoolTestTask task(count, 0.02);
        auto sequence = std::make_shared<TaskSequence>(std::vector<Task *>{&task});
        pool.submit(sequence);
        RequireThrows<std::logic_error>([&pool, &sequence]() { pool.submit(sequence); },
                                        "ThreadPool accepted a duplicate active sequence");
        if(!sequence->waitForCompletion(1.0) || count.load() != 1)
            throw std::runtime_error("ThreadPool did not complete its first sequence run");

        pool.submit(sequence);
        if(sequence->isCompleted())
            throw std::runtime_error("Resubmitted TaskSequence retained stale completion state");
        if(!sequence->waitForCompletion(1.0) || count.load() != 2)
            throw std::runtime_error("ThreadPool did not complete a resubmitted sequence");

        std::atomic<bool> direct_entered = false;
        std::atomic<bool> direct_release = false;
        ThreadPoolBlockingTestTask direct_task(direct_entered, direct_release);
        TaskSequence direct_sequence(std::vector<Task *>{&direct_task});
        std::thread direct_worker([&direct_sequence]() { direct_sequence.execute(); });
        while(!direct_entered)
            std::this_thread::yield();
        bool concurrent_execution_rejected = false;
        try
        {
            direct_sequence.execute();
        }
        catch(const std::logic_error &)
        {
            concurrent_execution_rejected = true;
        }
        direct_release = true;
        direct_worker.join();
        if(!concurrent_execution_rejected)
            throw std::runtime_error("TaskSequence allowed concurrent direct execution");

        TaskSequence idle_sequence(std::vector<Task *>{});
        RequireThrows<std::invalid_argument>([&idle_sequence]()
        {
            idle_sequence.waitForCompletion(std::numeric_limits<double>::quiet_NaN());
        }, "TaskSequence accepted a non-finite timeout");
        const auto wait_start = std::chrono::steady_clock::now();
        if(idle_sequence.waitForCompletion(0.0005))
            throw std::runtime_error("Idle TaskSequence unexpectedly completed");
        const double wait_duration = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - wait_start).count();
        if(wait_duration < 0.00025)
            throw std::runtime_error("TaskSequence truncated a sub-millisecond timeout");

        ThreadPoolTestTask failing_task(count, 0, true);
        auto failing_sequence = std::make_shared<TaskSequence>(std::vector<Task *>{&failing_task});
        pool.submit(failing_sequence);
        if(!failing_sequence->waitForCompletion(1.0) || !failing_sequence->hasError())
            throw std::runtime_error("TaskSequence did not retain its task error");
        RequireThrows<std::runtime_error>([&failing_sequence]() { failing_sequence->rethrowIfError(); },
                                          "TaskSequence did not rethrow its task error");

        Notify(msg_print, "THREAD_POOL_API_OK");
    }
};

INSTALL_CLASS(ThreadPoolAPITestModule)


class SocketDeadlineTestModule : public Module
{
    void Init() override
    {
        int listener = ::socket(AF_INET, SOCK_STREAM, 0);
        if(listener == -1)
            throw std::system_error(errno, std::system_category(), "Could not create deadline test listener");

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;
        if(bind(listener, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == -1 ||
           listen(listener, 1) == -1)
        {
            int error = errno;
            close(listener);
            throw std::system_error(error, std::system_category(), "Could not start deadline test listener");
        }

        socklen_t address_size = sizeof(address);
        if(getsockname(listener, reinterpret_cast<sockaddr *>(&address), &address_size) == -1)
        {
            int error = errno;
            close(listener);
            throw std::system_error(error, std::system_category(), "Could not inspect deadline test listener");
        }

        std::thread server([listener]()
        {
            int client;
            do
                client = accept(listener, nullptr, nullptr);
            while(client == -1 && errno == EINTR);

            if(client != -1)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
                close(client);
            }
            close(listener);
        });

        try
        {
            Socket client(std::chrono::milliseconds(50));
            const char request[] = "GET / HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
            if(!client.SendRequest("127.0.0.1", ntohs(address.sin_port), request))
                throw std::runtime_error("Socket deadline test could not connect");

            char response;
            auto read_start = std::chrono::steady_clock::now();
            int result = client.ReadData(&response, 1);
            auto read_duration = std::chrono::steady_clock::now() - read_start;
            client.Close();

            if(result != -1)
                throw std::runtime_error("Socket read did not report its deadline");
            if(read_duration < std::chrono::milliseconds(25) ||
               read_duration > std::chrono::milliseconds(200))
                throw std::runtime_error("Socket read deadline was outside its expected range");
        }
        catch(...)
        {
            server.join();
            throw;
        }

        server.join();
        Notify(msg_print, "SOCKET_DEADLINE_OK");
    }
};

INSTALL_CLASS(SocketDeadlineTestModule)


class SocketClientAPITestModule : public Module
{
    void Init() override
    {
        Socket client(std::chrono::milliseconds(50));
        if(client.Poll())
            throw std::runtime_error("Closed Socket reported pending data");
        if(client.SendRequest(nullptr, 80, "") ||
           client.SendRequest("127.0.0.1", 0, "") ||
           client.ReadData(nullptr, 1) != -1)
            throw std::runtime_error("Socket accepted invalid public API input");

        int listener = ::socket(AF_INET, SOCK_STREAM, 0);
        if(listener == -1)
            throw std::system_error(errno, std::system_category(), "Could not create Socket API test listener");

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;
        if(bind(listener, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == -1 ||
           listen(listener, 1) == -1)
        {
            int error = errno;
            close(listener);
            throw std::system_error(error, std::system_category(), "Could not start Socket API test listener");
        }

        socklen_t address_size = sizeof(address);
        if(getsockname(listener, reinterpret_cast<sockaddr *>(&address), &address_size) == -1)
        {
            int error = errno;
            close(listener);
            throw std::system_error(error, std::system_category(), "Could not inspect Socket API test listener");
        }

        std::thread server([listener]()
        {
            int peer;
            do
                peer = accept(listener, nullptr, nullptr);
            while(peer == -1 && errno == EINTR);

            if(peer != -1)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
                close(peer);
            }
            close(listener);
        });

        std::string oversized_request(16 * 1024 * 1024, 'x');
        bool complete = client.SendRequest("127.0.0.1", ntohs(address.sin_port),
                                           oversized_request.data(),
                                           static_cast<long>(oversized_request.size()));
        client.Close();
        server.join();
        if(complete)
            throw std::runtime_error("Socket accepted a partially written request");
        if(client.Poll())
            throw std::runtime_error("Failed Socket retained descriptor ownership");

        Notify(msg_print, "SOCKET_CLIENT_API_OK");
    }
};

INSTALL_CLASS(SocketClientAPITestModule)


class SocketSelectCapacityTestModule : public Module
{
    void Init() override
    {
        std::vector<int> descriptors;
        int descriptor = -1;
        while(descriptor < FD_SETSIZE)
        {
            descriptor = open("/dev/null", O_RDONLY);
            if(descriptor == -1)
                break;
            descriptors.push_back(descriptor);
        }

        bool rejected = false;
        if(descriptor >= FD_SETSIZE)
        {
            try
            {
                ServerSocket server(0, "127.0.0.1");
            }
            catch(const std::exception &)
            {
                rejected = true;
            }
        }

        for(int open_descriptor : descriptors)
            close(open_descriptor);

        if(descriptor < FD_SETSIZE)
            throw std::runtime_error("Could not reach FD_SETSIZE for the Socket capacity test");
        if(!rejected)
            throw std::runtime_error("ServerSocket accepted a descriptor outside select() capacity");

        Notify(msg_print, "SOCKET_SELECT_CAPACITY_OK");
    }
};

INSTALL_CLASS(SocketSelectCapacityTestModule)


class SocketConstructionTestModule : public Module
{
    void Init() override
    {
        int baseline_descriptor = open("/dev/null", O_RDONLY);
        if(baseline_descriptor == -1)
            throw std::system_error(errno, std::system_category(), "Could not establish descriptor baseline");
        close(baseline_descriptor);

        for(int attempt = 0; attempt < 32; ++attempt)
        {
            RequireThrows<std::invalid_argument>([]()
            {
                ServerSocket invalid_address(0, "not-an-ip-address");
            }, "ServerSocket accepted an invalid bind address");
        }

        int reused_descriptor = open("/dev/null", O_RDONLY);
        if(reused_descriptor == -1)
            throw std::system_error(errno, std::system_category(), "Could not inspect descriptor reuse");
        close(reused_descriptor);
        if(reused_descriptor != baseline_descriptor)
            throw std::runtime_error("Failed ServerSocket construction leaked descriptors");

        RequireThrows<std::invalid_argument>([]()
        {
            ServerSocket invalid_port(-1, "127.0.0.1");
        }, "ServerSocket accepted a negative port");
        RequireThrows<std::invalid_argument>([]()
        {
            ServerSocket invalid_port(65536, "127.0.0.1");
        }, "ServerSocket accepted a port above 65535");

        Notify(msg_print, "SOCKET_CONSTRUCTION_OK");
    }
};

INSTALL_CLASS(SocketConstructionTestModule)
