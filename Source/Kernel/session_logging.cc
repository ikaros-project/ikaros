#include "session_logging.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <filesystem>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <utility>

#include <curl/curl.h>

#include "ikaros.h"

namespace ikaros
{
    class KernelSessionLoggingAccess
    {
    public:
        static long SessionID(const Kernel & kernel)
        {
            return kernel.session_id;
        }

        static const dictionary & ModelInfo(const Kernel & kernel)
        {
            return kernel.info_;
        }

        static double SessionTime(const Kernel & kernel)
        {
            return kernel.session_timer.GetTime();
        }

        static int CPUCoreCount(const Kernel & kernel)
        {
            return kernel.cpu_cores;
        }
    };

    struct SessionLogDispatcher::State
    {
        State(std::size_t capacity, Transport transport):
            capacity(capacity),
            transport(std::move(transport))
        {}

        const std::size_t capacity;
        Transport transport;
        mutable std::mutex mutex;
        std::condition_variable work_available;
        std::condition_variable idle;
        std::condition_variable worker_finished;
        std::deque<SessionLogEvent> queue;
        bool active = false;
        bool stopping = false;
        bool finished = false;
        std::size_t dropped = 0;
        bool delivery_outage = false;
        std::size_t consecutive_delivery_failures = 0;
        bool queue_overflow_reported = false;
        std::deque<std::string> status_messages;
        std::atomic<bool> status_pending = false;

        void QueueStatusMessage(std::string message)
        {
            status_messages.push_back(std::move(message));
            status_pending.store(true, std::memory_order_release);
        }
    };


    SessionLogDispatcher::SessionLogDispatcher(
        std::size_t capacity,
        Transport transport,
        std::chrono::milliseconds shutdown_wait):
        shutdown_wait_(shutdown_wait)
    {
        if(capacity == 0)
            throw std::invalid_argument("Session log queue capacity must be positive");
        if(!transport)
            throw std::invalid_argument("Session log transport must be set");
        if(shutdown_wait < std::chrono::milliseconds::zero())
            throw std::invalid_argument("Session log shutdown wait must be non-negative");

        state_ = std::make_shared<State>(capacity, std::move(transport));
        worker_ = std::thread([state = state_]()
        {
            while(true)
            {
                SessionLogEvent event;
                {
                    std::unique_lock<std::mutex> lock(state->mutex);
                    state->work_available.wait(lock, [&]()
                    {
                        return state->stopping || !state->queue.empty();
                    });

                    if(state->stopping)
                    {
                        state->queue.clear();
                        break;
                    }

                    if(state->queue.empty())
                        continue;

                    event = std::move(state->queue.front());
                    state->queue.pop_front();
                    state->active = true;
                }

                bool delivered = true;
                std::string failure;
                try
                {
                    state->transport(std::move(event));
                }
                catch(const std::exception & e)
                {
                    delivered = false;
                    failure = e.what();
                }
                catch(...)
                {
                    delivered = false;
                    failure = "unknown transport error";
                }

                {
                    std::lock_guard<std::mutex> lock(state->mutex);
                    try
                    {
                        if(delivered)
                        {
                            if(state->delivery_outage)
                            {
                                const std::size_t failures = state->consecutive_delivery_failures;
                                state->QueueStatusMessage(
                                    "Session logging recovered after " + std::to_string(failures) +
                                    (failures == 1 ? " failed delivery." : " failed deliveries."));
                                state->delivery_outage = false;
                                state->consecutive_delivery_failures = 0;
                            }
                        }
                        else
                        {
                            ++state->consecutive_delivery_failures;
                            if(!state->delivery_outage)
                            {
                                state->delivery_outage = true;
                                state->QueueStatusMessage(
                                    "Session logging failed: " + failure +
                                    ". Further failures will be suppressed until delivery recovers.");
                            }
                        }
                    }
                    catch(...)
                    {
                        // Diagnostic reporting must not stop the delivery worker.
                    }

                    state->active = false;
                    if(state->queue.empty())
                        state->idle.notify_all();
                }
            }

            {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->active = false;
                state->finished = true;
                state->idle.notify_all();
                state->worker_finished.notify_all();
            }
        });
    }


    SessionLogDispatcher::~SessionLogDispatcher()
    {
        if(!state_ || !worker_.joinable())
            return;

        bool finished = false;
        {
            std::unique_lock<std::mutex> lock(state_->mutex);
            state_->stopping = true;
            state_->work_available.notify_all();
            finished = state_->worker_finished.wait_for(lock, shutdown_wait_, [&]()
            {
                return state_->finished;
            });
        }

        if(finished)
            worker_.join();
        else
            worker_.detach();
    }


    bool
    SessionLogDispatcher::Enqueue(SessionLogEvent event)
    {
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            if(state_->stopping || state_->queue.size() >= state_->capacity)
            {
                ++state_->dropped;
                if(!state_->queue_overflow_reported)
                {
                    state_->queue_overflow_reported = true;
                    try
                    {
                        state_->QueueStatusMessage(
                            "Session logging queue is full; additional log events are being dropped.");
                    }
                    catch(...)
                    {
                        // Dropping the diagnostic is preferable to interrupting the kernel.
                    }
                }
                return false;
            }
            state_->queue.push_back(std::move(event));
        }
        state_->work_available.notify_one();
        return true;
    }


    bool
    SessionLogDispatcher::WaitUntilIdle(std::chrono::milliseconds timeout)
    {
        if(timeout < std::chrono::milliseconds::zero())
            throw std::invalid_argument("Session log idle wait must be non-negative");

        std::unique_lock<std::mutex> lock(state_->mutex);
        return state_->idle.wait_for(lock, timeout, [&]()
        {
            return state_->queue.empty() && !state_->active;
        });
    }


    std::size_t
    SessionLogDispatcher::DroppedCount() const
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        return state_->dropped;
    }


    std::vector<std::string>
    SessionLogDispatcher::TakeStatusMessages()
    {
        if(!state_->status_pending.load(std::memory_order_acquire))
            return {};

        std::lock_guard<std::mutex> lock(state_->mutex);
        std::vector<std::string> messages;
        messages.reserve(state_->status_messages.size());
        while(!state_->status_messages.empty())
        {
            messages.push_back(std::move(state_->status_messages.front()));
            state_->status_messages.pop_front();
        }
        state_->status_pending.store(false, std::memory_order_release);
        return messages;
    }


    namespace
    {
        constexpr std::size_t kMaxLogValueLength = 1024;
        constexpr std::size_t kSessionLogQueueCapacity = 32;
        constexpr auto kSessionLogShutdownWait = std::chrono::milliseconds(5500);

        [[nodiscard]] std::string UrlEncode(const std::string & value)
        {
            std::string encoded;
            encoded.reserve(value.size());

            for(unsigned char c : value)
            {
                if((c >= 'A' && c <= 'Z') ||
                   (c >= 'a' && c <= 'z') ||
                   (c >= '0' && c <= '9') ||
                   c == '-' || c == '_' || c == '.' || c == '~')
                {
                    encoded += static_cast<char>(c);
                }
                else
                {
                    encoded += '%';
                    encoded += to_hex(c);
                }
            }

            return encoded;
        }

        [[nodiscard]] std::string LimitLogValue(const std::string & value, size_t max_length = kMaxLogValueLength)
        {
            return valid_utf8_prefix(value, max_length);
        }

        void AppendQueryParameter(std::string & query, const std::string & key, const std::string & value)
        {
            if(value.empty())
                return;

            if(query.find('?') == std::string::npos)
                query += '?';
            else
                query += '&';

            query += UrlEncode(key);
            query += '=';
            query += UrlEncode(LimitLogValue(value));
        }


        void AddCommonParameters(std::string & path, Kernel & kernel, const std::string & event_name, const dictionary & module_info, const std::string & agent)
        {
            AppendQueryParameter(path, "event", event_name);
            AppendQueryParameter(path, "sid", std::to_string(KernelSessionLoggingAccess::SessionID(kernel)));
            AppendQueryParameter(path, "timestamp", std::to_string(GetTimeStamp()));
            const dictionary & model_info = KernelSessionLoggingAccess::ModelInfo(kernel);
            AppendQueryParameter(path, "session_name", model_info.contains("name") ? std::string(model_info["name"]) : "");
            AppendQueryParameter(path, "file", model_info.contains("filename") ? std::string(model_info["filename"]) : kernel.GetOptionFilename());
            AppendQueryParameter(path, "file_path",
                                 std::filesystem::path(kernel.GetOptionFullPath()).filename().string());
            AppendQueryParameter(path, "clock_time", formatNumber(KernelSessionLoggingAccess::SessionTime(kernel), 4));
            AppendQueryParameter(path, "agent", agent);
            AppendQueryParameter(path, "cpu_cores", std::to_string(KernelSessionLoggingAccess::CPUCoreCount(kernel)));
            AppendQueryParameter(path, "classes", module_info.contains("classes") ? std::string(module_info["classes"]) : "");
#if DEBUG
            AppendQueryParameter(path, "debug", "1");
#else
            AppendQueryParameter(path, "debug", "0");
#endif
        }

        CURLcode
        InitializeCurl()
        {
            static std::once_flag initialization;
            static CURLcode result = CURLE_FAILED_INIT;
            std::call_once(initialization, []()
            {
                result = curl_global_init(CURL_GLOBAL_DEFAULT);
            });
            return result;
        }

        std::size_t
        DiscardResponse(char *, std::size_t size, std::size_t count, void *)
        {
            if(size != 0 && count > std::numeric_limits<std::size_t>::max() / size)
                return 0;
            return size * count;
        }

        void
        SendLogRequest(const std::string & path)
        {
            const CURLcode initialization_result = InitializeCurl();
            if(initialization_result != CURLE_OK)
                throw std::runtime_error(
                    "libcurl initialization failed: " +
                    std::string(curl_easy_strerror(initialization_result)));

            CURL * request = curl_easy_init();
            if(request == nullptr)
                throw std::runtime_error("libcurl could not create a request");

            const std::string url = "https://www.ikaros-project.org" + path;
            CURLcode result = CURLE_OK;
            auto set_option = [request, &result](CURLoption option, auto value)
            {
                if(result == CURLE_OK)
                    result = curl_easy_setopt(request, option, value);
            };
            set_option(CURLOPT_URL, url.c_str());
            set_option(CURLOPT_CUSTOMREQUEST, "PUT");
            set_option(CURLOPT_NOSIGNAL, 1L);
            set_option(CURLOPT_CONNECTTIMEOUT_MS, 3000L);
            set_option(CURLOPT_TIMEOUT_MS, 5000L);
            set_option(CURLOPT_SSL_VERIFYPEER, 1L);
            set_option(CURLOPT_SSL_VERIFYHOST, 2L);
            set_option(CURLOPT_FOLLOWLOCATION, 0L);
            set_option(CURLOPT_WRITEFUNCTION, DiscardResponse);
#if LIBCURL_VERSION_NUM >= 0x075500
            set_option(CURLOPT_PROTOCOLS_STR, "https");
#else
            set_option(CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
#endif

            std::string failure;
            if(result != CURLE_OK)
                failure = "request setup failed: " + std::string(curl_easy_strerror(result));
            else
            {
                result = curl_easy_perform(request);
                if(result != CURLE_OK)
                    failure = curl_easy_strerror(result);
                else
                {
                    long response_code = 0;
                    result = curl_easy_getinfo(request, CURLINFO_RESPONSE_CODE, &response_code);
                    if(result != CURLE_OK)
                        failure = "could not read the HTTP response: " +
                                  std::string(curl_easy_strerror(result));
                    else if(response_code < 200 || response_code >= 300)
                        failure = "server returned HTTP " + std::to_string(response_code);
                }
            }

            curl_easy_cleanup(request);
            if(!failure.empty())
                throw std::runtime_error(failure);
        }

        SessionLogDispatcher & LogDispatcher()
        {
            // Register curl's process cleanup before the dispatcher destructor.
            // The dispatcher must stop its worker before curl and OpenSSL shut down.
            static_cast<void>(InitializeCurl());
            static SessionLogDispatcher dispatcher(
                kSessionLogQueueCapacity,
                [](SessionLogEvent event)
                {
                    SendLogRequest(event.path);
                },
                kSessionLogShutdownWait);
            return dispatcher;
        }

        void
        EnqueueLogRequest(std::string path)
        {
            static_cast<void>(LogDispatcher().Enqueue({std::move(path)}));
        }
    }

    void
    QueueSessionLogEvent(Kernel & kernel, const std::string & endpoint, const std::string & event_name)
    {
        try
        {
            dictionary module_info = kernel.GetModuleInstantiationInfo();
            std::string agent = kernel.GetOption("agent");
            if(kernel.info_.contains_non_null("name"))
            {
                auto root = kernel.components.find(std::string(kernel.info_["name"]));
                if(root != kernel.components.end())
                {
                    try
                    {
                        agent = root->second->ComputeValueOf("agent");
                    }
                    catch(...)
                    {
                        // Keep the command-line agent when the model expression cannot be evaluated.
                    }
                }
            }
            std::string path = endpoint;
            AddCommonParameters(path, kernel, event_name, module_info, agent);
            EnqueueLogRequest(std::move(path));
        }
        catch(...)
        {
            // Remote session logging is best-effort and must never interrupt a run.
        }
    }

    void
    QueueProcessStartLogEvent(Kernel & kernel)
    {
        try
        {
            dictionary module_info = kernel.GetModuleInstantiationInfo();
            std::string path = "/process_start3/";
            AddCommonParameters(path, kernel, "process_start", module_info, kernel.GetOption("agent"));
            AppendQueryParameter(path, "uptime", formatNumber(kernel.uptime_timer.GetTime(), 4));
            EnqueueLogRequest(std::move(path));
        }
        catch(...)
        {
            // Process start logging is best-effort and must never interrupt startup.
        }
    }

    void
    QueueProcessExitLogEvent(Kernel & kernel)
    {
        try
        {
            dictionary module_info = kernel.GetModuleInstantiationInfo();
            std::string agent = kernel.GetOption("agent");
            if(kernel.info_.contains_non_null("name"))
            {
                auto root = kernel.components.find(std::string(kernel.info_["name"]));
                if(root != kernel.components.end())
                {
                    try
                    {
                        agent = root->second->ComputeValueOf("agent");
                    }
                    catch(...)
                    {
                        // Keep the command-line agent when the model expression cannot be evaluated.
                    }
                }
            }
            std::string path = "/exit3/";
            AddCommonParameters(path, kernel, "exit", module_info, agent);
            AppendQueryParameter(path, "uptime", formatNumber(kernel.uptime_timer.GetTime(), 4));
            EnqueueLogRequest(std::move(path));
        }
        catch(...)
        {
            // Process exit logging is best-effort and must never interrupt shutdown.
        }
    }


    void
    ReportSessionLogStatus(Kernel & kernel)
    {
        for(auto & message : LogDispatcher().TakeStatusMessages())
            kernel.Warning(std::move(message));
    }


    void
    Kernel::LogStart()
    {
#if defined(LOGGING_OFF)
        return;
#else
        LogSessionEvent("/start3/", "start");
#endif
    }


    void
    Kernel::LogStop()
    {
#if defined(LOGGING_FULL)
        LogSessionEvent("/stop3/", "stop");
#else
        return;
#endif
    }


    void
    Kernel::LogProcessStart()
    {
#if !defined(LOGGING_FULL)
        return;
#else
        if(process_start_logged)
            return;

        process_start_logged = true;
        QueueProcessStartLogEvent(*this);
#endif
    }


    void
    Kernel::LogProcessExit()
    {
#if !defined(LOGGING_FULL)
        return;
#else
        if(process_exit_logged)
            return;

        process_exit_logged = true;
        QueueProcessExitLogEvent(*this);
#endif
    }


    void
    Kernel::LogSessionEvent(const std::string & endpoint, const std::string & event_name)
    {
        QueueSessionLogEvent(*this, endpoint, event_name);
    }
}
