#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace ikaros
{
    class Kernel;

    struct SessionLogEvent
    {
        std::string path;
    };

    class SessionLogDispatcher
    {
    public:
        using Transport = std::function<void(SessionLogEvent)>;

        SessionLogDispatcher(std::size_t capacity,
                             Transport transport,
                             std::chrono::milliseconds shutdown_wait =
                                 std::chrono::milliseconds(250));
        SessionLogDispatcher(const SessionLogDispatcher &) = delete;
        SessionLogDispatcher & operator=(const SessionLogDispatcher &) = delete;
        SessionLogDispatcher(SessionLogDispatcher &&) = delete;
        SessionLogDispatcher & operator=(SessionLogDispatcher &&) = delete;
        ~SessionLogDispatcher();

        bool Enqueue(SessionLogEvent event);
        bool WaitUntilIdle(std::chrono::milliseconds timeout);
        std::size_t DroppedCount() const;
        std::vector<std::string> TakeStatusMessages();

    private:
        struct State;

        std::shared_ptr<State> state_;
        std::thread worker_;
        std::chrono::milliseconds shutdown_wait_;
    };

    void QueueSessionLogEvent(Kernel & kernel, const std::string & endpoint, const std::string & event_name);
    void QueueProcessStartLogEvent(Kernel & kernel);
    void QueueProcessExitLogEvent(Kernel & kernel);
    void ReportSessionLogStatus(Kernel & kernel);
}
