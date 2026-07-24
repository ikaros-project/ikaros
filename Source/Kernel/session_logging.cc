#include <condition_variable>
#include <cstddef>
#include <deque>
#include <filesystem>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <utility>

#include <curl/curl.h>

#include "session_logging.h"
#include "ikaros.h"

namespace ikaros
{
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

                    if(state->queue.empty())
                    {
                        if(state->stopping)
                            break;
                        continue;
                    }

                    event = std::move(state->queue.front());
                    state->queue.pop_front();
                    state->active = true;
                }

                try
                {
                    state->transport(std::move(event));
                }
                catch(...)
                {
                    // Transport failures must not stop later queued events.
                }

                {
                    std::lock_guard<std::mutex> lock(state->mutex);
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


    namespace
    {
        constexpr std::size_t kMaxLogValueLength = 1024;
        constexpr std::size_t kSessionLogQueueCapacity = 32;

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
            if(value.size() <= max_length)
                return value;
            return value.substr(0, max_length);
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
            AppendQueryParameter(path, "sid", std::to_string(kernel.session_id));
            AppendQueryParameter(path, "timestamp", std::to_string(GetTimeStamp()));
            AppendQueryParameter(path, "session_name", kernel.info_.contains("name") ? std::string(kernel.info_["name"]) : "");
            AppendQueryParameter(path, "file", kernel.info_.contains("filename") ? std::string(kernel.info_["filename"]) : kernel.GetOptionFilename());
            AppendQueryParameter(path, "file_path",
                                 std::filesystem::path(kernel.GetOptionFullPath()).filename().string());
            AppendQueryParameter(path, "clock_time", formatNumber(kernel.session_timer.GetTime(), 4));
            AppendQueryParameter(path, "agent", agent);
            AppendQueryParameter(path, "cpu_cores", std::to_string(kernel.cpu_cores));
            AppendQueryParameter(path, "classes", module_info.contains("classes") ? std::string(module_info["classes"]) : "");
#if DEBUG
            AppendQueryParameter(path, "debug", "1");
#else
            AppendQueryParameter(path, "debug", "0");
#endif
        }

        bool
        InitializeCurl()
        {
            static std::once_flag initialization;
            static CURLcode result = CURLE_FAILED_INIT;
            std::call_once(initialization, []()
            {
                result = curl_global_init(CURL_GLOBAL_DEFAULT);
            });
            return result == CURLE_OK;
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
            if(!InitializeCurl())
                return;

            CURL * request = curl_easy_init();
            if(request == nullptr)
                return;

            const std::string url = "https://www.ikaros-project.org" + path;
            bool configured =
                curl_easy_setopt(request, CURLOPT_URL, url.c_str()) == CURLE_OK &&
                curl_easy_setopt(request, CURLOPT_HTTPGET, 1L) == CURLE_OK &&
                curl_easy_setopt(request, CURLOPT_NOSIGNAL, 1L) == CURLE_OK &&
                curl_easy_setopt(request, CURLOPT_CONNECTTIMEOUT_MS, 3000L) == CURLE_OK &&
                curl_easy_setopt(request, CURLOPT_TIMEOUT_MS, 5000L) == CURLE_OK &&
                curl_easy_setopt(request, CURLOPT_SSL_VERIFYPEER, 1L) == CURLE_OK &&
                curl_easy_setopt(request, CURLOPT_SSL_VERIFYHOST, 2L) == CURLE_OK &&
                curl_easy_setopt(request, CURLOPT_FOLLOWLOCATION, 0L) == CURLE_OK &&
                curl_easy_setopt(request, CURLOPT_WRITEFUNCTION, DiscardResponse) == CURLE_OK;
#if LIBCURL_VERSION_NUM >= 0x075500
            configured = configured &&
                curl_easy_setopt(request, CURLOPT_PROTOCOLS_STR, "https") == CURLE_OK;
#else
            configured = configured &&
                curl_easy_setopt(request, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS) == CURLE_OK;
#endif
            if(configured)
                static_cast<void>(curl_easy_perform(request));
            curl_easy_cleanup(request);
        }

        SessionLogDispatcher & LogDispatcher()
        {
            static SessionLogDispatcher dispatcher(
                kSessionLogQueueCapacity,
                [](SessionLogEvent event)
                {
                    SendLogRequest(event.path);
                });
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
                    agent = root->second->ComputeValueOf("agent");
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
                    agent = root->second->ComputeValueOf("agent");
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
}
