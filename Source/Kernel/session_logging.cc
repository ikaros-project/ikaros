#include "session_logging.h"
#include "ikaros.h"

namespace ikaros
{
    namespace
    {
        constexpr size_t kMaxLogValueLength = 1024;

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

        void AddCommonParameters(std::string & path, Kernel & kernel, const std::string & event_name)
        {
            dictionary module_info = kernel.GetModuleInstantiationInfo();

            char hostname[256] = {0};
            if(gethostname(hostname, sizeof(hostname)-1) != 0)
                hostname[0] = '\0';

            AppendQueryParameter(path, "event", event_name);
            AppendQueryParameter(path, "sid", std::to_string(kernel.session_id));
            AppendQueryParameter(path, "timestamp", std::to_string(GetTimeStamp()));
            AppendQueryParameter(path, "session_name", kernel.info_.contains("name") ? std::string(kernel.info_["name"]) : "");
            AppendQueryParameter(path, "file", kernel.info_.contains("filename") ? std::string(kernel.info_["filename"]) : kernel.options_.filename());
            AppendQueryParameter(path, "file_path", kernel.options_.full_path());
            AppendQueryParameter(path, "clock_time", formatNumber(kernel.session_timer.GetTime(), 4));
            AppendQueryParameter(path, "host", hostname);
            AppendQueryParameter(path, "cpu_cores", std::to_string(kernel.cpu_cores));
            AppendQueryParameter(path, "classes", module_info.contains("classes") ? std::string(module_info["classes"]) : "");
#if DEBUG
            AppendQueryParameter(path, "debug", "1");
#else
            AppendQueryParameter(path, "debug", "0");
#endif
        }

        void SendLogRequest(const std::string & path)
        {
            std::string request = "GET " + path + " HTTP/1.1\r\nHost: www.ikaros-project.org\r\nConnection: close\r\n\r\n";

            Socket socket;
            char response[2048] = {0};
            socket.Get("www.ikaros-project.org", 80, request.c_str(), response, sizeof(response)-1);
            socket.Close();
        }
    }

    void
    SendSessionLogEvent(Kernel & kernel, const std::string & endpoint, const std::string & event_name)
    {
        try
        {
            std::string path = endpoint;
            AddCommonParameters(path, kernel, event_name);
            SendLogRequest(path);
        }
        catch(...)
        {
            // Remote session logging is best-effort and must never interrupt a run.
        }
    }

    void
    SendProcessStartLogEvent(Kernel & kernel)
    {
        try
        {
            std::string path = "/process_start3/";
            AddCommonParameters(path, kernel, "process_start");
            AppendQueryParameter(path, "uptime", formatNumber(kernel.uptime_timer.GetTime(), 4));
            SendLogRequest(path);
        }
        catch(...)
        {
            // Process start logging is best-effort and must never interrupt startup.
        }
    }

    void
    SendProcessExitLogEvent(Kernel & kernel)
    {
        try
        {
            std::string path = "/exit3/";
            AddCommonParameters(path, kernel, "exit");
            AppendQueryParameter(path, "uptime", formatNumber(kernel.uptime_timer.GetTime(), 4));
            SendLogRequest(path);
        }
        catch(...)
        {
            // Process exit logging is best-effort and must never interrupt shutdown.
        }
    }
}
