#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include "ikaros.h"
#include "Kernel/session_logging.h"

using namespace ikaros;

namespace
{
    struct TransportState
    {
        std::mutex mutex;
        std::condition_variable condition;
        std::vector<std::string> received;
        bool first_started = false;
        bool release_first = false;
    };


    void
    require(bool condition, const std::string & message)
    {
        if(!condition)
            throw std::runtime_error("SessionLoggingTestModule: " + message);
    }
}


class SessionLoggingTestModule : public Module
{
public:
    void
    Init() override
    {
        using namespace std::chrono_literals;

        auto transport_state = std::make_shared<TransportState>();
        SessionLogDispatcher dispatcher(
            2,
            [transport_state](SessionLogEvent event)
            {
                {
                    std::unique_lock<std::mutex> lock(transport_state->mutex);
                    transport_state->received.push_back(event.path);
                    if(event.path == "first")
                    {
                        transport_state->first_started = true;
                        transport_state->condition.notify_all();
                        transport_state->condition.wait(lock, [&]()
                        {
                            return transport_state->release_first;
                        });
                    }
                }

                if(event.path == "throws")
                    throw std::runtime_error("expected transport failure");
            },
            1s);

        const bool first_enqueued = dispatcher.Enqueue({"first"});
        bool first_started = false;
        {
            std::unique_lock<std::mutex> lock(transport_state->mutex);
            first_started = transport_state->condition.wait_for(lock, 1s, [&]()
            {
                return transport_state->first_started;
            });
        }

        const bool second_enqueued = dispatcher.Enqueue({"throws"});
        const bool third_enqueued = dispatcher.Enqueue({"third"});
        const bool overflow_rejected = !dispatcher.Enqueue({"overflow"});

        {
            std::lock_guard<std::mutex> lock(transport_state->mutex);
            transport_state->release_first = true;
        }
        transport_state->condition.notify_all();

        const bool became_idle = dispatcher.WaitUntilIdle(1s);
        std::vector<std::string> received;
        {
            std::lock_guard<std::mutex> lock(transport_state->mutex);
            received = transport_state->received;
        }

        require(first_enqueued && first_started,
                "worker did not begin processing the first event");
        require(second_enqueued && third_enqueued && overflow_rejected,
                "bounded queue did not accept exactly its configured capacity");
        require(became_idle, "queue did not become idle");
        require(received == std::vector<std::string>({"first", "throws", "third"}),
                "events were not delivered in FIFO order");
        require(dispatcher.DroppedCount() == 1,
                "queue did not count the rejected event");

        require(dispatcher.Enqueue({"after_failure"}),
                "worker stopped accepting events after a transport exception");
        require(dispatcher.WaitUntilIdle(1s),
                "queue did not process an event after a transport exception");

        {
            std::lock_guard<std::mutex> lock(transport_state->mutex);
            require(transport_state->received ==
                        std::vector<std::string>({"first", "throws", "third", "after_failure"}),
                    "worker did not continue after a transport exception");
        }

        std::cout << "SESSION LOGGING QUEUE TEST OK\n";
    }
};

INSTALL_CLASS(SessionLoggingTestModule)
