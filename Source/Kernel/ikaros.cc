// Ikaros 3.0

#include "ikaros.h"
#include "compute_engine.h"
#include "session_logging.h"

#include <charconv>
#include <cctype>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <random>
#include <sstream>
#include <system_error>
#include <sys/resource.h>

#if __has_include(<CommonCrypto/CommonDigest.h>) && __has_include(<CommonCrypto/CommonHMAC.h>)
#include <CommonCrypto/CommonDigest.h>
#include <CommonCrypto/CommonHMAC.h>
#define IKAROS_HAVE_COMMONCRYPTO 1
#endif

#if IKAROS_HAVE_OPENSSL
#include <openssl/evp.h>
#include <openssl/hmac.h>
#endif

using namespace ikaros;
using namespace std::chrono;
using namespace std::literals;

std::atomic<bool> global_terminate(false);

namespace ikaros
{
    parameter::parameter():
        state_(std::make_shared<parameter_state>())
    {
    }

    bool
    parameter::compare_string(const std::string & value) const
    {
        return as_string() == value;
    }

    Message::Message(int level, std::string message, std::string path):
        level_(level),
        message_(message),
        path_(path)
    {
    }

    std::string
    Message::json() const
    {
        return "[\""+std::to_string(level_)+"\",\""+escape_json_string(message_)+"\",\""+escape_json_string(path_)+"\"]";
    }

    namespace
    {
        constexpr size_t default_max_retained_webui_log_messages = 500;
        constexpr int maximum_connection_delay = 100;

        std::filesystem::path
        temporary_save_path(const std::filesystem::path & target_path)
        {
            static std::atomic<unsigned long long> serial{0};
            const auto timestamp =
                std::chrono::steady_clock::now().time_since_epoch().count();

            for(int attempt = 0; attempt < 100; attempt++)
            {
                const auto id = serial.fetch_add(1, std::memory_order_relaxed);
                const std::string temporary_name =
                    "." + target_path.filename().string() + ".ikaros-save-" +
                    std::to_string(timestamp) + "-" + std::to_string(id) +
                    ".tmp";
                const std::filesystem::path temporary_path =
                    target_path.parent_path() / temporary_name;
                std::error_code error;
                const bool exists = std::filesystem::exists(temporary_path, error);
                if(error)
                    throw std::system_error(
                        error, "Could not inspect temporary save file \"" +
                                   temporary_path.string() + "\"");
                if(!exists)
                    return temporary_path;
            }

            throw std::runtime_error(
                "Could not create a unique temporary save filename for \"" +
                target_path.string() + "\"");
        }


        void
        replace_file_atomically(const std::filesystem::path & source,
                                const std::filesystem::path & target)
        {
            std::error_code error;
            std::filesystem::rename(source, target, error);
            if(error)
                throw std::system_error(
                    error, "Could not atomically replace model \"" +
                               target.string() + "\"");
        }


        struct AsyncRuntimeSnapshot
        {
            tick_count tick;
            double tick_duration;
            double time;
            double real_time;
            double nominal_time;
            double run_time;
            double time_of_day;
            double lag;
            double uptime;
            double actual_tick_duration;
            double tick_time_usage;
            double cpu_usage;
            double idle_time;
        };

        thread_local const AsyncRuntimeSnapshot * active_async_runtime_snapshot = nullptr;

        class AsyncRuntimeSnapshotScope
        {
        public:
            explicit AsyncRuntimeSnapshotScope(const AsyncRuntimeSnapshot & snapshot):
                previous_(active_async_runtime_snapshot)
            {
                active_async_runtime_snapshot = &snapshot;
            }

            ~AsyncRuntimeSnapshotScope()
            {
                active_async_runtime_snapshot = previous_;
            }

            AsyncRuntimeSnapshotScope(const AsyncRuntimeSnapshotScope &) = delete;
            AsyncRuntimeSnapshotScope & operator=(const AsyncRuntimeSnapshotScope &) = delete;

        private:
            const AsyncRuntimeSnapshot * previous_;
        };

        AsyncRuntimeSnapshot
        CaptureAsyncRuntimeSnapshot(Kernel & k)
        {
            AsyncRuntimeSnapshot snapshot;
            snapshot.tick = k.GetTick();
            snapshot.tick_duration = k.GetTickDuration();
            snapshot.nominal_time = static_cast<double>(snapshot.tick) * snapshot.tick_duration;
            snapshot.real_time = k.GetRealTime();
            snapshot.time = k.GetRunMode() == run_mode_realtime ? snapshot.real_time : snapshot.nominal_time;
            snapshot.run_time = k.GetRunTime();
            snapshot.time_of_day = k.GetTimeOfDay();
            snapshot.lag = k.GetRunMode() == run_mode_realtime ? snapshot.nominal_time - snapshot.real_time : 0;
            snapshot.uptime = k.GetUptime();
            snapshot.actual_tick_duration = k.GetActualTickDuration();
            snapshot.tick_time_usage = k.GetTickTimeUsage();
            snapshot.cpu_usage = k.GetCPUUsage();
            snapshot.idle_time = k.GetIdleTime();
            return snapshot;
        }

        bool is_internal(const dictionary & info)
        {
            return info.is_set("internal");
        }

        bool try_parse_matrix_literal(matrix & out, const std::string & value)
        {
            try
            {
                out = matrix(value);
                return true;
            }
            catch(const std::invalid_argument &)
            {
                return false;
            }
            catch(const std::out_of_range &)
            {
                return false;
            }
        }

        std::optional<double> get_parameter_bound(const dictionary & info, const std::string & name)
        {
            if(!info.contains_non_null(name))
                return std::nullopt;

            double value = 0;
            if(!parse_double(std::string(info[name]), value))
                return std::nullopt;
            if(!std::isfinite(value))
                throw exception("Parameter " + name + " constraint must be finite.");
            return value;
        }

        void
        ValidateConnectionDelayRange(const range & delays,
                                     const std::string & source,
                                     const std::string & target,
                                     const std::string & path)
        {
            const std::string connection = "Connection \"" + source + " => " + target + "\" delay range ";

            if(delays.rank() != 1)
                throw build_failed(connection + "must be one-dimensional.", path);
            const int delay_start = delays.start(0);
            const int delay_stop = delays.stop(0);
            const int delay_step = delays.step(0);
            if(delay_start == delay_stop)
                throw build_failed(connection + "must not be empty.", path);
            if(delay_step <= 0)
                throw build_failed(connection + "must have a positive increment.", path);
            if(delay_start < 0)
                throw build_failed(connection + "must be non-negative.", path);
            if(delay_stop <= delay_start)
                throw build_failed(connection + "must be an ascending, non-empty range.", path);

            const long long distance = static_cast<long long>(delay_stop) - delay_start;
            const long long count = 1 + (distance - 1) / delay_step;
            const long long max_delay = static_cast<long long>(delay_start) +
                                        (count - 1) * delay_step;
            if(max_delay > maximum_connection_delay)
                throw build_failed(connection + "must not exceed " +
                                   std::to_string(maximum_connection_delay) + " ticks.", path);
        }

        int clamp_option_index(int index, const std::vector<std::string> & options)
        {
            if(options.empty())
                return 0;
            if(index < 0)
                return 0;
            if(index >= int(options.size()))
                return int(options.size()) - 1;
            return index;
        }

        bool constant_time_equals(const std::string & a, const std::string & b)
        {
            const size_t max_length = std::max(a.size(), b.size());
            unsigned char diff = static_cast<unsigned char>(a.size() ^ b.size());
            for(size_t i = 0; i < max_length; ++i)
            {
                const unsigned char ac = i < a.size() ? static_cast<unsigned char>(a[i]) : 0;
                const unsigned char bc = i < b.size() ? static_cast<unsigned char>(b[i]) : 0;
                diff |= static_cast<unsigned char>(ac ^ bc);
            }
            return diff == 0;
        }

        std::string extract_cookie_value(const std::string & cookie_header, const std::string & cookie_name)
        {
            for(const auto & cookie : split(cookie_header, ";"))
            {
                std::string trimmed_cookie = trim(cookie);
                auto separator = trimmed_cookie.find('=');
                if(separator == std::string::npos)
                    continue;

                std::string key = trim(trimmed_cookie.substr(0, separator));
                if(key != cookie_name)
                    continue;

                return trim(trimmed_cookie.substr(separator + 1));
            }
            return "";
        }

        std::string hex_encode(const unsigned char * data, size_t size)
        {
            static constexpr char hex[] = "0123456789abcdef";
            std::string encoded;
            encoded.reserve(size * 2);
            for(size_t i = 0; i < size; ++i)
            {
                encoded.push_back(hex[(data[i] >> 4) & 0x0F]);
                encoded.push_back(hex[data[i] & 0x0F]);
            }
            return encoded;
        }

        std::string random_hex_string(size_t byte_count)
        {
            std::random_device rd;
            std::uniform_int_distribution<int> dist(0, 255);
            std::vector<unsigned char> bytes(byte_count);
            for(unsigned char & byte : bytes)
                byte = static_cast<unsigned char>(dist(rd));
            return hex_encode(bytes.data(), bytes.size());
        }

        long long unix_time_seconds()
        {
            return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
        }

#if IKAROS_HAVE_COMMONCRYPTO
        std::string hmac_sha256_hex(const std::string & key, const std::string & message)
        {
            unsigned char digest[CC_SHA256_DIGEST_LENGTH];
            CCHmac(kCCHmacAlgSHA256,
                   key.data(),
                   key.size(),
                   message.data(),
                   message.size(),
                   digest);
            return hex_encode(digest, sizeof(digest));
        }
#elif IKAROS_HAVE_OPENSSL
        std::string hmac_sha256_hex(const std::string & key, const std::string & message)
        {
            unsigned char digest[EVP_MAX_MD_SIZE];
            unsigned int digest_size = 0;
            if(HMAC(EVP_sha256(),
                    key.data(),
                    static_cast<int>(key.size()),
                    reinterpret_cast<const unsigned char *>(message.data()),
                    message.size(),
                    digest,
                    &digest_size) == nullptr)
                throw exception("Could not calculate authentication HMAC.");
            return hex_encode(digest, digest_size);
        }
#endif

        matrix scalar_parameter_matrix(double value)
        {
            matrix m(1);
            m(0) = static_cast<float>(value);
            return m;
        }

        double get_scalar_matrix_value(const matrix & value, const std::string & conversion_name)
        {
            if(value.size() != 1)
                throw exception("Could not convert matrix to " + conversion_name + ". Matrix must contain exactly one element.");

            std::vector<int> zero_index(value.rank(), 0);
            return value.at(zero_index);
        }

        std::string format_shape(const std::vector<int> & shape)
        {
            std::string result = "{";
            std::string separator;
            for(int dimension : shape)
            {
                result += separator + std::to_string(dimension);
                separator = ", ";
            }
            result += "}";
            return result;
        }

        bool is_scalar_state_type(const std::string & type)
        {
            return type == "float" || type == "double" || type == "int" || type == "bool" || type == "string";
        }


        tick_count
        parse_stop_after(const std::string & value)
        {
            const std::string text = trim(value);
            tick_count result = 0;
            const char * begin = text.data();
            const char * end = begin + text.size();
            bool valid_sign = true;
            if(begin != end && *begin == '+')
            {
                ++begin;
                valid_sign = begin != end && *begin != '+' && *begin != '-';
            }
            const auto conversion = std::from_chars(begin, end, result);
            if(text.empty() || !valid_sign || conversion.ec != std::errc() ||
               conversion.ptr != end || result < -1)
                throw setup_failed("Invalid stop tick \"" + value +
                                   "\". Expected -1 or a non-negative integer.");
            return result;
        }


        double
        parse_tick_duration(const std::string & value)
        {
            double result = 0;
            if(!parse_double(value, result) || !std::isfinite(result) || result <= 0)
                throw setup_failed("Invalid tick duration \"" + value +
                                   "\". Expected a finite positive number of seconds.");
            return result;
        }


        std::string resolve_state_filename(const options & opts, const std::string & option_name)
        {
            std::string filename = opts.get(option_name);
            if(!filename.empty() && filename != "true")
                return filename;

            std::filesystem::path model_path = opts.full_path();
            if(model_path.empty())
                throw exception("Can not derive state filename because no model file is loaded.");

            model_path.replace_extension(".state");
            return model_path.string();
        }

        std::string resolve_state_filename_from_request(const Request & request, const std::string & user_dir, const options & opts, const std::string & option_name)
        {
            std::string requested_filename;
            if(request.parameters.contains("filename"))
                requested_filename = std::string(request.parameters["filename"]);
            else if(request.parameters.contains("file"))
                requested_filename = std::string(request.parameters["file"]);

            requested_filename = trim(requested_filename);
            if(requested_filename.empty())
                return resolve_state_filename(opts, option_name);

            std::filesystem::path path = add_extension(requested_filename, ".state");
            std::filesystem::path filename = path.filename();
            if(filename.empty() || filename == "." || filename == ".." || filename.stem().empty())
                throw exception("State filename is invalid.");

            return (std::filesystem::path(user_dir) / filename).string();
        }

        std::string component_path_from_request(const Request & request)
        {
            std::string component_path;
            if(request.parameters.contains("module"))
                component_path = std::string(request.parameters["module"]);
            else if(request.parameters.contains("path"))
                component_path = std::string(request.parameters["path"]);
            else if(request.parameters.contains("component"))
                component_path = std::string(request.parameters["component"]);

            component_path = trim(component_path);
            if(!component_path.empty() && component_path[0] == '.')
                component_path = component_path.substr(1);
            return component_path;
        }

        bool path_is_in_scope(const std::string & path, const std::string & scope)
        {
            if(scope.empty())
                return true;
            return path == scope || (path.size() > scope.size() && path.compare(0, scope.size(), scope) == 0 && path[scope.size()] == '.');
        }

        std::string remap_scoped_state_path(const std::string & saved_path, const std::string & saved_scope, const std::string & target_scope)
        {
            if(target_scope.empty())
                return saved_path;

            if(!saved_scope.empty() && saved_scope != "network")
            {
                if(!path_is_in_scope(saved_path, saved_scope))
                    return "";
                return target_scope + saved_path.substr(saved_scope.size());
            }

            return path_is_in_scope(saved_path, target_scope) ? saved_path : "";
        }

        std::string current_utc_timestamp()
        {
            auto now = system_clock::now();
            std::time_t now_time = system_clock::to_time_t(now);
            std::tm utc {};
            gmtime_r(&now_time, &utc);
            auto padded = [](int value, int width)
            {
                std::string result = std::to_string(value);
                if(result.size() < static_cast<std::size_t>(width))
                    result.insert(0, static_cast<std::size_t>(width) - result.size(), '0');
                return result;
            };
            return padded(utc.tm_year + 1900, 4) + "-" + padded(utc.tm_mon + 1, 2) + "-" +
                   padded(utc.tm_mday, 2) + "T" + padded(utc.tm_hour, 2) + ":" +
                   padded(utc.tm_min, 2) + ":" + padded(utc.tm_sec, 2) + "Z";
        }

        constexpr const char * ikaros_version = "3.0";

        double parse_parameter_number(const std::string & value, const std::string & conversion_name)
        {
            try
            {
                return parse_double(value);
            }
            catch(const std::invalid_argument &)
            {
                throw exception("Could not convert string \"" + value + "\" to " + conversion_name + ".");
            }
            catch(const std::out_of_range &)
            {
                throw exception("String \"" + value + "\" is out of range for " + conversion_name + ".");
            }
        }

        int parse_scalar_state_int(const std::string & value)
        {
            const std::string trimmed_value = trim(value);
            if(trimmed_value.empty())
                throw std::invalid_argument("Expected an integer.");

            int parsed_value = 0;
            const char * begin = trimmed_value.data();
            const char * end = begin + trimmed_value.size();
            const auto result = std::from_chars(begin, end, parsed_value);
            if(result.ec == std::errc::result_out_of_range)
                throw std::out_of_range("Integer is outside the supported range.");
            if(result.ec != std::errc() || result.ptr != end)
                throw std::invalid_argument("Expected an integer.");
            return parsed_value;
        }

        std::string canonicalize_shape_aliases(const std::string & xml)
        {
            std::string out;
            out.reserve(xml.size());

            for(size_t i = 0; i < xml.size();)
            {
                if(i + 5 <= xml.size() && xml.compare(i, 5, ".size") == 0)
                {
                    const char next = (i + 5 < xml.size()) ? xml[i + 5] : '\0';
                    const bool is_alias = next == '[' || next == '\0'
                        || (!ascii_is_alnum(static_cast<unsigned char>(next)) && next != '_');
                    if(is_alias)
                    {
                        out += ".shape";
                        i += 5;
                        continue;
                    }
                }

                out.push_back(xml[i]);
                ++i;
            }

            return out;
        }

        dictionary make_log_level_parameter()
        {
            dictionary log_param;
            log_param["_tag"] = "parameter";
            log_param["name"] = "log_level";
            log_param["type"] = "number";
            log_param["control"] = "menu";
            log_param["options"] = "inherit,quiet,exception,end_of_file,terminate,fatal_error,warning,print,debug,trace";
            log_param["default"] = 0;
            return log_param;
        }

        dictionary make_module_start_parameter()
        {
            dictionary module_start_param;
            module_start_param["_tag"] = "parameter";
            module_start_param["name"] = "module_start";
            module_start_param["type"] = "number";
            module_start_param["control"] = "menu";
            module_start_param["options"] = "at_tick,first_data,all_data";
            module_start_param["default"] = 0;
            return module_start_param;
        }

        dictionary make_start_tick_parameter()
        {
            dictionary start_tick_param;
            start_tick_param["_tag"] = "parameter";
            start_tick_param["name"] = "start_tick";
            start_tick_param["type"] = "number";
            start_tick_param["default"] = 0;
            return start_tick_param;
        }

        dictionary make_async_parameter()
        {
            dictionary async_param;
            async_param["_tag"] = "parameter";
            async_param["name"] = "async";
            async_param["type"] = "bool";
            async_param["default"] = "no";
            async_param["description"] = "Run this module asynchronously.";
            return async_param;
        }

        std::string normalize_request_value_path(const std::string & path)
        {
            if(!path.empty() && path[0] == '.')
                return path.substr(1);
            return path;
        }

        bool request_path_matches_command(const std::string & uri, const std::string & command)
        {
            std::string path = "/" + command;
            return uri == path || uri.rfind(path + "?", 0) == 0 || uri.rfind(path + "/", 0) == 0;
        }

        dictionary make_color_parameter()
        {
            dictionary color_param;
            color_param["_tag"] = "parameter";
            color_param["name"] = "color";
            color_param["type"] = "string";
            color_param["default"] = "black";
            color_param["description"] = "Selected ui color";
            color_param["control"] = "ui_color";
            return color_param;
        }

        dictionary make_ui_snapshot_rgb_quality_parameter()
        {
            dictionary quality_param;
            quality_param["_tag"] = "parameter";
            quality_param["name"] = "rgb_quality";
            quality_param["type"] = "number";
            quality_param["default"] = 75;
            quality_param["description"] = "JPEG quality used for RGB images in WebUI update snapshots.";
            return quality_param;
        }

        dictionary make_ui_snapshot_gray_quality_parameter()
        {
            dictionary quality_param;
            quality_param["_tag"] = "parameter";
            quality_param["name"] = "gray_quality";
            quality_param["type"] = "number";
            quality_param["default"] = 70;
            quality_param["description"] = "JPEG quality used for grayscale and pseudocolor images in WebUI update snapshots.";
            return quality_param;
        }

        dictionary make_snapshot_interval_parameter()
        {
            dictionary interval_param;
            interval_param["_tag"] = "parameter";
            interval_param["name"] = "snapshot_interval";
            interval_param["type"] = "number";
            interval_param["default"] = 0.1;
            interval_param["description"] = "Minimum interval in seconds between image refreshes in WebUI update snapshots.";
            return interval_param;
        }

        dictionary make_webui_request_interval_parameter()
        {
            dictionary interval_param;
            interval_param["_tag"] = "parameter";
            interval_param["name"] = "webui_req_int";
            interval_param["type"] = "number";
            interval_param["default"] = 0.1;
            interval_param["description"] = "WebUI update request and snapshot construction interval in seconds.";
            return interval_param;
        }

        dictionary make_webui_log_buffer_limit_parameter()
        {
            dictionary limit_param;
            limit_param["_tag"] = "parameter";
            limit_param["name"] = "webui_log_buffer_limit";
            limit_param["type"] = "number";
            limit_param["default"] = static_cast<int>(default_max_retained_webui_log_messages);
            limit_param["description"] = "Maximum number of recent log messages retained for delivery to WebUI clients.";
            return limit_param;
        }

        void ensure_list(dictionary & info, const std::string & key)
        {
            if(!info.contains_non_null(key) || !info[key].is_list())
                info[key] = list();
        }

        void ensure_component_collections(dictionary & info)
        {
            ensure_list(info, "inputs");
            ensure_list(info, "outputs");
            ensure_list(info, "states");
            ensure_list(info, "parameters");
            ensure_list(info, "groups");
            ensure_list(info, "modules");
        }

        int default_thread_pool_size(unsigned int cpu_cores)
        {
            return cpu_cores > 1 ? static_cast<int>(cpu_cores) - 1 : 1;
        }

        constexpr char ui_subscription_separator = '\n';
        constexpr double ui_subscription_timeout_seconds = 10.0;
        constexpr double profiling_subscription_timeout_seconds = 3.0;
        constexpr int ui_snapshot_rgb_jpeg_quality = 75;
        constexpr int ui_snapshot_gray_jpeg_quality = 70;

        bool is_snapshot_image_format(const std::string & format)
        {
            return format=="rgb" || format=="gray" || format=="red" || format=="green" || format=="blue" || format=="spectrum" || format=="fire";
        }

        int snapshot_jpeg_quality_for_format(const std::string & format)
        {
            return format == "rgb" ? ui_snapshot_rgb_jpeg_quality : ui_snapshot_gray_jpeg_quality;
        }
    }

    std::string  validate_identifier(std::string s)
    {
        static std::string legal = "_0123456789aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ";
        if(s.empty())
            throw exception("Identifier cannot be empty string");
        if('0' <= s[0] && s[0] <= '9')
            throw exception("Identifier cannot start with a number: "+s);
        for(auto c : s)
            if(legal.find(c) == std::string::npos)
                throw exception("Illegal character in identifier: "+s);
        return s;
    }

    long new_session_id()
    {
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }


    parameter::parameter(dictionary info):
        state_(std::make_shared<parameter_state>())
    {
        state_->info = std::move(info);
        state_->has_options = state_->info.contains("options");
        if(state_->has_options)
            state_->options = split(std::string(state_->info["options"]), ",");
        state_->dynamic = state_->info.is_set("dynamic");
        state_->minimum = get_parameter_bound(state_->info, "min");
        state_->maximum = get_parameter_bound(state_->info, "max");

        std::string type_string = state_->info["type"];

        if(type_string=="float" || type_string=="int" || type_string=="double")  // Temporary
            type_string = "number";

        auto type_index = std::find(parameter_strings.begin(), parameter_strings.end(), type_string);
        if(type_index == parameter_strings.end())
            throw exception("Unknown parameter type: "+type_string+".");

        state_->type = parameter_type(std::distance(parameter_strings.begin(), type_index));

        if(state_->minimum && state_->maximum && *state_->minimum > *state_->maximum)
            throw exception("Parameter minimum must not exceed maximum.");

        if(state_->has_options)
        {
            state_->value = 0;
            return;
        }

        switch(state_->type)
        {
            case number_type:
            case rate_type:
                state_->value = 0.0;
                break;

            case bool_type:
                state_->value = false;
                break;

            case string_type: 
                state_->value = std::string("");
                break;

            case matrix_type: 
                state_->value = matrix();
                break;

            default: 
                break;
        } 
    }



    parameter::parameter(const std::string type, const std::string options):
        parameter(options.empty() ? dictionary({{"type", type}}) : dictionary({{"type", type},{"options", options}}))
    {}


    parameter::parameter(const parameter & p):
        state_(p.clone_state())
    {
    }


    parameter &
    parameter::operator=(const parameter & p)
    {
        if(this != &p)
            state_ = p.clone_state();
        return *this;
    }


    std::shared_ptr<parameter::parameter_state>
    parameter::clone_state() const
    {
        auto cloned_state = std::make_shared<parameter_state>();
        if(!state_)
            return cloned_state;

        cloned_state->info = state_->info.copy();
        cloned_state->has_options = state_->has_options;
        cloned_state->options = state_->options;
        cloned_state->resolved = state_->resolved;
        cloned_state->type = state_->type;
        cloned_state->dynamic = state_->dynamic;
        cloned_state->minimum = state_->minimum;
        cloned_state->maximum = state_->maximum;

        if(const matrix * stored_matrix = matrix_value())
        {
            matrix copied_matrix;
            copied_matrix.copy(*stored_matrix);
            cloned_state->value = copied_matrix;
        }
        else
            cloned_state->value = state_->value;

        return cloned_state;
    }


    void
    parameter::bind_to(const parameter & p)
    {
        state_ = p.state_;
    }



    double 
    parameter::operator=(double v)
    {
        if(state_->has_options)
        {
            state_->value = clamp_option_index(checked_truncating_int(std::round(v), "option index"),
                                               state_->options);
            state_->resolved = true;
            return v;
        }

        switch(state_->type)
        {
            case number_type:
            case rate_type:
                validate_numeric_value(v);
                state_->value = double(v);
                break;
            case bool_type:
                state_->value = (v != 0.0);
                break;
            case string_type:
                state_->value = formatNumber(v);
                break;
            case matrix_type:
                set_matrix(scalar_parameter_matrix(v));
                break;
            default:
                throw exception("Invalid parameter type for numeric assignment.");
        }
        state_->resolved = true;
        return v;
    }

    std::string 
    parameter::operator=(std::string v)
    {
        double val = 0;
        bool has_numeric_value = false;
        if(state_->has_options)
        {
            auto it = std::find(state_->options.begin(), state_->options.end(), v);
            if(it != state_->options.end())
                state_->value = int(std::distance(state_->options.begin(), it));
            else if(is_number(v))
                state_->value = clamp_option_index(
                    checked_truncating_int(std::round(parse_parameter_number(v, "option index")), "option index"),
                    state_->options
                );
            else
                throw exception("Invalid parameter value");

            state_->resolved = true;
            return v;
        }
        else if(is_number(v))
        {
            val = parse_parameter_number(v, "number");
            has_numeric_value = true;
        }

        switch(state_->type)
        {
            case number_type:
            case rate_type:
                if(!has_numeric_value)
                    throw exception("Invalid numeric parameter value \"" + v + "\".");
                validate_numeric_value(val);
                state_->value = val;
                break;

            case bool_type:
            {
                bool bool_value = false;
                if(!parse_bool(v, bool_value))
                    throw exception("Invalid boolean parameter value \"" + v + "\".");
                state_->value = bool_value;
                break;
            }

            case string_type:
                state_->value = v;
                break;

            case matrix_type:
                set_matrix(matrix(v));
                break;

            default:
                throw exception("Invalid parameter type for string assignment.");
        }
        state_->resolved = true;
        return v;
    }


    void
    parameter::validate_numeric_value(double numeric_value) const
    {
        if(!state_->minimum && !state_->maximum)
            return;
        if(!std::isfinite(numeric_value))
            throw exception("Numeric parameter value must be finite when constraints are declared.");
        if(state_->minimum && numeric_value < *state_->minimum)
            throw exception("Numeric parameter value " + formatNumber(numeric_value) +
                            " is below minimum " + formatNumber(*state_->minimum) + ".");
        if(state_->maximum && numeric_value > *state_->maximum)
            throw exception("Numeric parameter value " + formatNumber(numeric_value) +
                            " is above maximum " + formatNumber(*state_->maximum) + ".");
    }


    matrix *
    parameter::matrix_value() noexcept
    {
        return std::get_if<matrix>(&state_->value);
    }


    const matrix *
    parameter::matrix_value() const noexcept
    {
        return std::get_if<matrix>(&state_->value);
    }


    void
    parameter::set_matrix(const matrix & v)
    {
        if(state_->type != matrix_type)
            throw exception("Invalid parameter value");

        matrix * stored_matrix = matrix_value();
        if(!stored_matrix)
            throw exception("Matrix parameter does not contain matrix storage.");

        matrix replacement;
        replacement.copy(v);
        const bool shape_changed = stored_matrix->shape() != replacement.shape() ||
                                   stored_matrix->size() != replacement.size();
        if(state_->resolved && shape_changed && !state_->dynamic)
            throw exception("Matrix parameter shape cannot change after startup from " +
                            format_shape(stored_matrix->shape()) + " to " +
                            format_shape(replacement.shape()) + ".");

        if(shape_changed)
            stored_matrix->realloc(replacement.shape());
        stored_matrix->copy(replacement);
        state_->resolved = true;
    }


    matrix &
    parameter::matrix_ref()
    {
        if(auto stored_matrix = matrix_value())
            return *stored_matrix;
        throw exception("Not a matrix value.");
    }


    const matrix &
    parameter::matrix_ref() const
    {
        if(auto stored_matrix = matrix_value())
            return *stored_matrix;
        throw exception("Not a matrix value.");
    }


    matrix
    parameter::as_matrix() const
    {
        matrix copied_matrix;
        copied_matrix.copy(matrix_ref());
        return copied_matrix;
    }


    int
    parameter::size() const
    {
        if(auto stored_matrix = matrix_value())
            return stored_matrix->size();
        throw exception("Not a matrix value.");
    }


    float
    parameter::get(int index, float default_value) const
    {
        if(auto stored_matrix = matrix_value())
        {
            if(index < 0 || index >= stored_matrix->size())
                return default_value;
            const int block_size = stored_matrix->logical_block_size();
            return stored_matrix->logical_block_data(index / block_size)[index % block_size];
        }
        throw exception("Not a matrix value.");
    }


    float
    parameter::operator[](int index) const
    {
        if(auto stored_matrix = matrix_value())
        {
            if(index < 0 || index >= stored_matrix->size())
                throw std::out_of_range("Parameter matrix index out of range.");
            const int block_size = stored_matrix->logical_block_size();
            return stored_matrix->logical_block_data(index / block_size)[index % block_size];
        }
        throw exception("Not a matrix value.");
    }


    parameter::operator std::string() const
    {
        if(state_->has_options)
        {
            auto option_index = std::get_if<int>(&state_->value);
            if(!option_index)
                throw exception("Option parameter missing index value.");
            int index = *option_index;
            if(index < 0 || static_cast<std::size_t>(index) >= state_->options.size())
                return std::to_string(index)+" (OUT-OF-RANGE)";
            else
                return state_->options[index];
        } 

        switch(state_->type)
        {
            case no_type: throw exception("Uninitialized or unbound parameter.");
            case number_type:
            case rate_type:
                if(auto number_value = std::get_if<double>(&state_->value))
                    return formatNumber(*number_value);
                break;
            case bool_type:
                if(auto bool_value = std::get_if<bool>(&state_->value))
                    return (*bool_value ? "true" : "false");
                break;
            case string_type:
                if(auto string_value = std::get_if<std::string>(&state_->value))
                    return *string_value;
                break;
            case matrix_type:
                if(auto stored_matrix = matrix_value())
                    return stored_matrix->json();
                break;
            default:
                break;
        }
        throw exception("Type conversion error for parameter.");
    }


    parameter::operator double() const
    {
        if(state_->has_options)
        {
            if(auto option_index = std::get_if<int>(&state_->value))
                return *option_index;
            throw exception("Option parameter missing index value.");
        }

        if(state_->type == rate_type)
        {
            if(auto number_value = std::get_if<double>(&state_->value))
                return *number_value * kernel().GetTickDuration();
        }
        if(auto number_value = std::get_if<double>(&state_->value))
            return *number_value;
        else if(auto bool_value = std::get_if<bool>(&state_->value))
            return *bool_value ? 1.0 : 0.0;
        else if(auto string_value = std::get_if<std::string>(&state_->value))
            return parse_parameter_number(*string_value, "double");
        else if(auto stored_matrix = matrix_value())
            return get_scalar_matrix_value(*stored_matrix, "double");
        else
            throw exception("Type conversion error. Parameter does not have a type Check spelling IKC and cc file.");
    }


    parameter::operator bool() const
    {
        return as_bool();
    }


    bool
    parameter::as_bool() const
    {
        if(state_->has_options)
            return as_int() != 0;
        if(state_->type == bool_type)
        {
            if(auto bool_value = std::get_if<bool>(&state_->value))
                return *bool_value;
        }
        if(state_->type == string_type)
        {
            if(auto string_value = std::get_if<std::string>(&state_->value))
                return is_true(*string_value);
        }
        return as_double() != 0;
    }


    float
    parameter::as_float() const
    {
        return float(as_double());
    }


    double
    parameter::as_double() const
    {
        return double(*this);
    }


    long
    parameter::as_long() const
    {
        if(state_->has_options)
        {
            if(auto option_index = std::get_if<int>(&state_->value))
                return static_cast<long>(*option_index);
            throw exception("Option parameter missing index value.");
        }

        switch(state_->type)
        {
            case no_type: throw exception("Uninitialized_parameter.");
            case number_type:
            case rate_type:
                if(auto number_value = std::get_if<double>(&state_->value))
                    return checked_truncating_long(
                        state_->type == rate_type ? *number_value * kernel().GetTickDuration() : *number_value,
                        "long"
                    );
                break;
            case bool_type:
                if(auto bool_value = std::get_if<bool>(&state_->value))
                    return *bool_value ? 1L : 0L;
                break;
            case string_type:
                if(auto string_value = std::get_if<std::string>(&state_->value))
                    return checked_truncating_long(parse_parameter_number(*string_value, "long"), "long");
                break;
            case matrix_type:
                if(auto stored_matrix = matrix_value())
                    return checked_truncating_long(get_scalar_matrix_value(*stored_matrix, "long"), "long");
                throw exception("Could not convert matrix to long");
            default: ;
        }
        throw exception("Type conversion error for parameter");
    }


    int
    parameter::as_int() const
    {
        if(state_->has_options)
        {
            if(auto option_index = std::get_if<int>(&state_->value))
                return *option_index;
            throw exception("Option parameter missing index value.");
        }

        switch(state_->type)
        {
            case no_type: throw exception("Uninitialized_parameter.");
            case number_type:
            case rate_type:
                if(auto number_value = std::get_if<double>(&state_->value))
                    return checked_truncating_int(
                        state_->type == rate_type ? *number_value * kernel().GetTickDuration() : *number_value,
                        "int"
                    );
                break;
            case bool_type:
                if(auto bool_value = std::get_if<bool>(&state_->value))
                    return *bool_value ? 1 : 0;
                break;
            case string_type:
                if(auto string_value = std::get_if<std::string>(&state_->value))
                    return checked_truncating_int(parse_parameter_number(*string_value, "int"), "int");
                break;
            case matrix_type:
                if(auto stored_matrix = matrix_value())
                    return checked_truncating_int(get_scalar_matrix_value(*stored_matrix, "int"), "int");
                throw exception("Could not convert matrix to int");
            default: ;
        }
        throw exception("Type conversion error for  parameter");
    }


    std::string
    parameter::as_int_string() const
    {
        return std::to_string(as_int());    
    }


    parameter_type
    parameter::get_type() const noexcept
    {
        return state_->type;
    }


    bool
    parameter::has_options() const noexcept
    {
        return state_->has_options;
    }


    bool
    parameter::is_resolved() const noexcept
    {
        return state_->resolved;
    }


    std::vector<std::string>
    parameter::options() const
    {
        return state_->options;
    }


    dictionary
    parameter::metadata() const
    {
        return state_->info.copy();
    }


    void
    parameter::set_source_value(const std::string & source_value)
    {
        state_->info["value"] = source_value;
    }


    std::string
    parameter::as_string() const
    {
        return std::string(*this);
    }



    bool
    parameter::empty() const
    {
        return (*this).as_string().empty();
    }



    void
    parameter::print(std::string name) const
    {
        const dictionary & metadata = state_->info;
        if(name.empty() && metadata.contains_non_null("name"))
            name = std::string(metadata.at("name"));

        if(!name.empty())
            std::cout << name << " = ";
        if(state_->type == no_type)
            std::cout << "not initialized\n";
        else if(!state_->resolved)
            std::cout << "unresolved\n";
        else
            std::cout << as_string() << '\n';
    }


    void
    parameter::info() const
    {
        const dictionary & metadata = state_->info;
        const auto metadata_value = [&](const std::string & key, const std::string & fallback)
        {
            return metadata.contains_non_null(key) ? std::string(metadata.at(key)) : fallback;
        };

        const std::size_t type_index = static_cast<std::size_t>(state_->type);
        const std::string type_name = type_index < parameter_strings.size() ?
                                      std::string(parameter_strings[type_index]) : "unknown";

        std::cout << "name: " << metadata_value("name", "(unnamed)") << '\n';
        std::cout << "type: " << type_name << '\n';
        std::cout << "resolved: " << (state_->resolved ? "true" : "false") << '\n';
        std::cout << "default: " << metadata_value("default", "(none)") << '\n';
        std::cout << "source: " << metadata_value("value", "(none)") << '\n';
        std::cout << "has_options: " << (state_->has_options ? "true" : "false") << '\n';
        std::cout << "options: ";
        if(state_->options.empty())
            std::cout << "(none)";
        else
        {
            std::string separator;
            for(const std::string & option : state_->options)
            {
                std::cout << separator << option;
                separator = ", ";
            }
        }
        std::cout << '\n';
        std::cout << "minimum: " << (state_->minimum ? formatNumber(*state_->minimum) : "(none)") << '\n';
        std::cout << "maximum: " << (state_->maximum ? formatNumber(*state_->maximum) : "(none)") << '\n';
        std::cout << "value: " << (state_->resolved ? as_string() : "unresolved") << '\n';
    }

    std::string 
    parameter::json() const
    {
        if(state_->has_options)
        {
            if(state_->type == number_type || state_->type == rate_type)
                return "[["+format_json_number(as_double())+"]]";
            if(state_->type == bool_type)
                return (as_bool() ? "[[true]]" : "[[false]]");
            if(state_->type == string_type)
                return "\""+escape_json_string(as_string())+"\"";
            throw exception("Cannot convert parameter to string");
        }

        if((state_->type == number_type || state_->type == rate_type) &&
           std::holds_alternative<double>(state_->value))
            return "[["+format_json_number(std::get<double>(state_->value))+"]]";
        if(state_->type == bool_type && std::holds_alternative<bool>(state_->value))
            return (std::get<bool>(state_->value) ? "[[true]]" : "[[false]]");
        if(state_->type == string_type && std::holds_alternative<std::string>(state_->value))
            return "\""+escape_json_string(std::get<std::string>(state_->value))+"\"";
        if(state_->type == matrix_type)
            if(auto stored_matrix = matrix_value())
                return stored_matrix->json();
        throw exception("Cannot convert parameter to string");
    }


    std::ostream& operator<<(std::ostream& os, const parameter & p)
    {
        os << p.as_string();
        return os;
    }


// Component

    void
    Component::print() const
    {
        std::cout << "Component: " << info_["name"]  << '\n';
    }


    int
    Component::EffectiveFirstTick() const
    {
        if(module_start == 1)
            return startup_first_real_input_step == std::numeric_limits<int>::max() ? std::numeric_limits<int>::max() : startup_first_real_input_step;
        if(module_start == 2)
            return startup_all_real_inputs_step == std::numeric_limits<int>::max() ? std::numeric_limits<int>::max() : startup_all_real_inputs_step;
        return start_tick;
    }


    bool
    Component::ShouldTick() const
    {
        int scheduled_start_tick = EffectiveFirstTick();
        if(scheduled_start_tick == std::numeric_limits<int>::max())
            return false;
        return kernel().GetTick() >= scheduled_start_tick;
    }


    void
    Component::SyncFirstTickFromParameter()
    {
        module_start = GetParameter("module_start").as_int();
        start_tick = GetParameter("start_tick").as_int();

        if(module_start < 0 || module_start > 2)
            throw exception("Invalid module_start value \"" + std::to_string(module_start) + "\". Expected 0 (at_tick), 1 (first_data), or 2 (all_data).");
    }


    bool
    Component::IsAsyncRunning() const
    {
        return async_mode && async_running.load();
    }


    bool
    Component::IsAsyncPending() const
    {
        return async_pending_action_count.load() > 0;
    }


    bool
    Component::IsAsyncFailed() const
    {
        return async_failed.load();
    }


    void
    Component::SyncAsyncModeFromParameter()
    {
        async_mode = GetParameter("async").as_bool();
    }


    bool
    Component::PollAsyncCompletion(bool apply_pending_actions)
    {
        std::exception_ptr error;
        {
            std::lock_guard<std::mutex> lock(async_state_mutex);
            if(!async_running.load() || !async_future.valid())
                return false;

            if(async_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
                return false;

            error = async_future.get();
            async_completed_tick = kernel().GetTick();
        }

        if(error)
        {
            async_failed = true;
            ClearPendingAsyncActions();
            async_running = false;
            try
            {
                std::rethrow_exception(error);
            }
            catch(const std::exception & e)
            {
                throw exception("Asynchronous tick failed in \"" + path_ + "\": " + e.what(), path_);
            }
            catch(...)
            {
                throw exception("Asynchronous tick failed in \"" + path_ + "\": Unknown error.", path_);
            }
        }

        if(apply_pending_actions)
        {
            try
            {
                ApplyPendingAsyncActions();
            }
            catch(const std::exception & e)
            {
                async_failed = true;
                async_publish_pending = false;
                async_running = false;
                ClearPendingAsyncActions();
                throw exception("Deferred action failed for asynchronous component \"" + path_ + "\": " + e.what(), path_);
            }
            catch(...)
            {
                async_failed = true;
                async_publish_pending = false;
                async_running = false;
                ClearPendingAsyncActions();
                throw exception("Deferred action failed for asynchronous component \"" + path_ + "\": Unknown error.", path_);
            }
            async_publish_pending = true;
        }
        else
        {
            ClearPendingAsyncActions();
            async_publish_pending = false;
        }
        async_running = false;
        return true;
    }


    void
    Component::LaunchAsyncTick()
    {
        std::lock_guard<std::mutex> lock(async_state_mutex);
        if(async_running.load() || async_failed.load() || async_publish_pending.load())
            return;

        const AsyncRuntimeSnapshot runtime_snapshot = CaptureAsyncRuntimeSnapshot(kernel());
        async_started_tick = kernel().GetTick();
        async_running = true;
        try
        {
            async_future = std::async(std::launch::async, [this, runtime_snapshot]() -> std::exception_ptr
            {
                AsyncRuntimeSnapshotScope runtime_snapshot_scope(runtime_snapshot);
                const bool profiling_started = TryProfilingBegin();
                try
                {
                    Tick();
                    if(profiling_started)
                        ProfilingEnd();
                    return nullptr;
                }
                catch(...)
                {
                    if(profiling_started)
                        ProfilingEnd();
                    return std::current_exception();
                }
            });
        }
        catch(...)
        {
            async_running = false;
            async_failed = true;
            throw;
        }
    }


    void
    Component::WaitForAsyncCompletion(bool apply_pending_actions)
    {
        {
            std::unique_lock<std::mutex> lock(async_state_mutex);
            if(!async_running.load() || !async_future.valid())
                return;

            async_future.wait();
        }
        PollAsyncCompletion(apply_pending_actions);
    }


    void
    Component::ClearPendingAsyncActions()
    {
        std::lock_guard<std::mutex> lock(async_pending_mutex);
        deferred_parameter_changes.clear();
        deferred_commands.clear();
        async_pending_action_count = 0;
    }


    std::string
    Component::AsyncParameterChangeKey(const DeferredParameterChange & change)
    {
        std::string key = change.parameter_path;
        if(change.is_matrix_cell)
            key += ":" + std::to_string(change.x) + ":" + std::to_string(change.y);
        return key;
    }


    void
    Component::QueueDeferredParameterChange(const DeferredParameterChange & change)
    {
        std::lock_guard<std::mutex> lock(async_pending_mutex);
        deferred_parameter_changes[AsyncParameterChangeKey(change)] = change;
        async_pending_action_count = static_cast<int>(deferred_parameter_changes.size() + deferred_commands.size());
    }


    void
    Component::QueueDeferredCommand(const std::string & command_name, const dictionary & parameters)
    {
        std::lock_guard<std::mutex> lock(async_pending_mutex);
        constexpr size_t max_deferred_commands = 100;
        if(deferred_commands.size() >= max_deferred_commands)
        {
            Warning("Async command queue is full; ignoring command \"" + command_name + "\".", path_);
            return;
        }

        DeferredCommand command;
        command.command_name = command_name;
        command.parameters = parameters.copy();
        deferred_commands.push_back(std::move(command));
        async_pending_action_count = static_cast<int>(deferred_parameter_changes.size() + deferred_commands.size());
    }


    void
    Component::ApplyPendingAsyncActions()
    {
        Kernel & k = kernel();
        std::map<std::string, DeferredParameterChange> parameter_changes;
        std::vector<DeferredCommand> commands;
        {
            std::lock_guard<std::mutex> lock(async_pending_mutex);
            parameter_changes.swap(deferred_parameter_changes);
            commands.swap(deferred_commands);
            async_pending_action_count = 0;
        }

        for(auto & [_, change] : parameter_changes)
        {
            auto parameter_it = k.parameters.find(change.parameter_path);
            if(parameter_it == k.parameters.end())
            {
                Warning("Queued parameter \"" + change.parameter_path + "\" no longer exists.", path_);
                continue;
            }

            parameter & p = parameter_it->second;
            if(change.is_matrix_cell)
            {
                if(p.get_type() == matrix_type)
                {
                    matrix & matrix_value = p.matrix_ref();
                    double value = parse_parameter_number(change.value, "matrix parameter cell");
                    if(matrix_value.rank() == 1)
                        matrix_value(change.x)= value;
                    else if(matrix_value.rank() == 2)
                        matrix_value(change.y, change.x)= value;
                    else
                        Warning("Queued parameter \"" + change.parameter_path + "\" has unsupported matrix rank.", path_);
                }
                else
                    Warning("Queued parameter \"" + change.parameter_path + "\" is not a matrix.", path_);
            }
            else
            {
                k.SetParameter(change.parameter_path, change.value);
            }
        }
        for(auto & command : commands)
            Command(command.command_name, command.parameters);
    }


    std::string
    Component::StartupFirstRealInputStepString() const
    {
        return startup_first_real_input_step == std::numeric_limits<int>::max() ? "unknown" : std::to_string(startup_first_real_input_step);
    }


    std::string
    Component::StartupAllRealInputsStepString() const
    {
        return startup_all_real_inputs_step == std::numeric_limits<int>::max() ? "unknown" : std::to_string(startup_all_real_inputs_step);
    }

        void 
        Component::info() const
        {
            std::cout << "Component: " << info_["name"]  << '\n';
            std::cout << "Path: " << path_  << '\n';
            std::cout << "Path: " << info_  << '\n';
        }

    bool 
    Component::BindParameter(parameter & p,  std::string & name) // Handle parameter sharing
    {
        std::string bind_to = GetBind(name);
        if(bind_to.empty())
            return false;
        else
            return LookupParameter(p, bind_to);
    }



    bool
    Component::GetRawParameterValue(const parameter & p, const std::string & name,
                                    std::string & raw_value, Component *& context) const
    {
        const Component * value_owner = GetValueOwner(name);
        if(value_owner && value_owner->info_.contains_non_null(name))
        {
            raw_value = std::string(value_owner->info_[name]);
            context = const_cast<Component *>(value_owner);
            return true;
        }

        context = const_cast<Component *>(this);
        if(p.metadata().contains("default"))
        {
            raw_value = std::string(p.metadata()["default"]);
            return true;
        }

        if(p.get_type() == matrix_type && !MatrixParameterShapeExpression(p).empty())
        {
            raw_value.clear();
            return true;
        }

        return false;
    }


    std::string
    Component::MatrixParameterShapeExpression(const parameter & p) const
    {
        if(p.metadata().contains_non_null("shape"))
            return std::string(p.metadata()["shape"]);
        if(p.metadata().contains_non_null("size"))
            return std::string(p.metadata()["size"]);
        return "";
    }


    matrix
    Component::ApplyParameterShape(const parameter & p, const matrix & value)
    {
        std::string shape_expression = MatrixParameterShapeExpression(p);
        if(shape_expression.empty())
            return value;

        std::vector<int> shape = EvaluateShapeList(shape_expression);
        if(shape.empty())
            throw exception("Matrix parameter shape \"" + shape_expression +
                            "\" did not resolve to a valid shape.");

        matrix shaped(shape);
        if(value.is_uninitialized() || value.empty())
            return shaped;

        if(value.size() > shaped.size())
            throw exception("Matrix parameter value has " + std::to_string(value.size()) +
                            " elements but shape \"" + shape_expression +
                            "\" only allows " + std::to_string(shaped.size()) + ".");

        float * target = shaped.contiguous_data();
        int target_index = 0;
        for(int block = 0; block < value.logical_block_count(); ++block)
        {
            std::copy_n(value.logical_block_data(block), value.logical_block_size(),
                        target + target_index);
            target_index += value.logical_block_size();
        }
        return shaped;
    }


    void
    Component::ResolveParameterValue(parameter & p, const std::string & name,
                                     const std::string & raw_value, Component * context)
    {
        if((p.get_type() == number_type || p.get_type() == rate_type) && !p.has_options())
        {
            SetParameter(name, formatNumber(context->ComputeDouble(raw_value)));
            return;
        }

        if(p.get_type() == matrix_type)
        {
            matrix literal;
            if(raw_value.empty() && !MatrixParameterShapeExpression(p).empty())
                SetParameter(name, ApplyParameterShape(p, matrix()), raw_value);
            else if(try_parse_matrix_literal(literal, raw_value))
                SetParameter(name, ApplyParameterShape(p, literal), raw_value);
            else
                SetParameter(name, ApplyParameterShape(p, matrix(context->ComputeValue(raw_value))),
                             raw_value);
            return;
        }

        if(p.get_type() == bool_type && !p.has_options())
        {
            SetParameter(name, std::string(context->ComputeBool(raw_value) ? "true" : "false"));
            return;
        }

        if(raw_value.find('@') != std::string::npos || raw_value.find('{') != std::string::npos)
            SetParameter(name, context->ComputeValue(raw_value));
        else
            SetParameter(name, raw_value);
    }


    bool
    Component::ResolveParameter(parameter & p,  std::string & name)
    {
        if(p.is_resolved())
            return true; // Already set from SetParameters

        try
        {
            // Look for binding
            std::string bind_to = GetBind(name);
            if(!bind_to.empty())
            {
                if(LookupParameter(p, bind_to))
                    return true;
            }

            std::string raw_value;
            Component * context = nullptr;
            if(!GetRawParameterValue(p, name, raw_value, context))
            {
                Error("Parameter \""+name+"\" has no default value in the ikc file.");
                return false;
            }

            ResolveParameterValue(p, name, raw_value, context);
            return true;
        }
        catch(exception & e)
        {
            throw exception("Could not resolve parameter \"" + name + "\": " + e.message(), e.path().empty() ? path_+"."+name : e.path());
        }
        catch(std::exception & e)
        {
            throw exception("Could not resolve parameter \"" + name + "\": " + e.what(), path_+"."+name);
        }
        return false;
    }



    bool 
    Component::KeyExists(const std::string & key) const
    {        
        if(info_.contains(key))
            return true;
        if(parent_)
            return parent_->KeyExists(key);
        else
            return false;
    }



    std::string 
    Component::GetValue(const std::string & key) const
    {        
        if(info_.contains(key))
            return info_[key];
        if(parent_)
            return parent_->GetValue(key);
        return kernel().GetTopLevelDefaultAttribute(key);
    }


    const Component *
    Component::GetValueOwner(const std::string & key) const
    {
        if(info_.contains(key))
            return this;
        if(parent_)
            return parent_->GetValueOwner(key);
        return nullptr;
    }
//
// GetComponent
//
// literal component navigation relative to the current component
//
 
    Component * 
    Component::GetComponent(const std::string & s) 
    {
        std::string path = s;
        try
        {
            if(path.empty()) // this
                return this;
            if(path[0]=='.') // global
                return kernel().components.at(path.substr(1)).get();
            if(kernel().components.count(path_+"."+peek_head(path,"."))) // inside
                return kernel().components[path_+"."+peek_head(path,".")]->GetComponent(peek_tail(path,"."));
            if(peek_rtail(peek_rhead(path_,"."),".") == peek_head(path,".") && parent_) // parent
                return parent_->GetComponent(peek_tail(path,"."));
            throw exception("Component \""+path+"\" does not exist.");
        }
        catch(const std::exception& e)
        {
            throw exception("Component \""+path+"\" does not exist.");
        }
    }


    int 
    Component::GetIntValue(const std::string & name, int d) const
    {
        std::string value = GetValue(name);
        if(value.empty())
            return d;
        try
        {
            return parse_scalar_state_int(value);
        }
        catch(const std::exception & e)
        {
            throw exception("Attribute \"" + name + "\" has invalid integer value \"" + value + "\": " + e.what(),
                            path_ + "." + name);
        }
    }


    std::string 
    Component::GetBind(const std::string & name) const
    {
        if(info_.contains(name))
            return ""; // Value set in attribute - do not bind
        if(info_.contains(name+".bind")) 
            return info_[name+".bind"];
        if(parent_)
            return parent_->GetBind(name);
        return "";
    }

       void Component::Bind(parameter & p, std::string name)
    {
        Kernel & k = kernel();
        std::string pname = path_+"."+name;
        if(k.parameters.count(pname))
            p.bind_to(kernel().parameters[pname]);
        else
            throw exception("Cannot bind to \""+name+"\"");
    };


    void Component::Bind(matrix & m, std::string n) // Bind input, output or parameter
    {
        std::string name = path_+"."+n;
        try
        {
            Kernel & k = kernel();
            if(k.buffers.count(name))
                m = k.buffers[name];
            else if(k.parameters.count(name))
                m = k.parameters[name].matrix_ref();
            else if(KeyExists(n))
                throw exception("Cannot bind to attribute \""+name+"\". Define it as a parameter!", path_);
            else
                throw exception("Input, output or parameter named \""+name+"\" does not exist", path_);
        }
        catch(const exception & e)
        {
            throw exception("Bind:\""+name+"\" failed. "+e.message(), path_);
        }
    }


    template<typename T>
    void
    Kernel::BindScalarState(T & value, const std::string & name,
                            const std::string & component_path,
                            const std::string & expected_type,
                            T ScalarState::* stored_value,
                            T * ScalarState::* bound_value)
    {
        auto it = scalar_states.find(name);
        if(it == scalar_states.end())
            throw exception("Bind:\"" + name + "\" failed. Scalar state does not exist.", component_path);

        ScalarState & state = it->second;
        if(state.type != expected_type)
            throw exception("Bind:\"" + name + "\" failed. Expected state type " +
                            expected_type + " but got " + state.type + ".",
                            component_path);

        T *& binding = state.*bound_value;
        if(binding)
        {
            if(binding == &value)
                return;
            throw exception("Bind:\"" + name +
                            "\" failed. Scalar state is already bound to another variable.",
                            component_path);
        }

        value = state.*stored_value;
        binding = &value;
    }


    void Component::Bind(float & v, std::string n)
    {
        kernel().BindScalarState(v, path_ + "." + n, path_, "float",
                                 &Kernel::ScalarState::float_value,
                                 &Kernel::ScalarState::float_ptr);
    }


    void Component::Bind(double & v, std::string n)
    {
        kernel().BindScalarState(v, path_ + "." + n, path_, "double",
                                 &Kernel::ScalarState::double_value,
                                 &Kernel::ScalarState::double_ptr);
    }


    void Component::Bind(int & v, std::string n)
    {
        kernel().BindScalarState(v, path_ + "." + n, path_, "int",
                                 &Kernel::ScalarState::int_value,
                                 &Kernel::ScalarState::int_ptr);
    }


    void Component::Bind(bool & v, std::string n)
    {
        kernel().BindScalarState(v, path_ + "." + n, path_, "bool",
                                 &Kernel::ScalarState::bool_value,
                                 &Kernel::ScalarState::bool_ptr);
    }


    void Component::Bind(std::string & v, std::string n)
    {
        kernel().BindScalarState(v, path_ + "." + n, path_, "string",
                                 &Kernel::ScalarState::string_value,
                                 &Kernel::ScalarState::string_ptr);
    }


    parameter &  
    Component::GetParameter(std::string name)
    {
        return kernel().parameters.at(path_+"."+name);
    }



    void Component::AddInput(dictionary parameters)
    {
        std::string input_name = path_+"."+validate_identifier(parameters["name"]);
        kernel().AddInput(input_name, parameters);
    }

    void Component::AddOutput(dictionary parameters)
    {
        std::string output_name = path_+"."+validate_identifier(parameters["name"]);
        kernel().AddOutput(output_name, parameters);
      };

    void Component::AddState(dictionary parameters)
    {
        std::string state_name = path_+"."+validate_identifier(parameters["name"]);
        kernel().AddState(state_name, parameters);
    }

    void Component::AddOutput(std::string name, int size, std::string description)
    {
        dictionary o = {
            {"name", name},
            {"size", std::to_string(size)},
            {"description", description},
            {"_tag", "output"}
        };
        list(info_["outputs"]).push_back(o);
        AddOutput(o);
    }

    void Component::AddParameter(dictionary parameters)
    {
        try
        {         
            std::string parameter_name = path_+"."+validate_identifier(parameters["name"]);
            kernel().AddParameter(parameter_name, parameters);
        }
        catch(const std::exception& e)
        {
            throw exception("While adding parameter \""+std::string(parameters["name"])+"\": "+ e.what());
        }
    }


    void Component::SetParameter(std::string name, std::string value)
    {
        std::string parameter_name = path_+"."+validate_identifier(name);
        kernel().SetParameter(parameter_name, value);
    }


    void Component::SetParameter(std::string name, const matrix & value, const std::string & source_value)
    {
        std::string parameter_name = path_+"."+validate_identifier(name);
        kernel().SetParameter(parameter_name, value, source_value);
    }


    bool Component::LookupParameter(parameter & p, const std::string & name)
    {
        Kernel & k = kernel();
        if(k.parameters.count(path_+"."+name))
        {
            p.bind_to(k.parameters[path_+"."+name]);
            return true;
        }
        else if(parent_)
            return parent_->LookupParameter(p, name);
        else
            return false;
    }




    matrix & 
    Component::GetBuffer(const std::string & s)
    {
        return kernel().buffers.at(path_+'.'+s);
    }

    std::string
    Component::ComputeValue(const std::string & s)
    {
        return ComputeEngine(*this).ComputeValue(s);
    }


    std::string
    Component::ComputeValueOf(const std::string & name)
    {
        return ComputeValue("@" + name);
    }


    double
    Component::ComputeDouble(const std::string & s)
    {
        return ComputeEngine(*this).ComputeDouble(s);
    }


    int
    Component::ComputeInt(const std::string & s)
    {
        return ComputeEngine(*this).ComputeInt(s);
    }


    bool
    Component::ComputeBool(const std::string & s)
    {
        return ComputeEngine(*this).ComputeBool(s);
    }


    bool
    Component::ComputeAttributeBool(dictionary d, const std::string & name, bool default_value)
    {
        if(!d.contains(name))
            return default_value;

        return ComputeBool(std::string(d[name]));
    }


    std::vector<int> 
    Component::EvaluateShapeList(std::string & s) // return shape list from shape expression string
    {
        return ComputeEngine(*this).EvaluateShapeList(s);
    }

    void
    Component::AddLogLevel()
    {
        for(auto p: info_["parameters"])
            if(p["name"].as_string()=="log_level")
                return;

        info_["parameters"].push_back(make_log_level_parameter().copy());
    }

    void
    Component::AddFirstTick()
    {
        for(auto p: info_["parameters"])
            if(p["name"].as_string()=="module_start")
            {
                bool has_start_tick = false;
                for(auto q: info_["parameters"])
                    if(q["name"].as_string()=="start_tick")
                    {
                        has_start_tick = true;
                        break;
                    }

                if(!has_start_tick)
                    info_["parameters"].push_back(make_start_tick_parameter().copy());
                return;
            }

        info_["parameters"].push_back(make_module_start_parameter().copy());
        info_["parameters"].push_back(make_start_tick_parameter().copy());
    }

    Component::Component():
        Task(Task::Kind::component),
        parent_(nullptr),
        info_(kernel().current_component_info),
        path_(kernel().current_component_path),
        module_start(0),
        start_tick(0),
        startup_first_real_input_step(std::numeric_limits<int>::max()),
        startup_all_real_inputs_step(std::numeric_limits<int>::max()),
        initialized_(false),
        async_mode(false),
        async_running(false),
        async_failed(false),
        async_publish_pending(false),
        async_started_tick(-1),
        async_completed_tick(-1),
        async_pending_action_count(0)
    {
        ensure_component_collections(info_);

        // Add log_level parameter to all components

        AddLogLevel();
        AddFirstTick();

        for(auto p: info_["parameters"])
            AddParameter(p);

        for(auto input: info_["inputs"])
            AddInput(input);

        for(auto output: info_["outputs"])
            AddOutput(output);

        for(auto state: info_["states"])
            AddState(state);

    // Set parent

        auto p = path_.rfind('.');
        if(p != std::string::npos)
            parent_ = kernel().components.at(path_.substr(0, p)).get();
    }



    std::string
    Component::Info() const
    {
        return path_;
    }

    bool
    Component::Print(std::string message, std::string path)
    {
        return Notify(msg_print, message, path);
    }

    bool
    Component::Error(std::string message, std::string path)
    {
        return Notify(msg_fatal_error, message, path);
    }

    bool
    Component::Warning(std::string message, std::string path)
    {
        return Notify(msg_warning, message, path);
    }

    bool
    Component::Debug(std::string message, std::string path)
    {
        return Notify(msg_debug, message, path);
    }

    bool
    Component::Trace(std::string message, std::string path)
    {
        return Notify(msg_trace, message, path);
    }

    void
    Component::SetParameters()
    {
    }

    void
    Component::Tick()
    {
    }

    void
    Component::Init()
    {
    }

    void
    Component::Stop()
    {
    }

    void
    Component::Reset()
    {
        Kernel & k = kernel();
        for(auto state_info : info_["states"])
        {
            std::string state_path = path_ + "." + std::string(state_info["name"]);
            auto scalar = k.scalar_states.find(state_path);
            if(scalar != k.scalar_states.end())
            {
                Kernel::ScalarState & state = scalar->second;
                if(state.type == "float")
                {
                    state.float_value = state.default_float_value;
                    if(state.float_ptr)
                        *state.float_ptr = state.default_float_value;
                }
                else if(state.type == "double")
                {
                    state.double_value = state.default_double_value;
                    if(state.double_ptr)
                        *state.double_ptr = state.default_double_value;
                }
                else if(state.type == "int")
                {
                    state.int_value = state.default_int_value;
                    if(state.int_ptr)
                        *state.int_ptr = state.default_int_value;
                }
                else if(state.type == "bool")
                {
                    state.bool_value = state.default_bool_value;
                    if(state.bool_ptr)
                        *state.bool_ptr = state.default_bool_value;
                }
                else if(state.type == "string")
                {
                    state.string_value = state.default_string_value;
                    if(state.string_ptr)
                        *state.string_ptr = state.default_string_value;
                }
                continue;
            }

            auto buffer = k.buffers.find(state_path);
            if(buffer != k.buffers.end() && k.state_buffers.count(state_path) && !buffer->second.is_uninitialized())
                buffer->second.reset();
        }
    }

    void
    Component::Command(std::string command_name, dictionary & parameters)
    {
        std::cout << "Received command: " << command_name << "\n";
        parameters.print();
    }

    std::string
    Component::json(const std::string &)
    {
        return "";
    }

    bool
    Component::Notify(int msg, std::string message, std::string path)
    {
        if(path.empty())
            path = path_;
        if(msg == msg_fatal_error && !initialized_)
            throw fatal_error(message, path);

        try
        {
            int log_level = GetParameter("log_level");
            if(log_level == 0)
            {
                if(parent_)
                    return parent_->Notify(msg, message, path);
                else
                    return true;
            }

            if(msg <= log_level)
                return kernel().Notify(msg, message, path);
        }
        catch(...)
        {
            // ignore errors in logging
        }

        return true;
    }


    tick_count Module::GetTick() const        { return kernel().GetTick(); }
    double Module::GetTickDuration() const    { return kernel().GetTickDuration(); } // Time for each tick in seconds (s)
    double Module::GetTime() const            { return kernel().GetTime(); }
    double Module::GetRealTime() const        { return kernel().GetRealTime(); }
    double Module::GetNominalTime() const     { return kernel().GetNominalTime(); }
    double Module::GetRunTime() const         { return kernel().GetRunTime(); }
    double Module::GetTimeOfDay() const       { return kernel().GetTimeOfDay(); }
    double Module::GetLag() const             { return kernel().GetLag(); }
    double Module::GetUptime() const          { return kernel().GetUptime(); }
    double Module::GetActualTickDuration() const { return kernel().GetActualTickDuration(); }
    double Module::GetTickTimeUsage() const   { return kernel().GetTickTimeUsage(); }
    double Module::GetCPUUsage() const        { return kernel().GetCPUUsage(); }
    double Module::GetIdleTime() const        { return kernel().GetIdleTime(); }
    int Module::GetRunMode() const            { return kernel().GetRunMode(); }
    int Module::GetCPUCoreCount() const       { return kernel().GetCPUCoreCount(); }
    int Module::GetModuleCount() const        { return kernel().GetModuleCount(); }
    int Module::GetClassCount() const         { return kernel().GetClassCount(); }
    tick_count Module::GetStopAfter() const   { return kernel().GetStopAfter(); }


    Module::Module()
    {

    }

    bool
    Module::TryProfilingBegin()
    {
        if(!kernel().ProfilingEnabled())
            return false;

        ProfilingBegin();
        return true;
    }

    void
    Module::ProfilingBegin()
    {
        profiler_.begin();
    }

    void
    Module::ProfilingEnd()
    {
        profiler_.end();
    }

    INSTALL_CLASS(Module)

// The following lines will create the kernel the first time it is accessed by one of the components

    Kernel& kernel()
    {
        //static Kernel * kernelInstance = new Kernel();
        static Kernel kernelInstance;  // Guaranteed to be thread-safe in C++11 and later
        return kernelInstance;
    }


    void
    Kernel::WaitForAsyncComponents(bool discard_pending_actions)
    {
        std::lock_guard<std::mutex> lifecycle_lock(component_lifecycle_mutex);
        for(auto & [path, component] : components)
        {
            try
            {
                if(discard_pending_actions)
                    component->ClearPendingAsyncActions();
                component->WaitForAsyncCompletion(!discard_pending_actions);
            }
            catch(const std::exception & e)
            {
                Notify(msg_warning, "Could not finish asynchronous component \"" + path + "\": " + e.what(), path);
                component->ClearPendingAsyncActions();
            }
        }
    }


    bool
    Kernel::StopComponents()
    {
        bool stopped = true;
        WaitForAsyncComponents(true);

        for(auto & [path, component] : components)
        {
            if(!component->initialized_)
                continue;
            try
            {
                component->Stop();
            }
            catch(const std::exception & e)
            {
                Notify(msg_warning, "Could not stop component \"" + path + "\": " + e.what(), path);
                stopped = false;
            }
            catch(...)
            {
                Notify(msg_warning, "Could not stop component \"" + path + "\": Unknown error.", path);
                stopped = false;
            }
        }
        return stopped;
    }


    void
    Kernel::Clear()
    {
        std::lock_guard<std::mutex> lifecycle_lock(component_lifecycle_mutex);
        // FIXME: retain persistent components

        components.clear();

        connections.clear();
        buffers.clear();
        state_buffers.clear();
        persistent_outputs.clear();
        persistent_state_buffers.clear();
        scalar_states.clear();
        max_delays.clear();
        circular_buffers.clear();
        parameters.clear();
        tasks.clear();
        top_group_path.clear();

        clear_matrix_states();  // if(NOT PERSISTENT)

        tick = 0;
        //run_mode = run_mode_pause;
        //tick_is_running = false;
        tick_time_usage = 0;
        cpu_usage = 0;
        last_cpu = 0;
        cpu_usage_initialized = false;
        cpu_usage_sample_time = std::chrono::steady_clock::time_point{};
        run_clock_origin = std::chrono::steady_clock::time_point{};
        run_time = 0;
        run_clock_started = false;
        tick_duration = 1; // default value
        task_timeout = 5.0;
        actual_tick_duration = tick_duration;
        idle_time = 0;
        stop_after = -1;
        lag = 0;
        lag_min = 0;
        lag_max = 0;
        lag_sum = 0;
        session_logging_active = false;
        shutdown = false;
        ResetUISnapshotCache();
    }


    void
    Kernel::New()
    {
        Notify(msg_print, "New file");
        bool had_components = false;
        {
            std::lock_guard<std::recursive_mutex> lock(kernelLock);
            had_components = !components.empty();
        }
        if(had_components)
            Stop();

        std::lock_guard<std::recursive_mutex> lock(kernelLock);
        if(!had_components && components.size() > 0)
            StopComponents();
        Clear();

        dictionary d;

        d["_tag"] = "group";
        d["name"] = "Untitled";
        d["groups"] = list();
        d["modules"] = list();
        d["widgets"] = list();
        d["connections"] = list();
        d["inputs"] = list();
        d["outputs"] = list();
        d["parameters"] = list();
        d["stop"] = "-1";

        SetCommandLineParameters(d);
        d["filename"] = options_.stem(); // Preserve the command-line basename; WebUI saves write a UserData copy.
        BuildGroup(d);
        info_ = d;

        run_mode = run_mode_stop;
        session_id = new_session_id();
        ResetUISnapshotCache();
        try
        {
            SetUp();
            BuildUISnapshot();
            needs_reload = false;
            automatic_reload_suppressed_until_save.store(false, std::memory_order_release);
        }
        catch(const setup_failed & e)
        {
            Notify(msg_warning, "Could not create new file: " + e.message(), e.path());
            if(!components.empty())
                StopComponents();
            Clear();
            info_ = d;
            run_mode = run_mode_stop;
            needs_reload = true;
        }
        catch(const std::exception & e)
        {
            Notify(msg_warning, "Could not create new file: " + std::string(e.what()));
            if(!components.empty())
                StopComponents();
            Clear();
            info_ = d;
            run_mode = run_mode_stop;
            needs_reload = true;
        }
    }

    bool
    Kernel::Terminate()
    {
        if(stop_after!= -1 &&  tick >= stop_after)
        {
            if(options_.is_set("batch_mode"))
                run_mode = run_mode_quit;
            else
                run_mode = run_mode_pause;

        }
        return (stop_after!= -1 &&  tick >= stop_after) || notify_stop_requested.load() || global_terminate.load();
    }


    void
    Kernel::ScanClasses(std::string path)
    {
        if(!std::filesystem::exists(path))
        {
            std::cout << "Could not scan for classes \"" + path + "\". Directory not found.\n";
            return;
        }
        for(auto& p: std::filesystem::recursive_directory_iterator(path))
            if(std::string(p.path().extension())==".ikc")
            {
                const std::string name = p.path().stem();
                auto existing_class = classes.find(name);
                if(existing_class != classes.end() && !existing_class->second.path.empty())
                    throw exception("Duplicate class \"" + name +
                                    "\" was found in more than one .ikc file: \"" +
                                    existing_class->second.path + "\" and \"" +
                                    p.path().string() + "\".", p.path().string());

                try
                {
                    dictionary class_info;
                    LoadXMLWithRestrictedIncludes(class_info, p.path());

                    const std::string root_element = class_info["_tag"];
                    if(root_element != "class")
                        throw exception("Root element must be <class>, not <" + root_element + ">.");

                    const std::string declared_name = class_info["name"];
                    if(declared_name != name)
                        throw exception("Declared class name \"" + declared_name +
                                        "\" does not match filename \"" + name + "\".");

                    ensure_list(class_info, "parameters");

                    bool has_log_level = false;
                    bool has_module_start = false;
                    bool has_start_tick = false;
                    bool has_async = false;
                    bool has_color = false;
                    for(auto parameter : class_info["parameters"])
                    {
                        std::string parameter_name = parameter["name"];
                        if(parameter_name == "log_level")
                            has_log_level = true;
                        else if(parameter_name == "module_start")
                            has_module_start = true;
                        else if(parameter_name == "start_tick")
                            has_start_tick = true;
                        else if(parameter_name == "async")
                            has_async = true;
                        else if(parameter_name == "color")
                            has_color = true;
                    }

                    if(!has_log_level)
                        class_info["parameters"].push_back(make_log_level_parameter().copy());

                    if(!has_module_start)
                        class_info["parameters"].push_back(make_module_start_parameter().copy());

                    if(!has_start_tick)
                        class_info["parameters"].push_back(make_start_tick_parameter().copy());

                    if(!has_async)
                        class_info["parameters"].push_back(make_async_parameter().copy());

                    if(!has_color)
                        class_info["parameters"].push_back(make_color_parameter().copy());

                    Class & scanned_class = classes[name];
                    scanned_class.info_ = std::move(class_info);
                    scanned_class.name = name;
                    scanned_class.path = p.path();
                }
                catch(const exception & e)
                {
                    Notify(msg_warning, "Could not load class file \"" + p.path().string() + "\": " + e.message(), p.path().string());
                }
                catch(const std::exception & e)
                {
                    Notify(msg_warning, "Could not load class file \"" + p.path().string() + "\": " + e.what(), p.path().string());
                }
            }
    }


    void
    Kernel::ScanFiles(std::string path, bool system, bool examples)
    {
        if(!std::filesystem::exists(path))
        {
            std::cout << "Could not scan for files in \"" + path + "\". Directory not found.\n";
            return;
        }
        for(auto& p: std::filesystem::recursive_directory_iterator(path))
        {
            const std::string extension = p.path().extension().string();
            if(extension==".ikg")
            {
                try
                {
                    dictionary file_info;
                    LoadXMLWithRestrictedIncludes(file_info, p.path());
                    if(is_internal(file_info))
                        continue;
                }
                catch(const std::exception &)
                {
                }

                std::string name = p.path().stem();

                if(system)
                     system_files[name] = p.path();
                else if(examples)
                     examples_files[name] = p.path();
                else
                     user_files[name] = p.path();
            }
            else if(!system && !examples && extension==".state")
            {
                std::string name = p.path().filename().string();
                user_state_files[name] = p.path();
            }
        }
    }


    void
    Kernel::ListClasses()
    {
        std::cout << "\nClasses:\n";
        for(auto & [name, component_class] : classes)
        {
            (void)name;
            component_class.Print();
        }
    }



    void
    Kernel::InitCircularBuffers()
    {
        for(const auto & [buffer_name, delay] : max_delays)
        {
            if(delay < 1)
                continue;
            auto source_buffer = buffers.find(buffer_name);
            if(source_buffer == buffers.end())
                continue;

            try
            {
                circular_buffers.try_emplace(buffer_name,
                                             source_buffer->second,
                                             delay,
                                             ComponentForValuePath(buffer_name));
            }
            catch(const out_of_memory_matrix_error &)
            {
                throw setup_failed("Could not allocate " + std::to_string(delay) +
                                   " ticks of delay history for \"" + buffer_name + "\".",
                                   buffer_name);
            }
            catch(const std::bad_alloc &)
            {
                throw setup_failed("Could not allocate " + std::to_string(delay) +
                                   " ticks of delay history for \"" + buffer_name + "\".", buffer_name);
            }
            catch(const std::length_error &)
            {
                throw setup_failed("Delay history for \"" + buffer_name + "\" is too large.", buffer_name);
            }
        }
    }


    void
    Kernel::RotateBuffers()
    {
        for(auto & [name, history] : circular_buffers)
        {
            tick_count completed_tick = -1;
            bool record_async_completion = false;

            if(history.source_component != nullptr && history.source_component->async_mode)
            {
                if(history.source_component->IsAsyncRunning() ||
                   history.source_component->IsAsyncFailed())
                    continue;

                completed_tick = history.source_component->async_completed_tick.load();
                if(completed_tick < 0 || completed_tick == history.last_async_completion)
                    continue;
                record_async_completion = true;
            }

            try
            {
                history.buffer.rotate(*history.source_buffer);
            }
            catch(const std::exception & e)
            {
                throw fatal_runtime_error("Error updating delay history for \"" +
                                          name + "\": " + e.what(), name);
            }
            catch(...)
            {
                throw fatal_runtime_error("Unknown error updating delay history for \"" +
                                          name + "\".", name);
            }

            if(record_async_completion)
                history.last_async_completion = completed_tick;
        }
    }



    void
    Kernel::ListComponents()
    {
        std::cout << "\nComponents:\n";
        for(auto & [name, component] : components)
        {
            (void)name;
            component->print();
        }
    }


    void
    Kernel::ListConnections()
    {
        std::cout << "\nConnections:\n";
        for(auto & c : connections)
            c.Print();
    }


    void
    Kernel::ListInputs()
    {
        std::cout << "\nInputs:\n";
        for(auto & [name, buffer] : buffers)
            std::cout << "\t" << name <<  buffer.shape() << '\n';
    }


   void Kernel::ListOutputs()
    {
        std::cout << "\nOutputs:\n";
        for(auto & [name, buffer] : buffers)
            std::cout  << "\t" << name << buffer.shape() << '\n';
    }


    void
    Kernel::ListBuffers()
    {
        std::cout << "\nBuffers:\n";
        for(auto & [name, buffer] : buffers)
            std::cout << "\t" << name <<  buffer.shape() << '\n';
    }


    void
    Kernel::ListCircularBuffers()
    {
        if(circular_buffers.empty())
            return;

        std::cout << "\nCircularBuffers:\n";
        for(auto & [name, history] : circular_buffers)
        {
            const matrix & latest = history.buffer.get(1);
            std::cout << "\t" << name << " " << history.buffer.size() << " "
                      << latest.rank() << latest.shape() << '\n';
        }
    }


    void
    Kernel::ListTasks()
    {
        for(auto & task_group : tasks)
        {
            std::cout << "\nTasks:\n";
            for(auto & task: task_group)
                std::cout << "\t" << task->Info() << '\n';
        }
    }

   void
   Kernel::ListParameters()
    {
        std::cout << "\nParameters:\n";
        for(auto & [name, parameter] : parameters)
            std::cout  << "\t" << name << ": " << parameter << '\n';
    }


    void
    Kernel::PrintLog()
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        for(auto & s : log)
            std::cout << "ikaros: " << s.level_ << ": " << s.message_ << '\n';
        log.clear();
        first_webui_log_sequence = next_webui_log_sequence;
    }


    Kernel::Kernel():
        session_id(new_session_id()),
        needs_reload(true),
        shutdown(false),
        run_mode(run_mode_pause),
        idle_time(0),
        tick_duration(1),
        task_timeout(5.0),
        actual_tick_duration(0), // FIME: Use desired tick duration here
        tick_time_usage(0),
        tick(0),
        stop_after(-1),
        lag(0),
        lag_min(0),
        lag_max(0),
        lag_sum(0)
    {
        const unsigned int detected_cpu_cores = std::thread::hardware_concurrency();
        cpu_cores = detected_cpu_cores > 0 ? static_cast<int>(detected_cpu_cores) : 1;
        thread_pool = std::make_unique<ThreadPool>(default_thread_pool_size(cpu_cores));
    }

    tick_count
    Kernel::GetTick()
    {
        if(active_async_runtime_snapshot)
            return active_async_runtime_snapshot->tick;
        return tick;
    }

    double
    Kernel::GetTickDuration()
    {
        if(active_async_runtime_snapshot)
            return active_async_runtime_snapshot->tick_duration;
        return tick_duration;
    }

    double
    Kernel::GetTime()
    {
        if(active_async_runtime_snapshot)
            return active_async_runtime_snapshot->time;
        return (run_mode.load() == run_mode_realtime) ? GetRealTime() : static_cast<double>(tick)*tick_duration;
    }

    double
    Kernel::GetRealTime()
    {
        if(active_async_runtime_snapshot)
            return active_async_runtime_snapshot->real_time;
        return (run_mode.load() == run_mode_realtime) ? timer.GetTime() : static_cast<double>(tick)*tick_duration;
    }

    double
    Kernel::GetNominalTime()
    {
        if(active_async_runtime_snapshot)
            return active_async_runtime_snapshot->nominal_time;
        return static_cast<double>(tick)*tick_duration;
    }

    double
    Kernel::GetRunTime()
    {
        if(active_async_runtime_snapshot)
            return active_async_runtime_snapshot->run_time;
        return run_time;
    }

    double
    Kernel::GetLag()
    {
        if(active_async_runtime_snapshot)
            return active_async_runtime_snapshot->lag;
        return (run_mode.load() == run_mode_realtime) ? static_cast<double>(tick)*tick_duration - timer.GetTime() : 0;
    }

    double
    Kernel::GetUptime()
    {
        if(active_async_runtime_snapshot)
            return active_async_runtime_snapshot->uptime;
        return uptime_timer.GetTime();
    }

    double
    Kernel::GetActualTickDuration() const
    {
        if(active_async_runtime_snapshot)
            return active_async_runtime_snapshot->actual_tick_duration;
        return actual_tick_duration;
    }

    double
    Kernel::GetTickTimeUsage() const
    {
        if(active_async_runtime_snapshot)
            return active_async_runtime_snapshot->tick_time_usage;
        return tick_time_usage;
    }

    double
    Kernel::GetCPUUsage() const
    {
        if(active_async_runtime_snapshot)
            return active_async_runtime_snapshot->cpu_usage;
        return cpu_usage;
    }

    double
    Kernel::GetIdleTime() const
    {
        if(active_async_runtime_snapshot)
            return active_async_runtime_snapshot->idle_time;
        return idle_time;
    }

    int
    Kernel::GetRunMode() const
    {
        return run_mode.load();
    }

    int
    Kernel::GetCPUCoreCount() const
    {
        return cpu_cores;
    }

    int
    Kernel::GetModuleCount() const
    {
        return static_cast<int>(components.size());
    }

    int
    Kernel::GetClassCount() const
    {
        return static_cast<int>(classes.size());
    }

    tick_count
    Kernel::GetStopAfter() const
    {
        return stop_after;
    }

    bool
    Kernel::ProfilingEnabled() const
    {
        return profiling_enabled.load(std::memory_order_relaxed);
    }

    bool
    Kernel::Print(std::string message)
    {
        return Notify(msg_print, message);
    }

    bool
    Kernel::Warning(std::string message, std::string path)
    {
        return Notify(msg_warning, message, path);
    }

    bool
    Kernel::Debug(std::string message)
    {
        return Notify(msg_debug, message);
    }

    bool
    Kernel::Trace(std::string message)
    {
        return Notify(msg_trace, message);
    }

    void
    Kernel::SetOptions(const options & opts)
    {
        options_ = opts;
        auth_enabled_ = options_.is_explicitly_set("auth_password");
        auth_password_ = auth_enabled_ ? options_.get("auth_password") : "";
        if(auth_enabled_ && auth_password_.empty())
            throw exception("Authentication requires a non-empty password.");
        if(auth_enabled_ && !LoadOrCreateAuthCookieSecret())
            throw exception("Authentication could not initialize its persistent cookie secret in UserData.");
    }

    bool
    Kernel::HasOption(const std::string & key) const
    {
        return options_.is_set(key);
    }

    bool
    Kernel::IsOptionExplicitlySet(const std::string & key) const
    {
        return options_.is_explicitly_set(key);
    }

    std::string
    Kernel::GetOption(const std::string & key) const
    {
        return options_.get(key);
    }

    long
    Kernel::GetOptionLong(const std::string & key) const
    {
        return options_.get_long(key);
    }

    std::string
    Kernel::GetOptionFilename() const
    {
        return options_.filename();
    }

    std::string
    Kernel::GetOptionFullPath() const
    {
        return options_.full_path();
    }

    std::filesystem::path
    Kernel::GetClassDirectory(const std::string & class_name) const
    {
        auto it = classes.find(class_name);
        if(it == classes.end())
            return {};
        return std::filesystem::path(it->second.path).parent_path();
    }

    bool
    Kernel::SanitizeReadPath(const std::filesystem::path & candidate_path, std::filesystem::path & sanitized_path) const
    {
        if(candidate_path.empty())
            return false;

        std::error_code ec;
        std::filesystem::path project_root = std::filesystem::weakly_canonical(options_.ikaros_root, ec);
        if(ec)
            return false;

        std::filesystem::path user_root = std::filesystem::weakly_canonical(user_dir, ec);
        if(ec)
            return false;

        std::filesystem::path base_path = candidate_path;
        if(candidate_path.is_relative())
            base_path = std::filesystem::path(user_dir) / candidate_path;

        std::filesystem::path resolved_path = std::filesystem::weakly_canonical(base_path, ec);
        if(ec)
            return false;

        auto is_within_root = [](const std::filesystem::path & root, const std::filesystem::path & path)
        {
            auto root_it = root.begin();
            auto root_end = root.end();
            auto path_it = path.begin();
            auto path_end = path.end();

            for(; root_it != root_end && path_it != path_end; ++root_it, ++path_it)
                if(*root_it != *path_it)
                    return false;

            return root_it == root_end;
        };

        if(!is_within_root(project_root, resolved_path) && !is_within_root(user_root, resolved_path))
            return false;

        sanitized_path = resolved_path;
        return true;
    }

    bool
    Kernel::SanitizeWritePath(const std::filesystem::path & candidate_path, std::filesystem::path & sanitized_path) const
    {
        if(candidate_path.empty())
            return false;

        std::error_code ec;
        std::filesystem::path user_root = std::filesystem::weakly_canonical(user_dir, ec);
        if(ec)
            return false;

        std::filesystem::path base_path = candidate_path.is_absolute() ? candidate_path : (user_root / candidate_path);
        std::filesystem::path resolved_path = std::filesystem::weakly_canonical(base_path, ec);
        if(ec)
            return false;

        auto root_it = user_root.begin();
        auto root_end = user_root.end();
        auto path_it = resolved_path.begin();
        auto path_end = resolved_path.end();

        for(; root_it != root_end && path_it != path_end; ++root_it, ++path_it)
            if(*root_it != *path_it)
                return false;

        if(root_it != root_end)
            return false;

        sanitized_path = resolved_path;
        return true;
    }


    void
        Kernel::PrintProfiling()
        {
            for(auto & [name, component] : components)
            {
                (void)name;
                component->profiler_.print(component->path_);
            }
        }


    void 
    Kernel::CalculateCheckSum()
    {
        if(!info_.contains("check_sum"))
            return;

        long correct_check_sum = info_["check_sum"];
        long calculated_check_sum = 0;
        prime prime_number;

        // Iterate over task lists to test partitioning

        calculated_check_sum += prime_number.next() * tasks.size();
        for(auto & t : tasks)
            calculated_check_sum += prime_number.next() * t.size();

        // Iterate over components
        
        for(auto & [n,c] : components)      
            c->CalculateCheckSum(calculated_check_sum, prime_number);
        if(correct_check_sum == calculated_check_sum)
            std::cout << "Correct Check Sum: " << calculated_check_sum << '\n';
        else
        {
            const std::string msg = "Incorrect Check Sum: " +
                                    std::to_string(calculated_check_sum) + " != " +
                                    std::to_string(correct_check_sum);
            if(info_.is_set("batch_mode"))
                throw setup_failed(msg);
            Notify(msg_fatal_error, msg);
        }
    }


    dictionary 
    Kernel::GetModuleInstantiationInfo()
    {
        dictionary d;
        std::map<std::string, int> class_counts;
        int module_count = 0;

        for(const auto & [path, component] : components)
        {
            (void)path;
            if(component == nullptr)
                continue;
            if(dynamic_cast<Group *>(component.get()) != nullptr)
                continue;

            std::string class_name = component->info_.contains_non_null("class") ? std::string(component->info_["class"]) : "";
            if(class_name.empty())
                class_name = component->Info();
            if(class_name.empty())
                class_name = "unknown";

            class_counts[class_name]++;
            module_count++;
        }

        std::vector<std::string> summary_entries;
        summary_entries.reserve(class_counts.size());
        for(const auto & [class_name, count] : class_counts)
            summary_entries.push_back(class_name + ":" + std::to_string(count));

        d["module_count"] = module_count;
        d["class_count"] = static_cast<int>(class_counts.size());
        d["classes"] = join(",", summary_entries);
        return d;
     }


    std::string
    Kernel::GetProfilingJSON() const
    {
        std::ostringstream body;
        body << "{";
        body << "\"tick\": " << tick << ", ";
        body << "\"run_mode\": " << run_mode.load() << ", ";
        body << "\"enabled\": "
             << (profiling_enabled.load(std::memory_order_relaxed) ? "true" : "false") << ", ";
        body << "\"components\": [";

        std::string separator;
        for(const auto & [path, component] : components)
        {
            if(component == nullptr)
                continue;

            body << separator;
            body << "{";
            body << "\"path\": \"" << escape_json_string(path) << "\", ";
            body << "\"name\": \"" << escape_json_string(component->Info()) << "\", ";

            std::string class_name;
            if(component->info_.contains_non_null("class"))
                class_name = std::string(component->info_["class"]);

            body << "\"class\": ";
            if(class_name.empty())
                body << "null";
            else
                body << "\"" << escape_json_string(class_name) << "\"";
            body << ", ";
            body << "\"profiling\": " << component->profiler_.json();
            body << "}";
            separator = ", ";
        }

        body << "]";
        body << "}";
        return body.str();
    }


    void
    Kernel::SetProfilingClientActive(long client_id, bool active)
    {
        const auto now = steady_clock::now();
        std::lock_guard<std::mutex> lock(profiling_clients_mutex);

        for(auto it = profiling_clients.begin(); it != profiling_clients.end();)
        {
            if(now - it->second > duration<double>(profiling_subscription_timeout_seconds))
                it = profiling_clients.erase(it);
            else
                ++it;
        }

        if(active)
            profiling_clients[client_id] = now;
        else
            profiling_clients.erase(client_id);

        profiling_enabled.store(!profiling_clients.empty(), std::memory_order_relaxed);
    }


    void
    Kernel::UpdateProfilingState()
    {
        if(!profiling_enabled.load(std::memory_order_relaxed))
            return;

        const auto now = steady_clock::now();
        std::lock_guard<std::mutex> lock(profiling_clients_mutex);
        for(auto it = profiling_clients.begin(); it != profiling_clients.end();)
        {
            if(now - it->second > duration<double>(profiling_subscription_timeout_seconds))
                it = profiling_clients.erase(it);
            else
                ++it;
        }
        profiling_enabled.store(!profiling_clients.empty(), std::memory_order_relaxed);
    }


    std::string
    Kernel::GetStartupStepsJSON() const
    {
        std::ostringstream body;
        body << "{";
        body << "\"tick\": " << tick << ", ";
        body << "\"run_mode\": " << run_mode.load() << ", ";
        body << "\"components\": [";

        std::string separator;
        for(const auto & [path, component] : components)
        {
            if(component == nullptr)
                continue;

            body << separator;
            body << "{";
            body << "\"path\": \"" << escape_json_string(path) << "\", ";
            body << "\"name\": \"" << escape_json_string(component->Info()) << "\", ";

            std::string class_name;
            if(component->info_.contains_non_null("class"))
                class_name = std::string(component->info_["class"]);

            body << "\"class\": ";
            if(class_name.empty())
                body << "null";
            else
                body << "\"" << escape_json_string(class_name) << "\"";
            body << ", ";
            body << "\"module_start\": " << component->module_start << ", ";
            body << "\"start_tick\": " << component->start_tick << ", ";
            body << "\"startup_first_real_input_step\": ";
            if(component->startup_first_real_input_step == std::numeric_limits<int>::max())
                body << "null";
            else
                body << component->startup_first_real_input_step;
            body << ", ";
            body << "\"startup_all_real_inputs_step\": ";
            if(component->startup_all_real_inputs_step == std::numeric_limits<int>::max())
                body << "null";
            else
                body << component->startup_all_real_inputs_step;
            body << "}";
            separator = ", ";
        }

        body << "]";
        body << "}";
        return body.str();
    }


    void 
    Kernel::Save() // Simple save function in present file from kernel data
    {
        std::cout << "ERROR: SAVE SHOULD NEVER BE CALLED\n";

        std::string data = xml();

        //std::cout << data << std::endl;

        std::ofstream file;
        std::string filename = add_extension(info_["filename"], ".ikg");
        file.open (filename);
        file << data;
        file.close();
        //needs_reload = true;
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



    void
    Kernel::SetUp()
    {
        try
        {
            task_timeout = 5.0;
            if(info_.contains_non_null("task_timeout"))
            {
                task_timeout = info_["task_timeout"].as_double();
                if(!std::isfinite(task_timeout) || task_timeout < 0)
                    throw setup_failed("task_timeout must be a finite non-negative number of seconds.");
            }

            PruneConnections();
            SortTasks();
            CalculateStartupSteps();
            ResolveParameters();
            CalculateDelays();
            CalculateSizes();
            ShareZeroDelayConnectionBuffers();

            InitCircularBuffers();
            for(auto & connection : connections)
                connection.ResolveRuntimeState();
            InitComponents();

            if(info_.is_set("info"))
            {
                ListOutputs();
                ListParameters();
                //ListComponents();
                ListConnections();
                ListInputs();
                ListOutputs();
                //ListBuffers();
                //ListCircularBuffers();
                //ListTasks();
            }

            //PrintLog();
        }
        catch(exception & e)
        {
            throw setup_failed("SetUp Failed. "+e.message(), e.path());
        }
        catch(std::exception & e)
        {
            throw setup_failed("SetUp Failed. "+std::string(+e.what()));
        }
    }


    //
    //  Serialization
    //

    std::string 
    Component::json() const
    {
        return info_.json();
    }


    std::string 
    Component::xml()
    {
        return canonicalize_shape_aliases(info_.xml("group"));
    }


    std::string 
    Kernel::json()
    {
        return info_.json();
    }


    std::string 
    Kernel::xml()
    {
        if(components.empty())
            return "";
        else
            return components.begin()->second->xml();
    }

    //
    // WebUI
    //

    std::string
    Kernel::SendImage(const matrix & image, const std::string & format, int quality) // Compress image to jpg and return a base64 data URI
    {
        jpeg_data jpeg;

        if(format=="rgb" && image.rank() == 3 && image.size(0) == 3)
            jpeg = create_color_jpeg(image, quality);

        else if(format=="gray" && image.rank() == 2)
            jpeg = create_gray_jpeg(image, 0, 1, quality);

        else if(image.rank() == 2) // taking our chances with the format...
            jpeg = create_pseudocolor_jpeg(image, 0, 1, format, quality);

        if(jpeg.empty())
            return "\"\"";

        const std::string jpeg_base64 = base64_encode(jpeg.data(), jpeg.size());
        std::string result = "\"data:image/jpeg;base64,";
        result += jpeg_base64;
        result += "\"";
        return result;
    }



    void
    Kernel::Stop()
    {
        notify_stop_requested = false;

        {
            std::lock_guard<std::recursive_mutex> lock(kernelLock);
            run_mode.store(std::min(run_mode_stop, run_mode.load()));
            timer.Pause();
            timer.SetPauseTime(0);
        }

        WaitForAsyncComponents(true);

        {
            std::lock_guard<std::recursive_mutex> lock(kernelLock);
            if(options_.is_explicitly_set("save_state") && !components.empty())
                SaveState(resolve_state_filename(options_, "save_state"));
            tick = 0;
#if !defined(LOGGING_OFF)
            if(session_logging_active)
            {
                LogStop();
                session_logging_active = false;
            }
#endif
            //PrintProfiling(); // FIXME: Use option to turn on and off
            if(!StopComponents())
            {
                int successful_exit = 0;
                process_exit_code.compare_exchange_strong(successful_exit, 1);
            }
        }
    }



    void
    Kernel::Pause()
    {
        if(needs_reload)
        {
            if(GetOptionFilename().empty())
                New();
            else
                LoadFile();
            run_mode = run_mode_pause;
        }
        else
        {
            run_mode = run_mode_pause;
            timer.Pause();
            timer.SetPauseTime(GetTime()+tick_duration);
            WaitForAsyncComponents(false);
        }
    }


    void
    Kernel::Realtime()
    {
        if(needs_reload)
        {
            if(GetOptionFilename().empty())
                New();
            else
                LoadFile();
        }
    
        Pause();
#if !defined(LOGGING_OFF)
        if(!session_logging_active)
        {
            session_timer.Restart();
            LogStart();
            session_logging_active = true;
        }
#endif
        timer.Continue(); 
        run_mode = run_mode_realtime;
    }



    void
    Kernel::Play()
    {
        if(needs_reload)
        {
            if(GetOptionFilename().empty())
                New();
            else
                LoadFile();
        }

#if !defined(LOGGING_OFF)
        if(!session_logging_active)
        {
            session_timer.Restart();
            LogStart();
            session_logging_active = true;
        }
#endif
        run_mode = run_mode_play;
        timer.Continue();
    }
double
Kernel::GetTimeOfDay()
{
    if(active_async_runtime_snapshot)
        return active_async_runtime_snapshot->time_of_day;

    auto now = system_clock::now();
    std::time_t now_time = system_clock::to_time_t(now);
    std::tm now_tm;
    localtime_r(&now_time, &now_tm); // thread-safe localtime
    now_tm.tm_hour = now_tm.tm_min = now_tm.tm_sec = 0;
    auto midnight = system_clock::from_time_t(std::mktime(&now_tm));
    return duration<double>(now - midnight).count();
}


void
Kernel::CalculateCPUUsage() // Fraction of total CPU capacity
{
    const auto sample_time = std::chrono::steady_clock::now();
    struct rusage usage{};
    if(getrusage(RUSAGE_SELF, &usage) != 0)
    {
        cpu_usage = 0;
        cpu_usage_initialized = false;
        return;
    }

    const double user_cpu = double(usage.ru_utime.tv_sec) + double(usage.ru_utime.tv_usec) / 1000000.0;
    const double system_cpu = double(usage.ru_stime.tv_sec) + double(usage.ru_stime.tv_usec) / 1000000.0;
    const double cpu = user_cpu + system_cpu;

    if(!cpu_usage_initialized)
    {
        last_cpu = cpu;
        cpu_usage = 0;
        cpu_usage_initialized = true;
        cpu_usage_sample_time = sample_time;
        return;
    }

    const double wall_time_delta = std::chrono::duration<double>(sample_time - cpu_usage_sample_time).count();
    cpu_usage = CPUUsageFraction(cpu - last_cpu, wall_time_delta, cpu_cores);
    last_cpu = cpu;
    cpu_usage_sample_time = sample_time;
}

InitClass::InitClass(const char * name, ModuleCreator mc)
{
    kernel().RegisterClass(name, mc);
}

Kernel::~Kernel()
{
    StopHTTPServer();
}

}; // namespace ikaros
