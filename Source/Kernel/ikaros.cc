// Ikaros 3.0

#include "ikaros.h"
#include "compute_engine.h"
#include "session_logging.h"

#include <cctype>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <optional>
#include <random>
#include <sstream>
#include <sys/resource.h>

#if __has_include(<CommonCrypto/CommonDigest.h>) && __has_include(<CommonCrypto/CommonHMAC.h>)
#include <CommonCrypto/CommonDigest.h>
#include <CommonCrypto/CommonHMAC.h>
#define IKAROS_HAVE_COMMONCRYPTO 1
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
#if defined(_WIN32)
            gmtime_s(&utc, &now_time);
#else
            gmtime_r(&now_time, &utc);
#endif
            std::ostringstream result;
            result << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
            return result.str();
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

        bool parse_shape_selector(const std::string & function_name, std::string & base_name, std::string & selector)
        {
            const std::size_t bracket = function_name.find('[');
            if(bracket == std::string::npos || function_name.empty() || function_name.back() != ']')
                return false;

            base_name = function_name.substr(0, bracket);
            selector = function_name.substr(bracket + 1, function_name.size() - bracket - 2);
            return base_name == "shape" || base_name == "size";
        }

        bool parse_non_negative_index(const std::string & text, int & value)
        {
            const std::string trimmed = trim(text);
            if(trimmed.empty())
                return false;

            value = 0;
            for(char c : trimmed)
            {
                if(!std::isdigit(static_cast<unsigned char>(c)))
                    return false;
                value = value * 10 + (c - '0');
            }
            return true;
        }

        std::string shape_string(const std::vector<int> & shape)
        {
            std::string result;
            for(size_t i = 0; i < shape.size(); ++i)
            {
                if(i > 0)
                    result += ",";
                result += std::to_string(shape[i]);
            }
            return result;
        }

        std::optional<std::string> matrix_shape_expression_value(const matrix & m, const std::string & function_name, const std::string & path)
        {
            if(m.is_uninitialized())
                return std::nullopt;

            const auto shape = m.shape();

            if(function_name == "size_x")
                return std::to_string(m.size_x());
            if(function_name == "size_y")
                return std::to_string(m.size_y());
            if(function_name == "size_z")
                return std::to_string(m.size_z());
            if(function_name == "rows")
                return std::to_string(m.rows());
            if(function_name == "cols")
                return std::to_string(m.cols());
            if(function_name == "rank")
                return std::to_string(m.rank());
            if(function_name == "size" || function_name == "shape")
                return shape_string(shape);

            std::string base_name;
            std::string selector;
            if(!parse_shape_selector(function_name, base_name, selector))
                return std::nullopt;

            const std::size_t colon = selector.find(':');
            if(colon == std::string::npos)
            {
                int index = 0;
                if(!parse_non_negative_index(selector, index))
                    throw exception("Invalid shape index \"" + function_name + "\".", path);
                if(shape.empty())
                    return "0";
                if(index < 0 || static_cast<std::size_t>(index) >= shape.size())
                    throw exception("Shape index out of range in \"" + function_name + "\".", path);
                return std::to_string(shape[static_cast<std::size_t>(index)]);
            }

            int start = 0;
            int end = static_cast<int>(shape.size());
            const std::string start_text = selector.substr(0, colon);
            const std::string end_text = selector.substr(colon + 1);
            if(!start_text.empty() && !parse_non_negative_index(start_text, start))
                throw exception("Invalid shape slice \"" + function_name + "\".", path);
            if(!end_text.empty() && !parse_non_negative_index(end_text, end))
                throw exception("Invalid shape slice \"" + function_name + "\".", path);

            start = std::clamp(start, 0, static_cast<int>(shape.size()));
            end = std::clamp(end, 0, static_cast<int>(shape.size()));
            if(end < start)
                end = start;
            return shape_string(std::vector<int>(shape.begin() + start, shape.begin() + end));
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
                        || (!std::isalnum(static_cast<unsigned char>(next)) && next != '_');
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

        struct DataSnapshotItem
        {
            std::string prefix;
            std::string value;
        };

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


// CircularBuffer

    CircularBuffer::CircularBuffer(const matrix & m, int size):
        buffer_(),
        index_(0)
    {
        if(size <= 0)
            throw std::invalid_argument("Circular buffer size must be positive");

        buffer_.resize(static_cast<size_t>(size));
        for(int i = 0; i < size; i++)
        {
            if(m.is_dynamic())
            {
                buffer_[i].reserve(m.capacity());
                buffer_[i].set_dynamic().set_fixed_capacity();
                buffer_[i].resize(m.shape());
            }
            else
                buffer_[i].realloc(m.shape());
        }
    }


    int
    CircularBuffer::size() const noexcept
    {
        return static_cast<int>(buffer_.size());
    }


    void 
    CircularBuffer::rotate(const matrix & m)
    {
        if(buffer_.empty())
            throw std::logic_error("Cannot rotate an empty circular buffer");

        matrix & frame = buffer_[index_];
        if(m.is_dynamic() && frame.shape() != m.shape())
            frame.resize(m.shape());
        frame.copy(m);

        ++index_;
        if(index_ == static_cast<int>(buffer_.size()))
            index_ = 0;
    }

    const matrix &
    CircularBuffer::get(int i) const // Get output with delay i
    {
        if(buffer_.empty())
            throw std::logic_error("Cannot read from an empty circular buffer");
        if(i < 1 || i > static_cast<int>(buffer_.size()))
            throw std::out_of_range("Circular buffer delay is outside its history");

        int position = index_ - i;
        if(position < 0)
            position += static_cast<int>(buffer_.size());
        return buffer_[position];
    }



// Parameter

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
                state_->value = std::to_string(v);
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
            ApplyPendingAsyncActions();
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

        async_started_tick = kernel().GetTick();
        async_running = true;
        try
        {
            async_future = std::async(std::launch::async, [this]() -> std::exception_ptr
            {
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
    AsyncParameterChangeKey(const Component::DeferredParameterChange & change)
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
    Component::ResolveParameter(parameter & p,  std::string & name)
    {
        if(p.is_resolved())
            return true; // Already set from SetParameters

        auto resolve_value = [&](const std::string & raw_value, Component * owner)
        {
            Component * context = owner ? owner : this;

            auto matrix_parameter_shape_expression = [&]() -> std::string
            {
                if(p.metadata().contains_non_null("shape"))
                    return std::string(p.metadata()["shape"]);
                if(p.metadata().contains_non_null("size"))
                    return std::string(p.metadata()["size"]);
                return "";
            };

            auto apply_sized_matrix_parameter = [&](const matrix & value) -> matrix
            {
                std::string shape_expr = matrix_parameter_shape_expression();
                if(shape_expr.empty())
                    return value;

                std::vector<int> shape = EvaluateShapeList(shape_expr);
                if(shape.empty())
                    throw exception("Matrix parameter shape \"" + shape_expr + "\" did not resolve to a valid shape.");

                matrix shaped(shape);
                if(!value.is_uninitialized() && !value.empty())
                {
                    if(value.size() > shaped.size())
                        throw exception("Matrix parameter value has " + std::to_string(value.size()) +
                            " elements but shape \"" + shape_expr + "\" only allows " + std::to_string(shaped.size()) + ".");

                    float * target = shaped.contiguous_data();
                    int target_index = 0;
                    for(int block = 0; block < value.logical_block_count(); ++block)
                    {
                        std::copy_n(value.logical_block_data(block), value.logical_block_size(), target + target_index);
                        target_index += value.logical_block_size();
                    }
                }
                return shaped;
            };

            if((p.get_type() == number_type || p.get_type() == rate_type) && !p.has_options())
            {
                SetParameter(name, formatNumber(context->ComputeDouble(raw_value)));
                return;
            }

            if(p.get_type() == matrix_type)
            {
                matrix literal;
                if(raw_value.empty() && !matrix_parameter_shape_expression().empty())
                    SetParameter(name, apply_sized_matrix_parameter(matrix()), raw_value);
                else if(try_parse_matrix_literal(literal, raw_value))
                    SetParameter(name, apply_sized_matrix_parameter(literal), raw_value);
                else
                    SetParameter(name, apply_sized_matrix_parameter(matrix(context->ComputeValue(raw_value))), raw_value);
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
        };

        try
        {
            // Look for binding
            std::string bind_to = GetBind(name);
            if(!bind_to.empty())
            {
                if(LookupParameter(p, bind_to))
                    return true;
            }

            const Component * value_owner = GetValueOwner(name);
            bool has_explicit_value = value_owner && value_owner->info_.contains_non_null(name);
            if(!has_explicit_value)
            {
                if(!p.metadata().contains("default"))
                {
                    if(p.get_type() == matrix_type &&
                       (p.metadata().contains_non_null("size") || p.metadata().contains_non_null("shape")))
                    {
                        resolve_value("", this);
                        return true;
                    }

                    Error("Parameter \""+name+"\" has no default value in the ikc file.");   
                    return false;
                }

                resolve_value(std::string(p.metadata()["default"]), this);
                return true;
            }

            resolve_value(std::string(value_owner->info_[name]), const_cast<Component *>(value_owner));
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
        return std::stoi(value);
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


    void Component::Bind(float & v, std::string n)
    {
        std::string name = path_+"."+n;
        Kernel & k = kernel();
        auto it = k.scalar_states.find(name);
        if(it == k.scalar_states.end())
            throw exception("Bind:\"" + name + "\" failed. Scalar state does not exist.", path_);
        if(it->second.type != "float")
            throw exception("Bind:\"" + name + "\" failed. Expected state type float but got " + it->second.type + ".", path_);
        v = it->second.float_value;
        it->second.float_ptr = &v;
    }


    void Component::Bind(double & v, std::string n)
    {
        std::string name = path_+"."+n;
        Kernel & k = kernel();
        auto it = k.scalar_states.find(name);
        if(it == k.scalar_states.end())
            throw exception("Bind:\"" + name + "\" failed. Scalar state does not exist.", path_);
        if(it->second.type != "double")
            throw exception("Bind:\"" + name + "\" failed. Expected state type double but got " + it->second.type + ".", path_);
        v = it->second.double_value;
        it->second.double_ptr = &v;
    }


    void Component::Bind(int & v, std::string n)
    {
        std::string name = path_+"."+n;
        Kernel & k = kernel();
        auto it = k.scalar_states.find(name);
        if(it == k.scalar_states.end())
            throw exception("Bind:\"" + name + "\" failed. Scalar state does not exist.", path_);
        if(it->second.type != "int")
            throw exception("Bind:\"" + name + "\" failed. Expected state type int but got " + it->second.type + ".", path_);
        v = it->second.int_value;
        it->second.int_ptr = &v;
    }


    void Component::Bind(bool & v, std::string n)
    {
        std::string name = path_+"."+n;
        Kernel & k = kernel();
        auto it = k.scalar_states.find(name);
        if(it == k.scalar_states.end())
            throw exception("Bind:\"" + name + "\" failed. Scalar state does not exist.", path_);
        if(it->second.type != "bool")
            throw exception("Bind:\"" + name + "\" failed. Expected state type bool but got " + it->second.type + ".", path_);
        v = it->second.bool_value;
        it->second.bool_ptr = &v;
    }


    void Component::Bind(std::string & v, std::string n)
    {
        std::string name = path_+"."+n;
        Kernel & k = kernel();
        auto it = k.scalar_states.find(name);
        if(it == k.scalar_states.end())
            throw exception("Bind:\"" + name + "\" failed. Scalar state does not exist.", path_);
        if(it->second.type != "string")
            throw exception("Bind:\"" + name + "\" failed. Expected state type string but got " + it->second.type + ".", path_);
        v = it->second.string_value;
        it->second.string_ptr = &v;
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

    void Component::ClearOutputs()
    {
       info_["outputs"] = list(); 
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
        std::vector<int> shape;
        auto resolve_matrix_size_function = [&](const std::string & token) -> std::optional<std::string>
        {
            static const std::vector<std::string> size_functions = {"size_x", "size_y", "size_z", "rows", "cols", "size", "shape", "rank"};
            std::string function_name;
            std::string matrix_path;

            const std::size_t dot = token.rfind('.');
            if(dot == std::string::npos || dot == token.size() - 1)
                return std::nullopt;

            function_name = token.substr(dot + 1);
            matrix_path = token.substr(0, dot);

            bool recognized = std::find(size_functions.begin(), size_functions.end(), function_name) != size_functions.end();
            if(!recognized)
            {
                std::string base_name;
                std::string selector;
                recognized = parse_shape_selector(function_name, base_name, selector);
            }

            if(!recognized || matrix_path.empty())
                return std::nullopt;

            Component * current = this;
            std::string local_path = matrix_path;
            if(!local_path.empty() && local_path[0] == '.')
            {
                std::string root_path = peek_head(path_, ".");
                current = kernel().components.at(root_path).get();
                local_path = local_path.substr(1);
            }

            const auto & segments = ComputeEngine::SplitTopLevel(local_path, '.');
            if(segments.empty())
                return std::nullopt;

            for(size_t i = 0; i + 1 < segments.size(); ++i)
                current = current->GetComponent(segments[i]);

            matrix m;
            current->Bind(m, segments.back());
            return matrix_shape_expression_value(m, function_name, path_);
        };
        auto resolve_parameter_for_shape = [&](const std::string & token) -> std::optional<std::string>
        {
            if(token.empty() || token[0] != '@')
                return std::nullopt;

            const std::string parameter_name = token.substr(1);
            if(parameter_name.empty() || parameter_name.find('.') != std::string::npos || parameter_name.find('@') != std::string::npos)
                return std::nullopt;

            parameter p;
            if(!LookupParameter(p, parameter_name))
                return std::nullopt;

            if(!p.is_resolved())
            {
                std::string local_parameter_name = parameter_name;
                if(!ResolveParameter(p, local_parameter_name))
                    return std::nullopt;
            }

            if(p.has_options() && (p.get_type() == number_type || p.get_type() == rate_type))
                return p.as_int_string();
            if(p.get_type() == number_type || p.get_type() == rate_type)
                return formatNumber(p.as_double());
            if(p.get_type() == bool_type)
                return p.as_bool() ? "1" : "0";

            return std::nullopt;
        };
        auto unwrap_optional_dimension = [](std::string & item)
        {
            const std::string optional_prefix = "optional(";
            if(item.size() <= optional_prefix.size() || !starts_with(item, std::string(optional_prefix)) || item.back() != ')')
                return false;

            int depth = 0;
            for(size_t i = 0; i < item.size(); ++i)
            {
                if(item[i] == '(')
                    ++depth;
                else if(item[i] == ')')
                {
                    --depth;
                    if(depth == 0 && i != item.size() - 1)
                        return false;
                }
            }

            item = trim(item.substr(optional_prefix.size(), item.size() - optional_prefix.size() - 1));
            return true;
        };

        for(std::string e : ComputeEngine::SplitTopLevel(s, ','))
        {
            e = trim(e);
            if(e.empty())
                continue;

            bool optional_dimension = unwrap_optional_dimension(e);
            if(optional_dimension && e.empty())
                throw std::invalid_argument("optional() requires a size expression.");

            bool unresolved_variable = false;
            expression expr(e);
            std::map<std::string, std::string> replacements;
            for(const auto & var : expr.variables())
            {
                if(auto replacement = resolve_matrix_size_function(var))
                    replacements[var] = *replacement;
                else if(!var.empty() && var[0] == '@')
                {
                    std::optional<std::string> shape_parameter = resolve_parameter_for_shape(var);
                    std::string replacement = shape_parameter ? *shape_parameter : ComputeValue(var);
                    if(replacement == "true")
                        replacement = "1";
                    else if(replacement == "false")
                        replacement = "0";
                    replacements[var] = replacement;
                }
                else
                    unresolved_variable = true;
            }
            if(unresolved_variable)
                return {};

            std::string rewritten = expr.substitute(replacements);

            bool purely_numeric_expression = std::none_of(rewritten.begin(), rewritten.end(), [](unsigned char c)
            {
                return std::isalpha(c) || c == '@' || c == '_';
            });
            bool contains_top_level_comma = ComputeEngine::SplitTopLevel(rewritten, ',').size() > 1;

            std::string computed;
            if(purely_numeric_expression && !contains_top_level_comma)
                computed = formatNumber(expression(rewritten).evaluate());
            else if(purely_numeric_expression)
                computed = rewritten;
            else
                computed = ComputeValue(rewritten);
            if(computed.find(';') != std::string::npos)
                throw std::invalid_argument("Size expression \""+e+"\" evaluated to a matrix.");

            bool had_dimension = false;
            for(std::string item : ComputeEngine::SplitTopLevel(computed, ','))
            {
                item = trim(item);
                if(item.empty())
                    continue;
                if(optional_dimension && had_dimension)
                    throw std::invalid_argument("optional() must resolve to a single dimension.");
                had_dimension = true;

                int d = ComputeInt(item);
                if(d < 0)
                    return {};
                if(d == 0 && optional_dimension)
                    continue;
                if(d == 0)
                    return {};
                shape.push_back(d);
            }
            if(optional_dimension && !had_dimension)
                return {};
        }
        return shape;
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

    void
    add_async_parameter_if_missing(dictionary & info)
    {
        if(info["parameters"].is_null())
            info["parameters"] = list();

        for(auto p: info["parameters"])
            if(p["name"].as_string()=="async")
                return;

        info["parameters"].push_back(make_async_parameter().copy());
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
        add_async_parameter_if_missing(info_);

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

    Module::~Module()
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


    bool
    Component::InputsReady(dictionary d,  input_map ingoing_connections)
    {
        if(d.contains("size"))
            return true;

        if(d.contains("inputs") && d["inputs"].is_list())
            return true;

        std::string full_name = path_ + "." + std::string(d["name"]);
        Trace("\t\t\tComponent::InputReady", full_name);
        Kernel& k = kernel();

        if(!ingoing_connections.count(full_name))
            return d.is_set("optional");

        for(auto & c : ingoing_connections.at(full_name))
            if(k.buffers.at(c->source).rank()==0)
                return false;
        return true;
    }


    int 
    Component::SetInputShape_Flat(dictionary d, input_map ingoing_connections)
    {
        Trace("\t\t\t\t\tComponent::SetInputShape_Flat", path_);

        std::string name = d.at("name");
        std::string full_name = path_ +"."+ name;
        bool has_fixed_size = d.contains("size");

        auto shape_string = [](const std::vector<int> & shape) -> std::string
        {
            std::vector<std::string> parts;
            parts.reserve(shape.size());
            for(int dim : shape)
                parts.push_back(std::to_string(dim));
            return join(",", parts, false);
        };

        auto validate_fixed_target = [&](const Connection & connection, const range & target_range)
        {
            const matrix & input_buffer = kernel().buffers.at(full_name);
            const std::vector<int> & shape = input_buffer.shape();
            if(input_buffer.rank() != 1)
                throw setup_failed("Input \"" + name + "\" in \"" + path_ + "\" uses flatten and must have a one-dimensional fixed size, got \"" + shape_string(shape) + "\".", path_);

            if(target_range.rank() != 1 || target_range.step(0) <= 0 ||
               target_range.start(0) < 0 || target_range.stop(0) > input_buffer.size())
                throw setup_failed("Connection \"" + connection.Info() + "\" writes outside fixed size of input \"" + name + "\" in \"" + path_ + "\" (" + shape_string(shape) + ").", path_);
        };

        if(!ingoing_connections.count(full_name)) // Not connected
            return has_fixed_size ? 0 : 1;

        if(has_fixed_size)
        {
            std::string shape_expr = std::string(d.at("size"));
            if(shape_expr.empty())
                throw setup_failed("Input \""+name+"\" must have a value for \"size\".", path_);
            std::vector<int> shape = EvaluateShapeList(shape_expr);
            kernel().buffers[full_name].realloc(shape);
        }

        int begin_index = 0;
        int end_index = 0;
        int flattened_input_size = 0;
        for(auto & c : ingoing_connections.at(full_name))
        {
            c->flatten_ = true;

            matrix & output_buffer = kernel().buffers[c->source];
            if(output_buffer.is_dynamic())
                throw setup_failed("Connection \"" + c->Info() + "\" can not flatten dynamic output \"" + c->source + "\".", path_);

            range output_matrix = output_buffer.get_range();
            c->Resolve(output_matrix);  //**NEW  

            const long long required_size = static_cast<long long>(c->source_range.size()) * c->DelayCount();
            if(required_size > std::numeric_limits<int>::max())
                throw setup_failed("Connection \"" + c->Info() + "\" requires an input larger than the supported size.", path_);
            int s = static_cast<int>(required_size);
            end_index = begin_index + s;
            c->target_range = range(begin_index, end_index);
            if(has_fixed_size)
                validate_fixed_target(*c, c->target_range);
            begin_index += s;
            flattened_input_size += s;
        }
    
        if(!has_fixed_size && flattened_input_size != 0)
        {
            kernel().buffers[full_name].realloc(flattened_input_size); 
          Trace("\t\t\tComponent::SetInputShape_Index Alloc "+std::to_string(flattened_input_size), path_);
        }

        if(d.is_set("use_label"))
        {
            begin_index = 0;
            for(auto & c : ingoing_connections.at(full_name))
            {
                const long long required_size = static_cast<long long>(c->source_range.size()) * c->DelayCount();
                if(required_size > std::numeric_limits<int>::max())
                    throw setup_failed("Connection \"" + c->Info() + "\" requires an input larger than the supported size.", path_);
                int s = static_cast<int>(required_size);
                if(c->label_.empty())
                    kernel().buffers[full_name].push_label(0, c->source, s); // WAS: kernel().buffers[d.at(full_name)].push_label(0, c->source, s);
                else
                    kernel().buffers[full_name].push_label(0, c->label_, s); // WAS: kernel().buffers[d.at(full_name)].push_label(0, c->label_, s);
            }
        }
        return 0;
    }


    int 
    Component::SetInputShape_Index(dictionary d, input_map ingoing_connections)
    {
       Trace("\t\t\tComponent::SetInputShape_Index ", path_ + "." + std::string(d["name"]));

        range input_size;
        std::string name = d.at("name");
        std::string full_name = path_ +"."+ name;
        bool has_fixed_size = d.contains("size");
        bool stack = ComputeAttributeBool(d, "stack");

        auto shape_string = [](const std::vector<int> & shape) -> std::string
        {
            std::vector<std::string> parts;
            parts.reserve(shape.size());
            for(int dim : shape)
                parts.push_back(std::to_string(dim));
            return join(",", parts, false);
        };

        auto validate_fixed_target = [&](const Connection & connection, const range & target_range)
        {
            const matrix & input_buffer = kernel().buffers.at(full_name);
            const std::vector<int> & shape = input_buffer.shape();
            if(target_range.rank() != input_buffer.rank())
                throw setup_failed("Connection \"" + connection.Info() + "\" writes outside fixed size of input \"" + name + "\" in \"" + path_ + "\" (" + shape_string(shape) + ").", path_);

            for(int i = 0; i < target_range.rank(); ++i)
                if(target_range.step(i) == 0 || target_range.start(i) < 0 ||
                   target_range.start(i) > target_range.stop(i) ||
                   target_range.stop(i) > input_buffer.size(i))
                    throw setup_failed("Connection \"" + connection.Info() + "\" writes outside fixed size of input \"" + name + "\" in \"" + path_ + "\" (" + shape_string(shape) + ").", path_);
        };

        if(!ingoing_connections.count(full_name)) // Not connected
            return 1;

        auto set_input_label = [&]()
        {
            if(!d.is_set("use_label"))
                return;

            const auto & connections = ingoing_connections.at(full_name);
            if(connections.size() == 1 && !connections[0]->label_.empty())
                kernel().buffers.at(full_name).set_name(connections[0]->label_);
        };

        if(has_fixed_size)
        {
            std::string shape_expr = std::string(d.at("size"));
            if(shape_expr.empty())
                throw setup_failed("Input \""+name+"\" must have a value for \"size\".", path_);
            std::vector<int> shape = EvaluateShapeList(shape_expr);
            kernel().buffers[full_name].realloc(shape);
        }

        // Handle stacked inputs by assigning each connection to one slice along a new first dimension.

        if(stack)
        {
            auto connections = ingoing_connections.at(full_name);
            int target_rank = 0;
            if(has_fixed_size)
                target_rank = kernel().buffers[full_name].rank();

            for(int stack_index = 0; stack_index < static_cast<int>(connections.size()); ++stack_index)
            {
                Connection * c = connections[stack_index];
                matrix & output_buffer = kernel().buffers[c->source];
                if(output_buffer.is_dynamic())
                    throw setup_failed("Connection \"" + c->Info() + "\" can not feed stacked input \"" + name + "\" from dynamic output \"" + c->source + "\".", path_);

                if(!c->stacked_)
                {
                    range output_matrix = output_buffer.get_range();
                    if(output_matrix.rank() == 0)
                        return 0;

                    range resolved_target = c->Resolve(output_matrix);
                    resolved_target.push_front(stack_index, stack_index + 1);
                    c->target_range = resolved_target;
                    c->stacked_ = true;
                }
                target_rank = std::max(target_rank, c->target_range.rank());
            }

            if(target_rank == 0)
                return 0;

            for(Connection * c : connections)
            {
                while(c->target_range.rank() < target_rank)
                    c->target_range.push(0, 1);

                if(has_fixed_size)
                    validate_fixed_target(*c, c->target_range);
                else if(input_size.rank() == 0)
                    input_size = c->target_range;
                else
                    input_size.extend(c->target_range);
            }

            if(!has_fixed_size)
            {
                kernel().buffers[full_name].realloc(input_size.extent());
                Trace("\t\t\tComponent::SetInputShape Stacked Alloc" + std::string(input_size), full_name);
            }

            set_input_label();
            return 1;
        }

        // Handle single connection without inidices - do not collapse dimensions

        auto & input_connections = ingoing_connections.at(full_name);
        Connection * single_connection = input_connections.size() == 1 ? input_connections[0] : nullptr;
        bool old_style_simple_connection =
            !has_fixed_size &&
            ingoing_connections.size() == 1 &&
            ingoing_connections.begin()->second[0]->DelayCount() == 1 &&
            ingoing_connections.begin()->second[0]->source_range.rank() == 0 &&
            ingoing_connections.begin()->second[0]->target_range.rank() == 0;
        bool dynamic_simple_connection =
            !has_fixed_size &&
            single_connection != nullptr &&
            single_connection->DelayCount() == 1 &&
            kernel().buffers[single_connection->source].is_dynamic() &&
            single_connection->IsWholeMatrixConnection();

        if(old_style_simple_connection || dynamic_simple_connection)
        {
            Connection * connection = old_style_simple_connection ?
                                      ingoing_connections.begin()->second[0] : single_connection;
            matrix & output_buffer = kernel().buffers[connection->source];
            range output_matrix = output_buffer.get_range();
            if(output_matrix.rank() == 0)
                return 0;

            if(output_buffer.is_dynamic())
            {
                kernel().buffers[full_name].reserve(output_buffer.capacity());
                kernel().buffers[full_name].set_dynamic().set_fixed_capacity();
                kernel().buffers[full_name].resize(output_buffer.shape());
            }
            else
                kernel().buffers[full_name].realloc(output_matrix.extent());

            Trace("\t\t\tComponent::SetInputShape Simple Alloc" + std::string(input_size), full_name);

            set_input_label();
            return 1;
        }

        for(auto c : ingoing_connections.at(full_name))
        {
            matrix & output_buffer = kernel().buffers[c->source];
            if(output_buffer.is_dynamic())
            {
                if(c->IsWholeMatrixConnection() && c->DelayCount() > 1)
                    throw setup_failed("Connection \"" + c->Info() +
                                       "\" requests multiple delay values from dynamic output \"" +
                                       c->source +
                                       "\". Dynamic outputs support only a single whole-matrix delay.",
                                       path_);
                throw setup_failed("Connection \"" + c->Info() +
                                   "\" uses an indexed or ranged connection from dynamic output \"" +
                                   c->source +
                                   "\". Dynamic outputs only support whole-matrix connections.",
                                   path_);
            }

            range output_matrix = output_buffer.get_range();
            if(output_matrix.rank() == 0)
                return 0;
            range resolved_target = c->Resolve(output_matrix);
            if(has_fixed_size)
                validate_fixed_target(*c, resolved_target);
            else
                input_size.extend(resolved_target);
        }
        if(!has_fixed_size)
        {
            kernel().buffers[full_name].realloc(input_size.extent());
            Trace("\t\t\tComponent::SetInputShape Alloc" + std::string(input_size), full_name);
        }

        set_input_label();

        return 1;
    }


// ****************************** COMPONENT Sizes ******************************


    int 
    Component::SetInputSize(dictionary d, input_map ingoing_connections)
    {
        Trace("\t\t\tComponent::SetInputSize ", path_ + "."+ std::string(d["name"]));

        if(d.is_set("flatten"))
            SetInputShape_Flat(d, ingoing_connections);
        else
            SetInputShape_Index(d, ingoing_connections);
        return 0;
    }



    int
    Component:: SetInputSizes(input_map ingoing_connections)
    {
        Kernel& k = kernel();

        Trace("\t\tComponent::SetInputSizes", path_);

        // Set input sizes (if possible)

        for(auto d : info_["inputs"])
        {
            dictionary input = d;
            std::string full_name = path_+"."+std::string(input["name"]);
            bool has_fixed_size = input.contains("size");
            if(has_fixed_size || k.buffers[full_name].is_uninitialized())
                if(InputsReady(input, ingoing_connections))
                    SetInputSize(input, ingoing_connections);
        }
        return 0;
    }


    int 
    Component::SetOutputShape(dictionary d, input_map ingoing_connections)
    {
       Trace("\t\t\tComponent::SetOutputShape " , path_ + "." + std::string(d["name"]));

        if(d.contains_non_null("alias"))
            return 0;

        if(d.contains("size"))
            throw setup_failed(u8"Output \""+std::string(d["name"])+"\"+in group \""+path_+"\" can not have size attribute.", path_);

        if(d.is_set("dynamic"))
            throw setup_failed("Group output \"" + std::string(d["name"]) + "\" in \"" + path_ + "\" can not be dynamic.", path_);

        range output_range;
        std::string name = d.at("name");
        std::string full_name = path_ +"."+ name;

        if(!ingoing_connections.count(full_name))
            return 0;

        for(auto c : ingoing_connections.at(full_name))
        {
            matrix & output_buffer = kernel().buffers[c->source];
            if(output_buffer.is_dynamic())
                throw setup_failed("Connection \"" + c->Info() + "\" can not map dynamic output \"" + c->source + "\" through group output \"" + full_name + "\".", path_);

            range output_matrix = output_buffer.get_range();
            
            if(output_matrix.rank() == 0)
                return 0;
            
            output_range.extend(c->Resolve(output_matrix));
        }
        kernel().buffers[full_name].realloc(output_range);
      Trace("\t\t\t\t\tComponent:: Alloc" + std::string(output_range), path_);

        return 1;
    }


    int
    Component::ApplyOutputAliases()
    {
        Kernel & k = kernel();

        for(auto & output_value : info_["outputs"])
        {
            dictionary d = output_value;
            if(!d.contains_non_null("alias"))
                continue;

            std::string output_name = d["name"].as_string();
            std::string full_output_name = path_ + "." + output_name;
            std::string alias_spec = trim(d["alias"].as_string());

            if(alias_spec.empty())
                throw setup_failed("Output \"" + output_name + "\" has an empty alias.", path_);

            if(d.contains("size") || d.contains("shape"))
                throw setup_failed("Aliased output \"" + output_name + "\" can not also specify a size or shape.", path_);

            std::string alias_source_name = peek_head(alias_spec, "[");
            std::string alias_selector = peek_tail(alias_spec, "[", true);
            if(alias_source_name.empty())
                throw setup_failed("Output \"" + output_name + "\" has malformed alias \"" + alias_spec + "\".", path_);

            if(alias_source_name.find('.') == std::string::npos)
                alias_source_name = path_ + "." + alias_source_name;

            if(alias_source_name == full_output_name)
                throw setup_failed("Output \"" + output_name + "\" can not alias itself.", path_);

            if(!k.buffers.count(alias_source_name))
                throw setup_failed("Output \"" + output_name + "\" aliases unknown output \"" + alias_source_name + "\".", path_);

            matrix aliased_output = k.buffers[alias_source_name];
            if(aliased_output.is_uninitialized())
                return 0;

            try
            {
                if(!alias_selector.empty())
                {
                    range selector(alias_selector);
                    for(int i = 0; i < selector.rank(); ++i)
                    {
                        bool is_single_index = !selector.empty(i) && selector.step(i) == 1 &&
                                               selector.stop(i) == selector.start(i) + 1;
                        if(!is_single_index)
                            throw setup_failed("Output \"" + output_name + "\" alias must use single indices only.", path_);

                        if(aliased_output.rank() == 0)
                            throw setup_failed("Output \"" + output_name + "\" alias indexes deeper than its source output.", path_);

                        aliased_output = aliased_output[selector.start(i)];
                    }
                }
            }
            catch(const setup_failed &)
            {
                throw;
            }
            catch(const std::exception & e)
            {
                throw setup_failed("Output \"" + output_name + "\" has invalid alias \"" + alias_spec + "\". " + std::string(e.what()), path_);
            }

            aliased_output.set_name(output_name);
            k.buffers[full_output_name] = aliased_output;
        }

        return 1;
    }


    int 
    Component::SetOutputShapes(input_map ingoing_connections)
    {
        Trace("\t\tComponent::SetOutputShapes", path_);
        for(auto & d : info_["outputs"])
            SetOutputShape(d, ingoing_connections);
        ApplyOutputAliases();

        return 0;
    }


    int
    Component::SetStateShape(dictionary d)
    {
        Trace("\t\t\tComponent::SetStateShape ", path_ + "." + std::string(d["name"]));

        if(d.contains_non_null("type") && std::string(d["type"]) != "matrix")
            return 0;

        if(!d.contains_non_null("type") || std::string(d["type"]) != "matrix")
            throw setup_failed("State \"" + std::string(d["name"]) + "\" in \"" + path_ + "\" must have type=\"matrix\" in this implementation.", path_);

        std::string shape_expr;
        if(d.contains("size"))
            shape_expr = std::string(d.at("size"));
        else if(d.contains("shape"))
            shape_expr = std::string(d.at("shape"));
        else
            throw setup_failed("State \"" + std::string(d["name"]) + "\" in \"" + path_ + "\" must have a value for \"size\" or \"shape\".", path_);

        if(shape_expr.empty())
            throw setup_failed("State \"" + std::string(d["name"]) + "\" in \"" + path_ + "\" must have a value for \"size\" or \"shape\".", path_);

        try
        {
            std::vector<int> shape = EvaluateShapeList(shape_expr);
            matrix state;
            Bind(state, d.at("name"));
            state.realloc(shape);
        }
        catch(const std::invalid_argument & e)
        {
            Notify(msg_warning, e.what());
            throw setup_failed("Size expression for state \"" + std::string(d["name"]) + "\" is invalid. " + e.what(), path_);
        }
        catch(const std::exception & e)
        {
            throw setup_failed("Size expression for state \"" + std::string(d["name"]) + "\" is invalid. " + std::string(e.what()), path_);
        }

        return 0;
    }


    int
    Component::SetStateShapes(input_map)
    {
        Trace("\t\tComponent::SetStateShapes", path_);
        for(auto & d : info_["states"])
            SetStateShape(d);

        return 0;
    }


    int
    Component::SetSizes(input_map ingoing_connections)
    {
        
        Trace("\tComponent::SetSizes",path_);
        SetInputSizes(ingoing_connections);
        SetOutputShapes(ingoing_connections);
        SetStateShapes(ingoing_connections);

        return 0;
    }


    void
    Component::CheckRequiredInputs()
    {
        Kernel & k = kernel();
        for(dictionary d : info_["inputs"])
        if(!d.is_set("optional") && k.buffers[path_+"."+d["name"].as_string()].is_uninitialized())
        {
            // Unconnected group inputs that are never referenced internally are harmless.
            if(dynamic_cast<Group *>(this) != nullptr)
            {
                std::string full_input_name = path_+"."+d["name"].as_string();
                bool consumed_inside_group = false;
                for(auto & c : k.connections)
                    if(c.source == full_input_name)
                    {
                        consumed_inside_group = true;
                        break;
                    }
                if(!consumed_inside_group)
                    continue;
            }

            throw setup_failed("Component \""+info_["name"].as_string()+"\" has required input \""+d["name"].as_string()+"\" that is not connected.", path_);
        }
    }



// ****************************** MODULE Sizes ******************************

    int 
    Module::SetOutputShape(dictionary d, input_map)
    {
        try
        {
            if(d.contains_non_null("alias"))
                return 0;

            bool dynamic_output = d.is_set("dynamic");
            if(dynamic_output && !d.contains("capacity"))
                throw setup_failed("Dynamic output \""+std::string(d.at("name")) +"\" must have a capacity attribute.", path_);

            std::string shape_expr;
            if(dynamic_output)
                shape_expr = std::string(d.at("capacity"));
            else if(d.contains("size"))
                shape_expr = std::string(d.at("size"));
            else if(d.contains("shape"))
                shape_expr = std::string(d.at("shape"));
            else if(info_.contains("size"))
                shape_expr = std::string(info_.at("size"));
            else
                throw setup_failed("Output \""+std::string(d.at("name")) +"\" must have a value for \"size\" or \"shape\".", path_);
            
            if(shape_expr.empty())
                throw setup_failed("Output \""+std::string(d.at("name")) +"\" must have a value for \"size\" or \"shape\".", path_);
            std::vector<int> shape = EvaluateShapeList(shape_expr);
            matrix o;
            Bind(o, d.at("name"));
            if(dynamic_output)
            {
                if(shape.empty())
                    throw setup_failed("Dynamic output \""+std::string(d.at("name")) +"\" capacity must have at least one dimension.", path_);
                o.reserve(shape);
                o.set_dynamic().set_fixed_capacity();
            }
            else
                o.realloc(shape);
            return 0;
        }
        catch(const std::invalid_argument & e)
        {
            Notify(msg_warning, e.what());
            throw setup_failed("Size expression for output \""+std::string(d.at("name")) +"\" is invalid. "+e.what(), path_);
        }
        catch(const std::exception & e)
        {
            throw setup_failed("Size expression for output \""+std::string(d.at("name")) +"\" is invalid. "+std::string(e.what()), path_);
        }
        catch(...)
        {
            throw setup_failed("Size expression for output \""+std::string(d.at("name")) +"\" is invalid.", path_);
        }
    }


    int 
    Module::SetOutputShapes(input_map ingoing_connections)
    {
        if(!InputsReady(info_, ingoing_connections))
            return 0; // Cannot set size yet

        for(auto & d : info_["outputs"])
            SetOutputShape(d, ingoing_connections);
        ApplyOutputAliases();

        return 0;
    }


    int
    Module::SetStateShapes(input_map ingoing_connections)
    {
        if(!InputsReady(info_, ingoing_connections))
            return 0;

        for(auto & d : info_["states"])
            SetStateShape(d);

        return 0;
    }


    int 
    Module::SetSizes(input_map ingoing_connections)
    {
        SetInputSizes(ingoing_connections);
        SetOutputShapes(ingoing_connections);
        SetStateShapes(ingoing_connections);
        return 0;
    }   


    void
    Component::CalculateCheckSum(long & check_sum, prime & prime_number) // Calculates a value that depends on all parameters and output sizes; used for integrity testing of kernel and module
    {
        // Iterate over all outputs
        for(auto & d : info_["outputs"])
        {
            matrix output;
            Bind(output, d["name"]);
            for(long d : output.shape())
                check_sum += prime_number.next() * d;
        } 

        // Iterate over all inputs

        for(auto & d : info_["inputs"])
        {
            matrix input;
            Bind(input, d["name"]);
            for(long d : input.shape())
                check_sum += prime_number.next() * d;
        } 


        // Iterate obver all parameters
    
        for(auto & d : info_["parameters"])
        {
            std::string parameter_name = d["name"].as_string();
            if(parameter_name == "log_level" || parameter_name == "module_start" || parameter_name == "start_tick" || parameter_name == "async" || parameter_name == "color" || parameter_name == "rgb_quality" || parameter_name == "gray_quality" || parameter_name == "snapshot_interval" || parameter_name == "webui_req_int" || parameter_name == "webui_log_buffer_limit")
                continue;

            parameter p;
            Bind(p, parameter_name);
            //std::cout << "Parameter: " << d["name"] << std::endl;

            if(p.get_type() == string_type)
                check_sum += prime_number.next() * character_sum(p);
            else
            if(p.get_type() == matrix_type)
            {
                const matrix & matrix_value = p.matrix_ref();
                check_sum += prime_number.next() * matrix_value.size();
            }
            else
                check_sum += prime_number.next() * p.as_long();
        }
        // std::cout << "Check sum: " << check_sum << std::endl;
    }



    // Connection

    Connection::Connection(std::string s, std::string t, range & delay_range, std::string label):
        Task(Task::Kind::connection)
    {
        source = peek_head(s, "[");
        std::string source_selector = peek_tail(s, "[", true);
        source_range = range(source_selector);
        target = peek_head(t, "[");
        std::string target_selector = peek_tail(t, "[", true);
        target_range = range(target_selector);
        delay_range_ = delay_range;
        flatten_ = false;
        label_ = label;
        source_indexed_ = !source_selector.empty();
        target_indexed_ = !target_selector.empty();
        stacked_ = false;
        shared_memory_ = false;
    }


    int
    Connection::DelayCount() const
    {
        return delay_range_.rank() == 0 ? 1 : delay_range_.size();
    }


    int
    Connection::MinDelay() const
    {
        return delay_range_.rank() == 0 ? 1 : delay_range_.a_[0];
    }


    int
    Connection::MaxDelay() const
    {
        if(delay_range_.rank() == 0)
            return 1;
        return delay_range_.a_[0] + (DelayCount() - 1) * delay_range_.inc_[0];
    }


    bool
    Connection::HasZeroDelay() const
    {
        return delay_range_.rank() != 0 && delay_range_.a_[0] == 0;
    }


    bool
    Connection::IsSingleDelay(int delay) const
    {
        if(delay_range_.rank() == 0)
            return delay == 1;
        return delay_range_.a_[0] == delay &&
               static_cast<long long>(delay_range_.a_[0]) + delay_range_.inc_[0] >=
                   delay_range_.b_[0];
    }


    bool
    Connection::UsesCircularBuffer() const
    {
        return !IsSingleDelay(0) && !IsSingleDelay(1);
    }


    bool
    Connection::ShouldTick() const
    {
        if(!has_async_endpoint_)
            return true;
        if(source_component_ != nullptr && source_component_->IsAsyncRunning())
            return false;
        return target_component_ == nullptr ||
               target_component_ == source_component_ ||
               !target_component_->IsAsyncRunning();
    }


    void
    Connection::ResolveRuntimeState()
    {
        auto & k = kernel();
        source_buffer_ = &k.buffers.at(source);
        target_buffer_ = &k.buffers.at(target);
        circular_buffer_ = UsesCircularBuffer() ? &k.circular_buffers.at(source).buffer : nullptr;
        source_component_ = k.ComponentForValuePath(source);
        target_component_ = k.ComponentForValuePath(target);
        has_async_endpoint_ =
            (source_component_ != nullptr && source_component_->async_mode) ||
            (target_component_ != nullptr && target_component_->async_mode);
    }


    range 
    Connection::Resolve(const range & source_output)
    {
        if(source_output.rank() == 0)
            return range();

        auto validate_selector_structure = [&](const range & selector, const std::string & side)
        {
            for(int dimension = 0; dimension < selector.rank(); ++dimension)
            {
                if(selector.is_placeholder(dimension))
                    continue;
                if(selector.step(dimension) == 0)
                    throw exception("Connection " + side +
                                    " selector must not have a zero increment: " + Info());
                if(selector.start(dimension) < 0 ||
                   selector.stop(dimension) < selector.start(dimension))
                    throw exception("Connection " + side +
                                    " selector must have non-negative, ordered bounds: " + Info());
            }
        };
        validate_selector_structure(source_range, "source");
        validate_selector_structure(target_range, "target");

        source_range.extend(source_output.rank());
        source_range.fill(source_output);
        for(int dimension = 0; dimension < source_range.rank(); ++dimension)
            if(source_range.start(dimension) < source_output.start(dimension) ||
               source_range.stop(dimension) > source_output.stop(dimension))
                throw exception("Connection source selector is outside its output bounds: " + Info());

        range reduced_source = source_range.strip().trim();

        if(target_range.rank() == 0)
            target_range = reduced_source;
        else
        {
            int j=0;
            for(int i=0; i<target_range.rank()-1; i++)
                if(target_range.is_placeholder(i) && j<reduced_source.rank())
                {
                    target_range.set(i, reduced_source.start(j),
                                     reduced_source.stop(j), reduced_source.step(j));
                    reduced_source.set(j, 0, 0, 0); // mark as used
                    j++;
                }

            int s = 1;
            for(int i=0; i<reduced_source.rank(); i++)
            {
                int si = reduced_source.size(i);
                s *= (si >0?si:1);
            }

            if(target_range.is_placeholder(target_range.rank()-1) &&
               j<reduced_source.rank())
                target_range.set(target_range.rank()-1, 0, s, 1);
        }
        int delay_size = DelayCount();
        if(delay_size > 1)
            target_range.push_front(0, delay_size);
        const long long source_size = static_cast<long long>(delay_size) * source_range.size();
        if(source_size != target_range.size())
            throw exception("Connection could not be resolved: "+source+"."+std::string(source_range)+"=>"+target+"."+std::string(target_range));

        return target_range;
    }


    bool
    Connection::IsWholeMatrixConnection() const
    {
        return !source_indexed_ && !target_indexed_ && !flatten_ && !stacked_;
    }



    void 
    Connection::Tick()
    {
        if(shared_memory_)
            return;

        if(IsWholeMatrixConnection() && DelayCount() == 1)
        {
            const matrix & sample =
                (IsSingleDelay(0) || IsSingleDelay(1)) ?
                *source_buffer_ : circular_buffer_->get(MinDelay());

            if(target_buffer_->is_dynamic())
                target_buffer_->resize(sample.shape());
            if(target_buffer_->shape() == sample.shape())
            {
                target_buffer_->copy(sample);
                return;
            }
        }

        if(IsSingleDelay(0))
        {
            target_buffer_->copy(*source_buffer_, target_range, source_range);
            //std::cout << source << " =0=> " << target << std::endl; 
        }
        else if(IsSingleDelay(1))
        {
            //std::cout << source << " =1=> " << target << std::endl; 
            target_buffer_->copy(*source_buffer_, target_range, source_range);
        }

        else if(flatten_) // Copy flattened delayed values
        {
            //std::cout << source << " =F=> " << target << std::endl; 
            matrix ctarget = *target_buffer_;
            int target_offset = target_range.a_[0];
            for(auto delay = delay_range_; delay.more(); ++delay)
            {
                const int delay_value = delay.index()[0];
                const matrix & s = delay_value == 0 ? *source_buffer_ :
                                   circular_buffer_->get(delay_value);

                for(auto ix=source_range; ix.more(); ++ix)
                {
                    ctarget(target_offset++) = s.at(ix.index());
                }
            }
        }

        else if(DelayCount() == 1) // Copy indexed delayed value with single delay
        {
            //std::cout << source << " =D=> " << target << std::endl;
            const matrix & s = circular_buffer_->get(MinDelay());
            target_buffer_->copy(s, target_range, source_range);
        }

        else // Copy indexed delayed values with more than one element
        {
            //std::cout << source << " =DD=> " << target << std::endl;
            int target_ix = 0;
            int delay_dimension = stacked_ ? 1 : 0;
            for(auto delay = delay_range_; delay.more(); ++delay, ++target_ix)
            {
                const int delay_value = delay.index()[0];
                const matrix & s = delay_value == 0 ? *source_buffer_ :
                                   circular_buffer_->get(delay_value);
                range tr = target_range;
                tr.set(delay_dimension, target_ix, target_ix + 1, 1);
                target_buffer_->copy(s, tr, source_range);

            }
        }
    };


    void
    Connection::Print() const
    {
        std::cout << "\t" << source <<  delay_range_.curly() <<  std::string(source_range) << " => " << target  << std::string(target_range);
        if(!label_.empty())
            std::cout << " \"" << label_ << "\"";
        std::cout << '\n'; 
    }


std::string
Connection::Info() const
{
    std::string s = source + delay_range_.curly() +  std::string(source_range) + " => " + target  + std::string(target_range);
        if(!label_.empty())
            s+=  " \"" + label_ + "\"";
    return s;
}

// Class
Class::Class() : info_(), module_creator(nullptr), name(), path()
{
}

Class::Class(std::string n, std::string p) : info_(), module_creator(nullptr), name(n), path(p)
    {
        info_.load_xml(p);
    }

    Class::Class(std::string n, ModuleCreator mc) : module_creator(mc), name(n)
    {
    }


    void 
    Class::Print() const
    {
        std::cout << name << ": " << path  << '\n';
    }

    // Request

    Request::Request(std::string uri, long sid, std::string b, std::string content_type, long cid):
        body(b)
    {
        session_id = sid;
        client_id = cid;
        std::string params = tail(uri, "?");
        url = decode_url_component(uri);

        if(!uri.empty() && uri[0] == '/')
            uri.erase(0, 1);
        uri = decode_url_component(uri);
        command = head(uri, "/");
        component_path = uri;
        parameters.parse_url(params);

        if(!body.empty())
        {
            if(!starts_with(content_type, "application/json"))
                throw exception("Request body must have Content-Type application/json.");

            json_body = parse_json(body);
        }
    }

    bool
    Request::HasJsonBody() const
    {
        return !json_body.is_null();
    }

    void
    Request::MergeJsonBodyIntoParameters(bool overwrite)
    {
        if(!HasJsonBody())
            return;
        if(!json_body.is_dictionary())
            throw exception("JSON request body must be an object.");
        parameters.merge(json_body.as_dictionary(), overwrite);
    }

bool operator==(Request & r, const std::string s)
{
    return r.command == s;
}

// Kernel

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


    void
    Kernel::StopComponents()
    {
        WaitForAsyncComponents(true);

        for(auto & [path, component] : components)
        {
            try
            {
                component->Stop();
            }
            catch(const std::exception & e)
            {
                Notify(msg_warning, "Could not stop component \"" + path + "\": " + e.what(), path);
            }
        }
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
        }
        catch(const setup_failed & e)
        {
            Notify(msg_warning, "Could not create new file: " + e.message(), e.path());
            Clear();
            info_ = d;
            run_mode = run_mode_stop;
            needs_reload = true;
        }
        catch(const std::exception & e)
        {
            Notify(msg_warning, "Could not create new file: " + std::string(e.what()));
            Clear();
            info_ = d;
            run_mode = run_mode_stop;
            needs_reload = true;
        }
    }


    bool
    Kernel::Tick()
    {
        UpdateProfilingState();
        tick++;

        PollAsyncComponents();
        if(auto failure = RunTasks())
        {
            Notify(msg_fatal_error, *failure);
            return false;
        }
        //RunTasksInSingleThread();

        save_matrix_states();
        RotateBuffers();
        Propagate();

        CalculateCPUUsage();
        return true;
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
    Kernel::ScanClasses(std::string path) // FIXME: Add error handling
    {
        if(!std::filesystem::exists(path))
        {
            std::cout << "Could not scan for classes \"" + path + "\". Directory not found.\n";
            return;
        }
        for(auto& p: std::filesystem::recursive_directory_iterator(path))
            if(std::string(p.path().extension())==".ikc")
            {
                try
                {
                    std::string name = p.path().stem();
                    classes[name].path = p.path();
                    LoadXMLWithRestrictedIncludes(classes[name].info_, p.path());

                    ensure_list(classes[name].info_, "parameters");

                    bool has_log_level = false;
                    bool has_module_start = false;
                    bool has_start_tick = false;
                    bool has_async = false;
                    bool has_color = false;
                    for(auto parameter : classes[name].info_["parameters"])
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
                        classes[name].info_["parameters"].push_back(make_log_level_parameter().copy());

                    if(!has_module_start)
                        classes[name].info_["parameters"].push_back(make_module_start_parameter().copy());

                    if(!has_start_tick)
                        classes[name].info_["parameters"].push_back(make_start_tick_parameter().copy());

                    if(!has_async)
                        classes[name].info_["parameters"].push_back(make_async_parameter().copy());

                    if(!has_color)
                        classes[name].info_["parameters"].push_back(make_color_parameter().copy());
                }
                catch(const exception & e)
                {
                    Notify(msg_warning, "Could not load class file \"" + p.path().string() + "\": " + e.message(), p.path().string());
                    classes.erase(p.path().stem());
                }
                catch(const std::exception & e)
                {
                    Notify(msg_warning, "Could not load class file \"" + p.path().string() + "\": " + e.what(), p.path().string());
                    classes.erase(p.path().stem());
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
    Kernel::ResolveParameter(parameter & p,  std::string & name)
    {
        if(p.is_resolved())
            return; // Already set from SetParameters

        std::size_t i = name.rfind(".");
        if(i == std::string::npos)
            throw exception("Malformed parameter name \"" + name + "\".");

        Component * c = components.at(name.substr(0, i)).get();
        std::string parameter_name = name.substr(i+1, name.size());
        c->ResolveParameter(p, parameter_name);
    }


    void 
    Kernel::ResolveParameters() // Find and evaluate value or default
    {
        // All all componenets to initialize parameters programmatically

        for(auto & [name, component] : components)
        {
            (void)name;
            component->SetParameters();
        }

        // resolve
        bool ok = true;
        for (auto p=parameters.rbegin(); p!=parameters.rend(); p++) // Reverse order equals outside in in groups
        {
            std::size_t i = p->first.rfind(".");
            if(i == std::string::npos)
                throw setup_failed("Malformed parameter name \""+p->first+"\".", p->first);

            Component * c = components.at(p->first.substr(0, i)).get();
            std::string parameter_name = p->first.substr(i+1, p->first.size());
            ok &= c->ResolveParameter(p->second, parameter_name);
        }

        for(auto & [name, component] : components)
        {
            (void)name;
            component->SyncFirstTickFromParameter();
            component->SyncAsyncModeFromParameter();
        }

        if(!ok)
        {
            for(auto & [name, parameter] : parameters)
                if(!parameter.is_resolved())
                    throw setup_failed("Parameter \""+name+"\" could not be resolved.", name);
            throw setup_failed("All parameters could not be resolved.");
        }
    }


    void 
    Kernel::CalculateSizes()    
    {
        try 
        {
            // Build input table
            std::map<std::string,std::vector<Connection *>> ingoing_connections; 
            for(auto & c : connections)
                ingoing_connections[c.target].push_back(&c);

            auto count_pending_sizes = [&]() -> std::size_t
            {
                std::size_t pending = 0;
                for(auto & [name, component] : components)
                {
                    for(dictionary d : component->info_["inputs"])
                    {
                        std::string full_name = name + "." + d["name"].as_string();
                        if(!d.is_set("optional") && ingoing_connections.count(full_name) && buffers[full_name].is_uninitialized())
                            pending++;
                    }

                    bool is_module = dynamic_cast<Module *>(component.get()) != nullptr;
                    for(dictionary d : component->info_["outputs"])
                    {
                        std::string full_name = name + "." + d["name"].as_string();
                        if((is_module || ingoing_connections.count(full_name)) && buffers[full_name].is_uninitialized())
                            pending++;
                    }

                    for(dictionary d : component->info_["states"])
                    {
                        std::string full_name = name + "." + d["name"].as_string();
                        if(state_buffers.count(full_name) && buffers[full_name].is_uninitialized())
                            pending++;
                    }
                }
                return pending;
            };

            auto pending_size_names = [&]() -> std::vector<std::string>
            {
                std::vector<std::string> pending;
                for(auto & [name, component] : components)
                {
                    for(dictionary d : component->info_["inputs"])
                    {
                        std::string full_name = name + "." + d["name"].as_string();
                        if(!d.is_set("optional") && ingoing_connections.count(full_name) && buffers[full_name].is_uninitialized())
                            pending.push_back(full_name);
                    }

                    bool is_module = dynamic_cast<Module *>(component.get()) != nullptr;
                    for(dictionary d : component->info_["outputs"])
                    {
                        std::string full_name = name + "." + d["name"].as_string();
                        if((is_module || ingoing_connections.count(full_name)) && buffers[full_name].is_uninitialized())
                            pending.push_back(full_name);
                    }

                    for(dictionary d : component->info_["states"])
                    {
                        std::string full_name = name + "." + d["name"].as_string();
                        if(state_buffers.count(full_name) && buffers[full_name].is_uninitialized())
                            pending.push_back(full_name);
                    }
                }
                return pending;
            };

            auto size_signature = [&]() -> std::size_t
            {
                std::size_t signature = 0;
                for(auto & [name, buffer] : buffers)
                {
                    std::size_t local = std::hash<std::string>{}(name);
                    local ^= std::hash<int>{}(buffer.rank()) + 0x9e3779b9 + (local << 6) + (local >> 2);
                    for(int dim : buffer.shape())
                        local ^= std::hash<int>{}(dim) + 0x9e3779b9 + (local << 6) + (local >> 2);
                    signature ^= local + 0x9e3779b9 + (signature << 6) + (signature >> 2);
                }
                return signature;
            };

            std::size_t previous_pending = count_pending_sizes();
            std::size_t previous_signature = size_signature();
            for(std::size_t i = 0; i < components.size(); ++i)
            {
                for(auto & [n, c] : components)
                    c->SetSizes(ingoing_connections);

                std::size_t pending = count_pending_sizes();
                std::size_t signature = size_signature();
                if(signature == previous_signature && pending == previous_pending)
                    break;
                previous_pending = pending;
                previous_signature = signature;
            }

            for(auto & [n, c] : components)
                c->CheckRequiredInputs();

            std::size_t pending = count_pending_sizes();
            if(pending != 0)
                throw setup_failed("Could not resolve all input and output sizes. " + std::to_string(pending) + " buffers remain unresolved: " + join(", ", pending_size_names(), false) + ".");
        }
        catch(fatal_error & e)
        {
            Notify(msg_warning, e.message());
            throw setup_failed("Could not calculate input and output sizes. "+e.message(), e.path());
        }

        catch(setup_failed & e)
        {
            Notify(msg_warning, e.message());
            throw setup_failed("Could not calculate input and output sizes. "+e.message(), e.path());
        }

        catch(const std::exception & e)
        {
            throw setup_failed("Could not calculate input and output sizes. " + std::string(e.what()));
        }

        catch(...)
        {
            throw setup_failed("Could not calculate input and output sizes.");
        }
    }


    void
    Kernel::ShareZeroDelayConnectionBuffers()
    {
        std::map<std::string, std::vector<Connection *>> incoming_connections;
        for(auto & connection : connections)
            incoming_connections[connection.target].push_back(&connection);

        for(auto & connection : connections)
        {
            connection.shared_memory_ = false;

            if(!connection.IsSingleDelay(0))
                continue;

            if(Component * source_component = ComponentForValuePath(connection.source); source_component != nullptr && source_component->async_mode)
                continue;

            if(Component * target_component = ComponentForValuePath(connection.target); target_component != nullptr && target_component->async_mode)
                continue;

            if(!connection.IsWholeMatrixConnection() || connection.stacked_)
                continue;

            if(incoming_connections[connection.target].size() != 1)
                continue;

            auto source_buffer = buffers.find(connection.source);
            auto target_buffer = buffers.find(connection.target);
            if(source_buffer == buffers.end() || target_buffer == buffers.end())
                continue;

            if(connection.source == connection.target)
                continue;

            if(source_buffer->second.is_dynamic() || target_buffer->second.is_dynamic())
                continue;

            if(source_buffer->second.shape() != target_buffer->second.shape())
                continue;

            target_buffer->second.share_storage(source_buffer->second);
            connection.shared_memory_ = true;
        }
    }


    void 
    Kernel::CalculateDelays()
    {
        max_delays.clear();
        for(auto & c : connections)
        {
            if(!c.UsesCircularBuffer())
                continue;
            max_delays[c.source] = std::max(max_delays[c.source], c.MaxDelay());
        }

        enum class DataSnapshotKind
        {
            json,
            image
        };

        struct DataSnapshotItem
        {
            std::string prefix;
            DataSnapshotKind kind = DataSnapshotKind::json;
            std::string json_value;
            matrix image;
            std::string image_format;
        };
    }

    namespace
    {
        constexpr int unresolved_startup_step = std::numeric_limits<int>::max();

        int
        ConnectionDelayMin(const Connection & connection)
        {
            return connection.MinDelay();
        }


        int
        ConnectionDelayMax(const Connection & connection)
        {
            return connection.MaxDelay();
        }

    }


    void
    Kernel::CalculateStartupSteps()
    {
        std::map<std::string, std::vector<Connection *>> incoming_connections;
        std::map<std::string, int> buffer_first_real_step;

        for(auto & [buffer_name, buffer] : buffers)
        {
            (void)buffer;
            buffer_first_real_step[buffer_name] = unresolved_startup_step;
        }

        for(auto & connection : connections)
            incoming_connections[connection.target].push_back(&connection);

        for(auto & [path, component] : components)
        {
            if(dynamic_cast<Module *>(component.get()) == nullptr)
                continue;

            bool has_connected_input = false;
            if(component->info_.contains("inputs") && component->info_["inputs"].is_list())
                for(auto & input : component->info_["inputs"])
                {
                    std::string input_name = path + "." + std::string(input["name"]);
                    if(incoming_connections.count(input_name))
                    {
                        has_connected_input = true;
                        break;
                    }
                }

            if(has_connected_input)
                continue;

            if(component->info_.contains("outputs") && component->info_["outputs"].is_list())
                for(auto & output : component->info_["outputs"])
                {
                    std::string output_name = path + "." + std::string(output["name"]);
                    buffer_first_real_step[output_name] = 0;
                }
        }

        size_t max_iterations = std::max<size_t>(1, buffers.size() + components.size() + connections.size());
        for(size_t iteration = 0; iteration < max_iterations; ++iteration)
        {
            bool changed = false;

            for(auto & [path, component] : components)
            {
                auto * module = dynamic_cast<Module *>(component.get());
                if(module == nullptr)
                    continue;

                int first_input_step = unresolved_startup_step;
                bool has_connected_input = false;

                if(component->info_.contains("inputs") && component->info_["inputs"].is_list())
                    for(auto & input : component->info_["inputs"])
                    {
                        std::string input_name = path + "." + std::string(input["name"]);
                        if(!incoming_connections.count(input_name))
                            continue;

                        has_connected_input = true;
                        first_input_step = std::min(first_input_step, buffer_first_real_step[input_name]);
                    }

                int first_output_step = has_connected_input ? first_input_step : 0;
                if(first_output_step == unresolved_startup_step)
                    continue;

                if(component->info_.contains("outputs") && component->info_["outputs"].is_list())
                    for(auto & output : component->info_["outputs"])
                    {
                        std::string output_name = path + "." + std::string(output["name"]);
                        if(first_output_step < buffer_first_real_step[output_name])
                        {
                            buffer_first_real_step[output_name] = first_output_step;
                            changed = true;
                        }
                    }
            }

            for(auto & connection : connections)
            {
                auto source_it = buffer_first_real_step.find(connection.source);
                if(source_it == buffer_first_real_step.end() || source_it->second == unresolved_startup_step)
                    continue;

                int candidate_step = source_it->second + ConnectionDelayMin(connection);
                int & target_step = buffer_first_real_step[connection.target];
                if(candidate_step < target_step)
                {
                    target_step = candidate_step;
                    changed = true;
                }
            }

            if(!changed)
                break;
        }

        for(auto & [path, component] : components)
        {
            int first_real_input_step = unresolved_startup_step;
            int all_real_inputs_step = 0;
            bool has_connected_input = false;
            bool all_inputs_resolved = true;

            if(component->info_.contains("inputs") && component->info_["inputs"].is_list())
                for(auto & input : component->info_["inputs"])
                {
                    std::string input_name = path + "." + std::string(input["name"]);
                    if(!incoming_connections.count(input_name))
                        continue;

                    for(auto * connection : incoming_connections[input_name])
                    {
                        has_connected_input = true;

                        auto source_it = buffer_first_real_step.find(connection->source);
                        if(source_it == buffer_first_real_step.end() || source_it->second == unresolved_startup_step)
                        {
                            all_inputs_resolved = false;
                            continue;
                        }

                        int connection_first_step = source_it->second + ConnectionDelayMin(*connection);
                        int connection_all_step = source_it->second + ConnectionDelayMax(*connection);

                        first_real_input_step = std::min(first_real_input_step, connection_first_step);
                        all_real_inputs_step = std::max(all_real_inputs_step, connection_all_step);
                    }
                }

            if(!has_connected_input)
            {
                first_real_input_step = 0;
                all_real_inputs_step = 0;
            }
            else if(!all_inputs_resolved)
            {
                all_real_inputs_step = unresolved_startup_step;
            }

            component->startup_first_real_input_step = first_real_input_step;
            component->startup_all_real_inputs_step = all_real_inputs_step;
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
        return tick;
    }

    double
    Kernel::GetTickDuration()
    {
        return tick_duration;
    }

    double
    Kernel::GetTime()
    {
        return (run_mode.load() == run_mode_realtime) ? GetRealTime() : static_cast<double>(tick)*tick_duration;
    }

    double
    Kernel::GetRealTime()
    {
        return (run_mode.load() == run_mode_realtime) ? timer.GetTime() : static_cast<double>(tick)*tick_duration;
    }

    double
    Kernel::GetNominalTime()
    {
        return static_cast<double>(tick)*tick_duration;
    }

    double
    Kernel::GetLag()
    {
        return (run_mode.load() == run_mode_realtime) ? static_cast<double>(tick)*tick_duration - timer.GetTime() : 0;
    }

    double
    Kernel::GetUptime()
    {
        return uptime_timer.GetTime();
    }

    double
    Kernel::GetActualTickDuration() const
    {
        return actual_tick_duration;
    }

    double
    Kernel::GetTickTimeUsage() const
    {
        return tick_time_usage;
    }

    double
    Kernel::GetCPUUsage() const
    {
        return cpu_usage;
    }

    double
    Kernel::GetIdleTime() const
    {
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
        if(auth_enabled_ && (auth_password_.empty() || auth_password_ == "true"))
            throw exception("Authentication requires a non-empty password supplied as -a<password>.");
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
    Kernel::AuthEnabled() const
    {
        return auth_enabled_;
    }

    bool
    Kernel::CheckPassword(const std::string & candidate) const
    {
        return auth_enabled_ && constant_time_equals(candidate, auth_password_);
    }

    std::string
    Kernel::CreateSessionToken()
    {
        if(!auth_enabled_ || auth_cookie_secret_.empty())
            return "";

        static constexpr long long auth_cookie_lifetime_seconds = 30LL * 24LL * 60LL * 60LL;
        const long long expires_at = unix_time_seconds() + auth_cookie_lifetime_seconds;
        const std::string payload =
            std::to_string(expires_at) + "." +
            random_hex_string(16) + "." +
            PasswordMarker();

#if IKAROS_HAVE_COMMONCRYPTO
        return payload + "." + hmac_sha256_hex(auth_cookie_secret_, payload);
#else
        return "";
#endif
    }

    std::string
    Kernel::PasswordMarker() const
    {
#if IKAROS_HAVE_COMMONCRYPTO
        if(auth_cookie_secret_.empty() || auth_password_.empty())
            return "";
        return hmac_sha256_hex(auth_cookie_secret_, "password:" + auth_password_).substr(0, 32);
#else
        return "";
#endif
    }

    bool
    Kernel::LoadOrCreateAuthCookieSecret()
    {
#if !IKAROS_HAVE_COMMONCRYPTO
        return false;
#else
        std::lock_guard<std::mutex> lock(auth_mutex_);
        if(!auth_cookie_secret_.empty())
            return true;

        std::filesystem::path secret_path = std::filesystem::path(user_dir) / ".auth_cookie_secret";
        std::error_code ec;

        if(std::filesystem::exists(secret_path, ec) && !ec)
        {
            std::ifstream secret_file(secret_path);
            std::string secret;
            std::getline(secret_file, secret);
            secret = trim(secret);
            if(secret.size() >= 32)
            {
                auth_cookie_secret_ = secret;
                return true;
            }
        }

        auth_cookie_secret_ = random_hex_string(32);
        std::ofstream secret_file(secret_path, std::ios::trunc);
        if(!secret_file.is_open())
        {
            auth_cookie_secret_.clear();
            return false;
        }

        secret_file << auth_cookie_secret_ << '\n';
        secret_file.close();
        if(secret_file.fail())
        {
            auth_cookie_secret_.clear();
            return false;
        }

        std::filesystem::permissions(
            secret_path,
            std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
            std::filesystem::perm_options::replace,
            ec);

        return true;
#endif
    }

    bool
    Kernel::IsRequestAuthenticated() const
    {
        if(!auth_enabled_)
            return true;
        if(socket == nullptr || !socket->header.contains_non_null("cookie"))
            return false;

        std::string cookie_value = extract_cookie_value(std::string(socket->header["cookie"]), "ikaros_session");
        if(cookie_value.empty())
            return false;

        const auto parts = split(cookie_value, ".");
        if(parts.size() != 4)
            return false;

        long long expires_at = 0;
        try
        {
            expires_at = std::stoll(parts[0]);
        }
        catch(const std::exception &)
        {
            return false;
        }

        if(expires_at < unix_time_seconds())
            return false;

        const std::string payload = parts[0] + "." + parts[1] + "." + parts[2];
        if(!constant_time_equals(parts[2], PasswordMarker()))
            return false;

#if IKAROS_HAVE_COMMONCRYPTO
        std::lock_guard<std::mutex> lock(auth_mutex_);
        if(auth_cookie_secret_.empty())
            return false;
        return constant_time_equals(parts[3], hmac_sha256_hex(auth_cookie_secret_, payload));
#else
        return false;
#endif
    }

    bool
    Kernel::IsPublicRequest(const Request & request) const
    {
        std::string public_url = request.url;
        if(!public_url.empty() && public_url[0] == '/')
            public_url.erase(0, 1);

        if(request.command == "auth" || request.command == "login")
            return true;

        if(request.command.empty() || public_url == "index.html")
            return true;

        static const std::array<std::string, 8> public_prefixes = {
            "core/",
            "ui/",
            "widgets/",
            "js/",
            "Images/",
            "images/",
            "Models/",
            "models/"
        };

        for(const auto & prefix : public_prefixes)
            if(starts_with(public_url, prefix))
                return true;

        static const std::array<std::string, 10> public_files = {
            "index.html",
            "style.css",
            "defaults.css",
            "widget_style.css",
            "widget_defaults.css",
            "info.html",
            "profiling_window.html",
            "startup_steps_window.html",
            "error.glb",
            "old_style.css"
        };

        return std::find(public_files.begin(), public_files.end(), public_url) != public_files.end();
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



    // Functions for creating the network

    void 
    Kernel::AddInput(std::string name, dictionary parameters) 
    {
        buffers[name] = matrix().set_name(parameters["name"]);
    }

    void 
    Kernel::AddOutput(std::string name, dictionary parameters)
    {
        buffers[name] = matrix().set_name(parameters["name"]);
        if(parameters.is_set("persistent"))
            persistent_outputs.insert(name);
    }


    void
    Kernel::AddState(std::string name, dictionary parameters)
    {
        if(!parameters.contains_non_null("type"))
            throw exception("State \"" + name + "\" must have a type.");

        std::string type = parameters["type"];
        if(type == "matrix")
        {
            buffers[name] = matrix().set_name(parameters["name"]);
            state_buffers.insert(name);
            if(parameters.is_set("persistent"))
                persistent_state_buffers.insert(name);
            return;
        }

        if(!is_scalar_state_type(type))
            throw exception("State \"" + name + "\" has unsupported type \"" + type + "\".");

        ScalarState state;
        state.type = type;
        state.persistent = parameters.is_set("persistent");
        std::string default_value = parameters.contains_non_null("default") ? std::string(parameters["default"]) : "";
        try
        {
            if(type == "float")
                state.default_float_value = state.float_value = default_value.empty() ? 0 : static_cast<float>(parse_parameter_number(default_value, "float"));
            else if(type == "double")
                state.default_double_value = state.double_value = default_value.empty() ? 0 : parse_parameter_number(default_value, "double");
            else if(type == "int")
                state.default_int_value = state.int_value = default_value.empty() ? 0 : std::stoi(default_value);
            else if(type == "bool")
                state.default_bool_value = state.bool_value = default_value.empty() ? false : ::ikaros::is_true(default_value);
            else if(type == "string")
                state.default_string_value = state.string_value = default_value;
        }
        catch(const std::exception & e)
        {
            throw exception("State \"" + name + "\" has invalid default value \"" + default_value + "\": " + e.what());
        }

        scalar_states[name] = state;
    }

    void 
    Kernel::AddParameter(std::string name, dictionary params)
    {
         parameters.emplace(name, parameter(params));
    }


    void 
    Kernel::SetParameter(std::string name, std::string value)
    {
        if(!parameters.count(name))
            throw exception("Parameter \""+name+"\" could not be set because it does not exist.");

        try
        {
            parameters[name] = value;
            parameters[name].set_source_value(value);
        }
        catch(const exception & e)
        {
            throw exception("Parameter \""+name+"\" could not be set: "+e.message());
        }
        catch(const std::exception & e)
        {
            throw exception("Parameter \""+name+"\" could not be set: "+std::string(e.what()));
        }
        catch(...)
        {
            throw exception("Parameter \""+name+"\" could not be set. Check that the parameter exists and that the data type and value is correct.");
        }
    }


    void
    Kernel::SetParameter(std::string name, const matrix & value, const std::string & source_value)
    {
        if(!parameters.count(name))
            throw exception("Parameter \""+name+"\" could not be set because it does not exist.");

        try
        {
            parameters[name].set_matrix(value);
            matrix stored_value = value;
            parameters[name].set_source_value(source_value.empty() ? stored_value.json() : source_value);
        }
        catch(const exception & e)
        {
            throw exception("Parameter \""+name+"\" could not be set: "+e.message());
        }
        catch(const std::exception & e)
        {
            throw exception("Parameter \""+name+"\" could not be set: "+std::string(e.what()));
        }
        catch(...)
        {
            throw exception("Parameter \""+name+"\" could not be set. Check that the parameter exists and that the data type and value is correct.");
        }
    }


    void 
    Kernel::AddGroup(dictionary info, std::string path, bool is_top_group)
    {
        if(info["parameters"].is_null())
            info["parameters"] = list();

        if(is_top_group)
        {
            top_group_path = path;
            bool has_color = false;
            bool has_rgb_quality = false;
            bool has_gray_quality = false;
            bool has_snapshot_interval = false;
            bool has_webui_req_int = false;
            bool has_webui_log_buffer_limit = false;
            for(auto parameter : info["parameters"])
            {
                std::string parameter_name = parameter["name"];
                if(parameter_name == "color")
                    has_color = true;
                else if(parameter_name == "rgb_quality")
                    has_rgb_quality = true;
                else if(parameter_name == "gray_quality")
                    has_gray_quality = true;
                else if(parameter_name == "snapshot_interval")
                    has_snapshot_interval = true;
                else if(parameter_name == "webui_req_int")
                    has_webui_req_int = true;
                else if(parameter_name == "webui_log_buffer_limit")
                    has_webui_log_buffer_limit = true;
            }

            if(!has_color)
                info["parameters"].push_back(make_color_parameter().copy());
            if(!has_rgb_quality)
                info["parameters"].push_back(make_ui_snapshot_rgb_quality_parameter().copy());
            if(!has_gray_quality)
                info["parameters"].push_back(make_ui_snapshot_gray_quality_parameter().copy());
            if(!has_snapshot_interval)
                info["parameters"].push_back(make_snapshot_interval_parameter().copy());
            if(!has_webui_req_int)
                info["parameters"].push_back(make_webui_request_interval_parameter().copy());
            if(!has_webui_log_buffer_limit)
                info["parameters"].push_back(make_webui_log_buffer_limit_parameter().copy());
        }

        current_component_info = info;
        current_component_path = path;

        if(components.count(current_component_path)> 0)
            throw build_failed("Module or group named \""+current_component_path+"\" already exists.", path);

        components[current_component_path] = std::make_unique<Group>(); // Implicit argument passing as for components
    }


    void 
    Kernel::InstantiatePythonModule(dictionary & info, const std::string & path)
    {
        current_component_info = info;
        current_component_path = path+"."+std::string(info["name"]);

        if(!classes.count("PythonModule") || classes["PythonModule"].module_creator == nullptr)
            throw build_failed("Internal PythonModule runtime class is not installed.", path);

        components[current_component_path] = std::unique_ptr<Component>(classes["PythonModule"].module_creator());
    }


    bool
    Kernel::PreparePythonModule(dictionary & info, const std::string & classname)
    {
        std::filesystem::path class_path = classes[classname].path;
        std::filesystem::path class_directory = class_path.parent_path();

        std::filesystem::path python_path = class_directory / (classname + ".py");
        bool is_python_backed = std::filesystem::exists(python_path);

        if(!is_python_backed)
            return false;

        std::error_code ec;
        std::filesystem::path canonical_class_directory = std::filesystem::weakly_canonical(class_directory, ec);
        if(ec)
            throw build_failed("Could not resolve python class directory for class \"" + classname + "\".");

        if(classes.count("PythonModule"))
        {
            dictionary python_runtime_info = classes["PythonModule"].info_.copy();
            python_runtime_info.erase("name");
            python_runtime_info.erase("description");
            info.merge(python_runtime_info);

            if(info["parameters"].is_null())
                info["parameters"] = list();

            std::set<std::string> parameter_names;
            for(auto parameter : info["parameters"])
                parameter_names.insert(std::string(parameter["name"]));

            for(auto parameter : python_runtime_info["parameters"])
            {
                std::string parameter_name = parameter["name"];
                if(!parameter_names.count(parameter_name))
                {
                    info["parameters"].push_back(parameter);
                    parameter_names.insert(parameter_name);
                }
            }
        }

        std::filesystem::path canonical_python_path = std::filesystem::weakly_canonical(python_path, ec);
        if(ec)
            throw build_failed("Python script for class \"" + classname + "\" could not be resolved: " + python_path.string());

        auto class_it = canonical_class_directory.begin();
        auto class_end = canonical_class_directory.end();
        auto python_it = canonical_python_path.begin();
        auto python_end = canonical_python_path.end();
        for(; class_it != class_end && python_it != python_end; ++class_it, ++python_it)
            if(*class_it != *python_it)
                throw build_failed("Python script for class \"" + classname + "\" must stay within its class directory.");
        if(class_it != class_end)
            throw build_failed("Python script for class \"" + classname + "\" must stay within its class directory.");

        info["python"] = canonical_python_path.string();
        return true;
    }


    void
    Kernel::InstantiateStandardModule(dictionary & info, const std::string & classname, const std::string & path)
    {
        current_component_info = info;
        current_component_path = path+"."+std::string(info["name"]);

        if(classes[classname].module_creator == nullptr)
        {
            if(info.is_not_set("no_code"))
                std::cout << "Class \""<< classname << "\" has no installed code. Creating group." << std::endl; // throw exception("Class \""+classname+"\" has no installed code. Check that it is included in CMakeLists.txt."); // TODO: Check that this works for classes that are allowed to have no code
            info["_tag"]="group";
            BuildGroup(info, path); 
        }
        else
            components[current_component_path] = std::unique_ptr<Component>(classes[classname].module_creator());
    }


    void 
    Kernel::AddModule(dictionary info, std::string path)
    {
        current_component_info = info;
        current_component_path = path+"."+std::string(info["name"]);

        if(components.count(current_component_path)> 0)
            throw build_failed("Module or group with this name already exists. \""+std::string(info["name"])+"\".", path);

        std::string classname = info["class"];

        if(!classname.empty() && (classname.find('@') != std::string::npos || classname.find('{') != std::string::npos))
        {
            Component * c = components.at(path).get();
            classname = c->ComputeValue(classname);
            info["class"] = classname;
        }

        if(!classes.count(classname))
            throw build_failed("Class \""+classname+"\" does not exist.", path);

        if(classes[classname].path.empty())
            throw build_failed("Class file \""+classname+".ikc\" could not be found.", path);

        info.merge(classes[classname].info_);  // merge with scanned class data, including injected defaults

        bool is_python_backed = PreparePythonModule(info, classname);

        if(info["parameters"].is_null())
            info["parameters"] = list();

        bool has_log_level = false;
        bool has_module_start = false;
        bool has_start_tick = false;
        bool has_async = false;
        bool has_color = false;
        for(auto parameter : info["parameters"])
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
        {
            dictionary log_param;
            log_param["_tag"] = "parameter";
            log_param["name"] = "log_level";
            log_param["type"] = "number";
            log_param["control"] = "menu";
            log_param["options"] = "inherit,quiet,exception,end_of_file,terminate,fatal_error,warning,print,debug,trace";
            log_param["default"] = 0;
            info["parameters"].push_back(log_param);
        }

        if(!has_module_start)
            info["parameters"].push_back(make_module_start_parameter().copy());

        if(!has_start_tick)
            info["parameters"].push_back(make_start_tick_parameter().copy());

        if(!has_async)
            info["parameters"].push_back(make_async_parameter().copy());

        if(!has_color)
        {
            dictionary color_param;
            color_param["_tag"] = "parameter";
            color_param["name"] = "color";
            color_param["type"] = "string";
            color_param["default"] = "black";
            color_param["description"] = "Selected ui color";
            color_param["control"] = "ui_color";
            info["parameters"].push_back(color_param);
        }

        if(is_python_backed)
            InstantiatePythonModule(info, path);
        else
            InstantiateStandardModule(info, classname, path);
    }


    void 
    Kernel::AddConnection(dictionary info, std::string path)
    {
         std::string source = path + "." + std::string(info["source"]); 
         std::string target = path + "." + std::string(info["target"]);

        if(state_buffers.count(source) || scalar_states.count(source))
            throw build_failed("Connection source \"" + source + "\" is private state and can not be connected.", source);
        if(state_buffers.count(target) || scalar_states.count(target))
            throw build_failed("Connection target \"" + target + "\" is private state and can not be connected.", target);

         std::string delay_range = info.contains_non_null("delay") ? info["delay"] : "";
         std::string label = info.contains_non_null("label") ? info["label"] : "";

        if(delay_range.empty() || delay_range=="null")
            delay_range = "[1]";
        else if(delay_range[0] != '[')
            delay_range = "["+delay_range+"]";
        range r;
        try
        {
            r = range(delay_range);
        }
        catch(const std::exception &)
        {
            throw build_failed("Connection \"" + source + " => " + target +
                               "\" has malformed delay range \"" + delay_range + "\".", path);
        }
        ValidateConnectionDelayRange(r, source, target, path);
        connections.push_back(Connection(source, target, r, label));
    }



    void Kernel::LoadExternalGroup(dictionary & d)
    {
        std::filesystem::path sanitized_path;
        if(!SanitizeImportPath(std::string(d["external"]), sanitized_path))
            throw build_failed("External group path must stay within the project root or user data directory.");

        dictionary external;
        LoadXMLWithRestrictedIncludes(external, sanitized_path);
        external["name"] = d["name"];
        d.merge(external);
        d.erase("external");
    }



    void 
    Kernel::BuildGroup(dictionary d, std::string path) // Traverse dictionary and build all items at each level
    {
        try
        {
            if(std::string(d["_tag"]) != "group")
                throw build_failed("Main element is '"+std::string(d["_tag"])+"' but must be 'group' for ikg-file.");

            if(!d.contains("name"))
                throw build_failed("Groups must have a name.", path);

            if(path.empty())
            {
                std::string log_level = d.contains_non_null("log_level") ? std::string(d["log_level"]) : "";
                if(log_level.empty() || log_level == "0")
                    d["log_level"] = msg_warning;
            }

            std::string name = validate_identifier(d["name"]);
            if(!path.empty())
                name = path+"."+name;

            if(d.contains("external"))
                LoadExternalGroup(d);

            AddGroup(d, name, path.empty());

            for(auto g : d["groups"])
                BuildGroup(g, name);
            for(auto m : d["modules"])
                AddModule(m, name);
            for(auto c : d["connections"])
                AddConnection(c, name);

            if(d["widgets"].is_null())
                d["widgets"] = list();
        }
        catch(const exception& e)
        {
            throw build_failed("Build group failed for "+path+": "+e.message());
        }
        catch(const std::exception& e)
        {
            throw build_failed("Build group failed for "+path+": "+std::string(e.what()), path);
        }
    }


    void 
    Kernel::InitComponents()
    {
        // Call Init for all modules (after CalcalateSizes and Allocate)
        for(auto & [name, component] : components)
        {
            (void)name;
            try
            {
                component->Init();
            }
            catch(const fatal_error& e)
            {
                throw init_error(u8"Fatal error. Init failed for \""+component->path_+"\": "+std::string(e.what()), component->path_);
            }
            catch(const std::exception& e)
            {
                throw init_error(u8"Init failed for "+component->path_+": "+std::string(e.what()), component->path_);
            }
            catch(...)
            {
                throw init_error(u8"Init failed");
            }
        }
    }


    void 
    Kernel::SetCommandLineParameters(dictionary & d) // Add explicit command line overrides without clobbering file values with defaults
    {
        // user_data is intentionally CLI-only and must never be sourced from a model file.
        if(d.contains("user_data"))
            d.erase("user_data");

        for(auto & [name, value] : options_.d)
            if(options_.is_explicitly_set(name))
                if(name != "user_data" && name != "auth_password")
                    d[name] = value;

        if(d.contains("stop"))
            stop_after = d["stop"];

        if(d.contains("tick_duration"))
            tick_duration = d["tick_duration"];

        if(d.contains_non_null("threads"))
        {
            std::string thread_pool_value = std::string(d["threads"]);
            int requested_threads = 0;
            try
            {
                requested_threads = std::stoi(thread_pool_value);
            }
            catch(const std::exception &)
            {
                throw setup_failed("Invalid thread pool size \"" + thread_pool_value + "\". Expected a positive integer.");
            }

            if(requested_threads < 1)
                throw setup_failed("Invalid thread pool size \"" + thread_pool_value + "\". Expected a positive integer.");

            thread_pool = std::make_unique<ThreadPool>(requested_threads);
        }
    }


    std::string
    Kernel::GetTopLevelDefaultAttribute(const std::string & key) const
    {
        if(key == "tick_duration")
            return std::to_string(tick_duration);
        if(key == "stop")
            return std::to_string(stop_after);
        if(key == "filename")
            return options_.stem();
        if(key == "batch_mode")
            return options_.is_explicitly_set("batch_mode") ? "true" : "false";
        if(key == "info")
            return options_.is_explicitly_set("info") ? "true" : "false";
        if(key == "real_time")
            return options_.is_explicitly_set("real_time") ? "true" : "false";
        if(key == "start")
            return options_.is_explicitly_set("start") ? "true" : "false";

        auto it = options_.d.find(key);
        if(it != options_.d.end())
            return it->second;

        return "";
    }

    void
    Kernel::RegisterClass(const char * name, ModuleCreator mc)
    {
        classes[name].name = name;
        classes[name].module_creator = mc;
    }


    void 
    Kernel::LoadFile()
    {
        std::lock_guard<std::recursive_mutex> lock(kernelLock);
        try
        {
            if(components.size() > 0)
            {
                StopComponents();
                Clear();
            }
            if(!std::filesystem::exists(options_.full_path()))
                throw load_failed(u8"File \""+options_.full_path()+"\" does not exist.");

                try
                {
                    dictionary d;
                    LoadXMLWithRestrictedIncludes(d, options_.full_path());
                    SetCommandLineParameters(d);
                    d["filename"] = options_.stem();
                    BuildGroup(d);
                    info_ = d;
                    session_id = new_session_id(); 
                    ResetUISnapshotCache();
                    Notify(msg_print, u8"Loaded "s+options_.full_path());
                    SetUp();
                    if(options_.is_explicitly_set("load_state"))
                        LoadState(resolve_state_filename(options_, "load_state"));
                    CalculateCheckSum();
                    BuildUISnapshot();
                    needs_reload = false;
                    Pause(); // Reset clocks
                }

                catch(const load_failed& e)
                {
                    throw load_failed(u8"Load file failed for "s+options_.full_path()+". "+e.message(), e.path());
                }
                catch(const setup_failed& e)
                {
                    throw setup_failed(u8"Set-up file failed for "s+options_.full_path()+". "+e.message(), e.path());
                }
                catch(const std::exception& e)
                {
                    throw load_failed(u8"Load or set-up failed for "s+options_.full_path()+". "+e.what());
                }

        }
        catch(const exception& e)
        {
            Notify(msg_warning, e.what(), e.path());
            throw;
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
            std::string msg = "Incorrect Check Sum: "+std::to_string(calculated_check_sum)+" != "+std::to_string(correct_check_sum);
            Notify(msg_fatal_error, msg);
            if(info_.is_set("batch_mode"))
            {
                StopHTTPServer();
                thread_pool.reset();
                exit(1);
            }
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
        d["classes"] = join(",", summary_entries, false);
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
    Kernel::SaveState(const std::string & filename, const std::string & component_path)
    {
        if(filename.empty())
            throw exception("State filename is empty.");

        const std::string scope = trim(component_path);
        if(!scope.empty() && components.find(scope) == components.end())
            throw exception("Component \"" + scope + "\" could not be found.");

        std::ofstream file(filename);
        if(!file)
            throw exception("Could not open state file \"" + filename + "\" for writing.");

        struct StateItem
        {
            std::string path;
            std::string kind;
            const matrix * buffer = nullptr;
            const ScalarState * scalar = nullptr;
        };

        std::vector<StateItem> items;
        auto collect_matrix_item = [&](const std::string & path, const std::string & kind)
        {
            if(!path_is_in_scope(path, scope))
                return;
            auto buffer = buffers.find(path);
            if(buffer == buffers.end())
                throw exception("Persistent " + kind + " \"" + path + "\" does not exist.");
            if(buffer->second.is_uninitialized())
                throw exception("Persistent " + kind + " \"" + path + "\" has no allocated value.");
            items.push_back({path, kind, &buffer->second, nullptr});
        };

        for(const auto & path : persistent_outputs)
            collect_matrix_item(path, "output");
        for(const auto & path : persistent_state_buffers)
            collect_matrix_item(path, "state");
        for(const auto & [path, state] : scalar_states)
            if(state.persistent && path_is_in_scope(path, scope))
                items.push_back({path, "state", nullptr, &state});

        file << "{\n";
        file << "  \"format\": \"ikaros-state-v1\",\n";
        file << "  \"tick\": " << tick << ",\n";
        file << "  \"saved_at_utc\": \"" << current_utc_timestamp() << "\",\n";
        file << "  \"ikaros_version\": \"" << ikaros_version << "\",\n";
        file << "  \"model_filename\": \"" << escape_json_string(options_.filename()) << "\",\n";
        file << "  \"model_name\": \"" << escape_json_string(info_["name"]) << "\",\n";
        file << "  \"scope\": \"" << escape_json_string(scope.empty() ? "network" : scope) << "\",\n";
        file << "  \"item_count\": " << items.size() << ",\n";
        file << "  \"items\": {\n";

        std::string separator;
        auto write_matrix_item = [&](const StateItem & item)
        {
            file << separator;
            file << "    \"" << escape_json_string(item.path) << "\": {\n";
            file << "      \"kind\": \"" << item.kind << "\",\n";
            file << "      \"type\": \"matrix\",\n";
            file << "      \"shape\": [";
            std::string shape_separator;
            for(int dimension : item.buffer->shape())
            {
                file << shape_separator << dimension;
                shape_separator = ", ";
            }
            file << "],\n";
            file << "      \"value\": " << item.buffer->json() << "\n";
            file << "    }";
            separator = ",\n";
        };

        auto write_scalar_item = [&](const StateItem & item)
        {
            const ScalarState & state = *item.scalar;

            file << separator;
            file << "    \"" << escape_json_string(item.path) << "\": {\n";
            file << "      \"kind\": \"" << item.kind << "\",\n";
            file << "      \"type\": \"" << state.type << "\",\n";
            file << "      \"value\": ";
            if(state.type == "float")
            {
                double value = state.float_ptr ? *state.float_ptr : state.float_value;
                file << format_json_number(value);
            }
            else if(state.type == "double")
            {
                double value = state.double_ptr ? *state.double_ptr : state.double_value;
                file << format_json_number(value);
            }
            else if(state.type == "int")
            {
                int value = state.int_ptr ? *state.int_ptr : state.int_value;
                file << value;
            }
            else if(state.type == "bool")
            {
                bool value = state.bool_ptr ? *state.bool_ptr : state.bool_value;
                file << (value ? "true" : "false");
            }
            else if(state.type == "string")
            {
                const std::string & value = state.string_ptr ? *state.string_ptr : state.string_value;
                file << ikaros::value(value).json();
            }
            file << "\n";
            file << "    }";
            separator = ",\n";
        };

        for(const auto & item : items)
        {
            if(item.buffer)
                write_matrix_item(item);
            else
                write_scalar_item(item);
        }

        file << "\n";
        file << "  }\n";
        file << "}\n";

        if(!file)
            throw exception("Could not write state file \"" + filename + "\".");

        Notify(msg_print, "Saved state to " + filename + (scope.empty() ? "" : " for " + scope));
    }


    void
    Kernel::LoadState(const std::string & filename, const std::string & component_path)
    {
        if(filename.empty())
            throw exception("State filename is empty.");

        const std::string target_scope = trim(component_path);
        if(!target_scope.empty() && components.find(target_scope) == components.end())
            throw exception("Component \"" + target_scope + "\" could not be found.");

        dictionary state;
        try
        {
            state.load_json(filename);
        }
        catch(const std::exception & e)
        {
            throw exception("Could not load state file \"" + filename + "\": " + e.what());
        }

        if(std::string(state["format"]) != "ikaros-state-v1")
            throw exception("State file \"" + filename + "\" has unsupported format \"" + std::string(state["format"]) + "\".");
        if(!state["items"].is_dictionary())
            throw exception("State file \"" + filename + "\" does not contain an items object.");

        std::string saved_scope = state["scope"].is_string() ? state["scope"].as_string() : "";
        dictionary items = std::get<dictionary>(state["items"].value_);
        for(const auto & [path, saved_value] : *items.dict_)
        {
            std::string target_path = remap_scoped_state_path(path, saved_scope, target_scope);
            if(target_path.empty())
                continue;

            if(!saved_value.is_dictionary())
                throw exception("State item \"" + path + "\" is not an object.");

            dictionary item = std::get<dictionary>(saved_value.value_);
            std::string kind = item["kind"];
            std::string type = item["type"];
            if(kind != "output" && kind != "state")
                throw exception("State item \"" + path + "\" has unsupported kind \"" + kind + "\".");
            if(kind == "output" && type != "matrix")
                throw exception("State item \"" + path + "\" is an output but does not have type matrix.");
            if(kind == "output" && !persistent_outputs.count(target_path))
                throw exception("State item \"" + path + "\" does not match a persistent output in the loaded model.");

            if(kind == "state" && type != "matrix")
            {
                auto scalar = scalar_states.find(target_path);
                if(scalar == scalar_states.end() || !scalar->second.persistent)
                    throw exception("State item \"" + path + "\" does not match a persistent private state in the loaded model.");
                if(scalar->second.type != type)
                    throw exception("State item \"" + path + "\" has type \"" + type + "\" but target state has type \"" + scalar->second.type + "\".");

                try
                {
                    if(type == "float")
                    {
                        double value = item["value"].as_double();
                        scalar->second.float_value = static_cast<float>(value);
                        if(scalar->second.float_ptr)
                            *scalar->second.float_ptr = static_cast<float>(value);
                    }
                    else if(type == "double")
                    {
                        double value = item["value"].as_double();
                        scalar->second.double_value = value;
                        if(scalar->second.double_ptr)
                            *scalar->second.double_ptr = value;
                    }
                    else if(type == "int")
                    {
                        int value = item["value"].as_int();
                        scalar->second.int_value = value;
                        if(scalar->second.int_ptr)
                            *scalar->second.int_ptr = value;
                    }
                    else if(type == "bool")
                    {
                        bool value = item["value"].is_true();
                        scalar->second.bool_value = value;
                        if(scalar->second.bool_ptr)
                            *scalar->second.bool_ptr = value;
                    }
                    else if(type == "string")
                    {
                        if(!item["value"].is_string())
                            throw exception("Expected string value.");
                        std::string value = item["value"].as_string();
                        scalar->second.string_value = value;
                        if(scalar->second.string_ptr)
                            *scalar->second.string_ptr = value;
                    }
                    else
                        throw exception("Unsupported scalar state type \"" + type + "\".");
                }
                catch(const exception &)
                {
                    throw;
                }
                catch(const std::exception & e)
                {
                    throw exception("State item \"" + path + "\" has invalid value: " + std::string(e.what()));
                }
                continue;
            }

            if(kind == "state" && !persistent_state_buffers.count(target_path))
                throw exception("State item \"" + path + "\" does not match a persistent private state in the loaded model.");

            auto target = buffers.find(target_path);
            if(target == buffers.end())
                throw exception("State item \"" + path + "\" does not match a value in the loaded model.");

            matrix restored(item["value"].json());
            if(restored.shape() != target->second.shape())
                throw exception("State item \"" + path + "\" has shape " + format_shape(restored.shape()) + " but target " + kind + " has shape " + format_shape(target->second.shape()) + ".");

            target->second.copy(restored);
        }

        Notify(msg_print, "Loaded state from " + filename + (target_scope.empty() ? "" : " into " + target_scope));
    }


    void
    Kernel::ResetState(const std::string & component_path)
    {
        if(component_path.empty())
        {
            for(auto & [path, component] : components)
                component->Reset();
            Notify(msg_print, "Reset state");
            return;
        }

        std::string path = component_path;
        if(!path.empty() && path[0] == '.')
            path = path.substr(1);

        auto component = components.find(path);
        if(component == components.end())
            throw exception("Component \"" + component_path + "\" could not be found.");

        component->second->Reset();
        Notify(msg_print, "Reset state for " + path);
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
        SendProcessStartLogEvent(*this);
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
        SendProcessExitLogEvent(*this);
#endif
    }


    void
    Kernel::LogSessionEvent(const std::string & endpoint, const std::string & event_name)
    {
        SendSessionLogEvent(*this, endpoint, event_name);
    }

    void 
    Kernel::Propagate()
    {
        for(auto & c : connections)
            if(c.HasZeroDelay())
                continue;
            else if(!c.ShouldTick())
                continue;
            else
                try
                {
                    c.Tick();
                }
                catch(const std::exception & e)
                {
                    throw std::runtime_error("Error propagating connection \"" +
                                             c.Info() + "\": " + e.what());
                }
                catch(...)
                {
                    throw std::runtime_error("Unknown error propagating connection \"" +
                                             c.Info() + "\".");
                }
    }



    void
    Kernel::InitSocket(long port)
    {
        try
        {
            shutdown.store(false, std::memory_order_release);
            socket = std::make_unique<ServerSocket>(port, GetOption("bind_address"));
        }
        catch (const exception& e)
        {
            throw socket_startup_error("Ikaros is unable to start the webserver on port " + std::to_string(port) + ": " + e.message(), e.path());
        }
        catch (const std::exception& e)
        {
            throw socket_startup_error("Ikaros is unable to start the webserver on port " + std::to_string(port) + ": " + std::string(e.what()));
        }

        httpThread = std::thread(Kernel::StartHTTPThread, this);
    }


    void
    Kernel::StopHTTPServer()
    {
        shutdown.store(true, std::memory_order_release);

        if(socket != nullptr)
            socket->StopListening();

        if(httpThread.joinable())
        {
            httpThread.join();
        }

        socket.reset();
    }


    void 
    Kernel::PruneConnections()
    {
        for (auto it = connections.begin(); it != connections.end(); ) 
        {
            if(state_buffers.count(it->source) || state_buffers.count(it->target))
            {
                Notify(msg_warning, "Connection \"" + it->Info() + "\" uses private state and can not be connected.");
                it = connections.erase(it);
            }
            else
            if(buffers.count(it->source) && buffers.count(it->target))
                it++;
            else
            {
                Notify(msg_print, u8"Pruning "  + it->source + "=>" + it->target);
                it = connections.erase(it);
            }
        }
    }


/*************************
 * 
 *  Task sorting
 * 
 *************************/

    bool 
    Kernel::DFSCycleCheck(const std::string& node, const std::unordered_map<std::string, std::vector<std::string>>& graph, std::unordered_set<std::string>& visited, std::unordered_set<std::string>& recStack) 
    {
        if(recStack.find(node) != recStack.end())
            return true;

        if(visited.find(node) != visited.end())
            return false;

        visited.insert(node);
        recStack.insert(node);

        if(graph.find(node) != graph.end()) 
            for (const std::string& neighbor : graph.at(node)) 
                if(DFSCycleCheck(neighbor, graph, visited, recStack)) 
                    return true;
        recStack.erase(node);
        return false;
    }



    bool 
    Kernel::HasCycle(const std::vector<std::string>& nodes, const std::vector<std::pair<std::string, std::string>>& edges) 
    {
        std::unordered_map<std::string, std::vector<std::string>> graph;
        for (const auto& edge : edges)
            graph[edge.first].push_back(edge.second);

        std::unordered_set<std::string> visited;
        std::unordered_set<std::string> recStack;

        for (const std::string& node : nodes)
            if(visited.find(node) == visited.end() &&  (DFSCycleCheck(node, graph, visited, recStack)))
                    return true;

        return false;
    }

    void 
    Kernel::DFSSubgroup(const std::string& node, const std::unordered_map<std::string, std::vector<std::string>>& graph, std::unordered_set<std::string>& visited, std::vector<std::string>& component) 
    {
        visited.insert(node);
        component.push_back(node);

        if(graph.find(node) != graph.end()) 
        {
            for (const std::string& neighbor : graph.at(node)) 
            {
                if(visited.find(neighbor) == visited.end()) 
                    DFSSubgroup(neighbor, graph, visited, component);
            }
        }
    }


    std::vector<std::vector<std::string>> 
    Kernel::FindSubgraphs(const std::vector<std::string>& nodes, const std::vector<std::pair<std::string, std::string>>& edges) 
    {
        std::unordered_map<std::string, std::vector<std::string>> graph;
        for (const auto& edge : edges) 
        {
            graph[edge.first].push_back(edge.second);
            graph[edge.second].push_back(edge.first);
        }

        std::unordered_set<std::string> visited;
        std::vector<std::vector<std::string>> components;

        for (const std::string& node : nodes) 
        {
            if(visited.find(node) == visited.end()) 
            {
                std::vector<std::string> component;
                DFSSubgroup(node, graph, visited, component);
                components.push_back(component);
            }
        }

        return components;
    }


    void 
    Kernel::TopologicalSortUtil(const std::string& node, const std::unordered_map<std::string, std::vector<std::string>>& graph, std::unordered_set<std::string>& visited, std::stack<std::string>& Stack) 
    {
        visited.insert(node);

        if(graph.find(node) != graph.end()) 
        {
            for (const std::string& neighbor : graph.at(node)) 
            {
                if(visited.find(neighbor) == visited.end())
                    TopologicalSortUtil(neighbor, graph, visited, Stack);
            }
        }
        Stack.push(node);
    }



    std::vector<std::string> 
    Kernel::TopologicalSort(const std::vector<std::string>& component, const std::unordered_map<std::string, std::vector<std::string>>& graph) 
    {
        std::unordered_set<std::string> visited;
        std::stack<std::string> Stack;

        for (const std::string& node : component) 
            if(visited.find(node) == visited.end())
                TopologicalSortUtil(node, graph, visited, Stack);

        std::vector<std::string> sortedSubgraph;
        while (!Stack.empty()) 
        {
            sortedSubgraph.push_back(Stack.top());
            Stack.pop();
        }

        return sortedSubgraph;
    }


    std::vector<std::vector<std::string>>
    Kernel::Sort(std::vector<std::string> nodes, std::vector<std::pair<std::string, std::string>> edges)
    {
        if(HasCycle(nodes, edges)) 
            throw setup_failed("Network has zero-delay loops");
        else 
        {

            std::vector<std::vector<std::string>> components = FindSubgraphs(nodes, edges);

            // Rebuild the original graph for directed edges
            std::unordered_map<std::string, std::vector<std::string>> graph;
            for (const auto& edge : edges) {
                graph[edge.first].push_back(edge.second);
            }

            std::vector<std::vector<std::string>>  result;

            for (const auto& component : components) {
                std::vector<std::string> sortedSubgraph = TopologicalSort(component, graph);
                result.push_back(sortedSubgraph);
            }

            return result;
        }
    }



    void
    Kernel::SortTasks()
    {
        std::vector<std::string> nodes;
        std::vector<std::pair<std::string, std::string>> arcs;
        std::map<std::string, Task *> task_map;

        for(auto & [s,c] : components)
        {
            nodes.push_back(s);
            task_map[s] = c.get(); // Save in task map
        }

        for(auto & c : connections) // Connections containing delay zero are sorted into tasks
        if(c.HasZeroDelay())
            {
                std::string s = peek_rhead(c.source,".");
                std::string t = peek_rhead(c.target,".");
                std::string cc = "CON("+s+","+t+")"; // Connection node name

                nodes.push_back(cc);
                arcs.push_back({s, cc});
                arcs.push_back({cc, t});
                task_map[cc] = &c; // Save in task map
            }

        auto r = Sort(nodes, arcs);

        // Fill task list

        tasks.clear();
        for(auto s : r)
        {
            std::vector<Task *> task_list;
            bool priority_task = false;
            for(auto ss: s)
            {
                if(task_map[ss]->Priority())
                    priority_task = true;
                task_list.push_back(task_map[ss]); // Get task pointer here
            }
            if(priority_task)
                tasks.insert(tasks.begin(), task_list);
            else
                tasks.push_back(task_list);

        }
    }



    Component *
    Kernel::ComponentForValuePath(const std::string & value_path) const
    {
        std::string component_path = peek_rhead(value_path, ".");
        auto component = components.find(component_path);
        if(component == components.end())
            return nullptr;
        return component->second.get();
    }


    bool
    Kernel::ValueOwnedByRunningAsyncComponent(const std::string & value_path) const
    {
        Component * component = ComponentForValuePath(value_path);
        return component != nullptr && component->IsAsyncRunning();
    }


    void
    Kernel::RunTask(Task * task)
    {
        if(task == nullptr)
            return;
        if(!task->ShouldTick())
            return;

        if(task->kind() == Task::Kind::component)
        {
            auto * component = static_cast<Component *>(task);
            if(component->async_mode)
            {
                if(component->async_publish_pending.exchange(false))
                    return;
                if(component->IsAsyncRunning() || component->IsAsyncFailed())
                    return;
                if(stop_after != -1 && tick >= stop_after)
                    return;

                component->LaunchAsyncTick();
                return;
            }
        }

        const bool profiling_started = task->TryProfilingBegin();
        try
        {
            task->Tick();
        }
        catch(...)
        {
            if(profiling_started)
                task->ProfilingEnd();
            throw;
        }
        if(profiling_started)
            task->ProfilingEnd();
    }


    void
    Kernel::PollAsyncComponents()
    {
        for(auto & [path, component] : components)
        {
            if(!component->async_mode)
                continue;

            try
            {
                component->PollAsyncCompletion();
            }
            catch(const std::exception & e)
            {
                Notify(msg_fatal_error, "Error during asynchronous task completion for \"" + path + "\": " + std::string(e.what()), path);
            }
            catch(...)
            {
                Notify(msg_fatal_error, "Error during asynchronous task completion for \"" + path + "\": Unknown error.", path);
            }
        }
    }


    class KernelTaskSequence: public TaskSequence
    {
    public:
        KernelTaskSequence(Kernel & kernel, const std::vector<Task *> & tasks):
            TaskSequence(tasks),
            kernel_(kernel)
        {
        }

    protected:
        void Tick() override
        {
            for(auto & task : tasks_)
            {
                try
                {
                    kernel_.RunTask(task);
                }
                catch(const std::exception & e)
                {
                    throw std::runtime_error("Error in task \"" + (task ? task->Info() : std::string("<null>")) + "\": " + e.what());
                }
                catch(...)
                {
                    throw std::runtime_error("Error in task \"" + (task ? task->Info() : std::string("<null>")) + "\": Unknown error.");
                }
            }
        }

    private:
        Kernel & kernel_;
    };


    std::optional<std::string>
    Kernel::RunTasks()
    {
        std::vector<std::shared_ptr<TaskSequence>> sequences;
        sequences.reserve(tasks.size());

        std::optional<std::string> failure;
        try
        {
            for(auto & task_sequence : tasks)
            {
                auto ts = std::make_shared<KernelTaskSequence>(*this, task_sequence);
                thread_pool->submit(ts);
                sequences.push_back(ts);
            }
        }
        catch(const std::exception & e)
        {
            failure = "Could not submit task sequence: " + std::string(e.what());
        }
        catch(...)
        {
            failure = "Could not submit task sequence: Unknown error.";
        }

        bool timed_out = false;
        if(!failure && task_timeout > 0)
        {
            const auto timeout = duration_cast<steady_clock::duration>(duration<double>(task_timeout));
            const auto deadline = steady_clock::now() + timeout;
            for(auto & ts : sequences)
            {
                if(ts->isCompleted())
                    continue;

                const auto now = steady_clock::now();
                if(now >= deadline || !ts->waitForCompletion(duration<double>(deadline - now).count()))
                {
                    timed_out = true;
                    break;
                }
            }

            if(timed_out)
            {
                try
                {
                    Notify(msg_warning, "Task execution exceeded " + formatNumber(task_timeout) +
                        " seconds. Waiting for active tasks to finish before stopping safely.");
                }
                catch(...)
                {
                    // The completion barrier below must still run if reporting the watchdog fails.
                }
            }
        }

        // Do not inspect failures or return while submitted sequences can still access kernel data.
        for(auto & ts : sequences)
        {
            try
            {
                ts->waitForCompletion();
            }
            catch(const std::exception & e)
            {
                if(!failure)
                    failure = "Could not wait for task sequence: " + std::string(e.what());
            }
            catch(...)
            {
                if(!failure)
                    failure = "Could not wait for task sequence: Unknown error.";
            }
        }

        if(timed_out)
            failure = "Task execution timed out after " + formatNumber(task_timeout) + " seconds.";

        for(auto & ts : sequences)
        {
            try
            {
                ts->rethrowIfError();
            }
            catch(const std::exception & e)
            {
                if(!failure)
                    failure = "Error during task execution: " + std::string(e.what());
            }
            catch(...)
            {
                if(!failure)
                    failure = "Error during task execution: Unknown error.";
            }
        }

        return failure;
    }



    void
    Kernel::RunTasksInSingleThread()
    {
        for(auto & task_group : tasks)
            for(auto & task: task_group)
                RunTask(task);
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


    void
    Kernel::Run()
    {
        bool has_async_workers = false;
        if(options_.is_set("batch_mode"))
            for(auto & [name, parameter] : parameters)
                if(ends_with(name, ".async") && parameter.as_bool())
                {
                    has_async_workers = true;
                    break;
                }

        // Main loop
        while(run_mode.load() > run_mode_quit && !global_terminate.load())  // Not quit
        {
            while (!Terminate() && run_mode.load() > run_mode_quit)
            {
                if(run_mode.load() == run_mode_realtime)
                {
                    const double target_time = double(tick+1)*tick_duration;
                    lag = timer.WaitUntil(target_time);

                    const bool real_time_catch_up =
                        !info_.contains("real_time_catch_up") || info_.is_set("real_time_catch_up");
                    double real_time_resync_lag = 1.0;
                    if(info_.contains_non_null("real_time_resync_lag"))
                        real_time_resync_lag = std::max(0.0, info_["real_time_resync_lag"].as_double());

                    const bool resync_lag_exceeded = real_time_resync_lag > 0 && lag > real_time_resync_lag;
                    const bool should_resync = resync_lag_exceeded || (!real_time_catch_up && lag > 0);
                    if(should_resync)
                    {
                        static Timer resync_warning_timer;
                        static bool has_warned_about_resync = false;
                        if(resync_lag_exceeded && (!has_warned_about_resync || resync_warning_timer.GetTime() >= 1.0))
                        {
                            Notify(msg_warning, "Realtime lag exceeded " + std::to_string(real_time_resync_lag) +
                                " seconds. Resynchronizing realtime clock instead of catching up missed ticks.");
                            resync_warning_timer.Restart();
                            has_warned_about_resync = true;
                        }
                        timer.SetTime(target_time);
                        lag = 0;
                    }
                }
                else if(run_mode.load() == run_mode_play)
                {
                    timer.SetStartTime(double(tick+1)*tick_duration); // Fake time increase
                    lag = 0;
                    if(!options_.is_set("batch_mode") || has_async_workers)
                        Sleep(0.01);
                }
                else
                    Sleep(0.01); // Wait 10 ms to avoid wasting cycles if there are no requests

                if(run_mode.load() == run_mode_realtime)
                {
                    static Timer lag_warning_timer;
                    if(lag > 1.0 && lag_warning_timer.GetTime() >= 1.0)
                    {
                        Notify(msg_warning, "Performance warning: System is " + std::to_string(lag) +  " seconds behind real time. Consider increasing tick_duration.");
                        lag_warning_timer.Restart();
                    }
                }

                // Run_mode may have changed during the delay - needs to be checked again

                if(run_mode.load() == run_mode_realtime || run_mode.load() == run_mode_play) 
                {
                    actual_tick_duration = intra_tick_timer.GetTime();
                    intra_tick_timer.Restart();
                    try
                    {
                        std::lock_guard<std::recursive_mutex> lock(kernelLock);
                        if(Tick() && socket != nullptr)
                            BuildUISnapshot(true);
                    }
                    catch(const std::exception & e)
                    {
                        Notify(msg_fatal_error, e.what());
                        break;
                    }
                    catch(...)
                    {
                        Notify(msg_fatal_error, "Unknown error during kernel execution.");
                        break;
                    }
                    tick_time_usage = intra_tick_timer.GetTime();
                    idle_time = std::max(0.0, tick_duration - tick_time_usage);
                }    
            }
            Stop();
            if(!options_.is_set("batch_mode"))
                Sleep(0.1);

        }
    }

        bool
        Kernel::Notify(int msg, std::string message, std::string path)
        {
            const std::string timestamped_message = "[" + TimeString(GetTime()) + "] " + message;
            {
                std::lock_guard<std::mutex> lock(log_mutex);
                const size_t max_retained_webui_log_messages = MaxRetainedWebUILogMessages();
                while(log.size() >= max_retained_webui_log_messages)
                {
                    log.erase(log.begin());
                    ++first_webui_log_sequence;
                }
                log.push_back(Message(msg, timestamped_message, path));
                ++next_webui_log_sequence;
            }

        std::cout << timestamped_message;
        if(!path.empty())
            std::cout  << " ("<<path << ")";
        std::cout << '\n';

        if(msg == msg_fatal_error || msg == msg_terminate)
        {
            if(options_.is_set("batch_mode"))
            {
                process_exit_code = (msg == msg_fatal_error) ? 1 : 0;
                global_terminate = true;
            }
            else
            {
                notify_stop_requested = true;
            }
        }
        else if(msg <= msg_end_of_file)
        {
            global_terminate = true;
        }
        return true;
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
    Kernel::SendImage(matrix & image, const std::string & format, int quality) // Compress image to jpg and return a base64 data URI
    {
        long size = 0;
        unsigned char * jpeg = nullptr;
        size_t output_length = 0 ;

        if(format=="rgb" && image.rank() == 3 && image.size(0) == 3)
            jpeg = (unsigned char *)create_color_jpeg(size, image, quality);

        else if(format=="gray" && image.rank() == 2)
            jpeg = (unsigned char *)create_gray_jpeg(size, image, 0, 1, quality);

        else if(image.rank() == 2) // taking our chances with the format...
            jpeg = (unsigned char *)create_pseudocolor_jpeg(size, image, 0, 1, format, quality);

            if(!jpeg)
            {
                return "\"\"";
            }

        char * jpeg_base64 = base64_encode(jpeg, size, &output_length);
        std::string result = "\"data:image/jpeg;base64,";
        result.append(jpeg_base64, output_length);
        result += "\"";
        destroy_jpeg(jpeg);
        free(jpeg_base64);
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
            StopComponents();
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


    void
    Kernel::SendStringResponse(dictionary header, const std::string & body, const char * response)
    {
        header["Content-Length"] = std::to_string(body.size());
        socket->SendHTTPHeader(header, response);
        socket->Append(body);
    }


    std::string
    Kernel::DoSendDataStatus()
    {
        std::ostringstream response;
        std::string nm;
        if(info_.contains_non_null("filename"))
            nm = std::string(info_["filename"]);
        else
            nm = options_.stem();

        if(!nm.empty())
            nm = std::filesystem::path(nm).filename().string();

        response << "\t\"file\": " << value(nm).json() << ",\n";

#if DEBUG
        response << "\t\"debug\": true,\n";
#else
        response << "\t\"debug\": false,\n";
#endif

        response << "\t\"state\": " << run_mode.load() << ",\n";
        if(stop_after != -1)
        {
            response << "\t\"tick\": \"" << tick << " / " << stop_after << "\",\n";
            response << "\t\"progress\": "
                     << (stop_after > 0 ? static_cast<double>(tick) / static_cast<double>(stop_after) : 0.0)
                     << ",\n";
        }
        else
        {
            response << "\t\"progress\": 0,\n";
        }

        // Timing information

        double uptime = uptime_timer.GetTime();
        double total_time = GetTime();

        response << "\t\"timestamp\": " << GetTimeStamp() << ",\n";
        response << "\t\"uptime\": " << uptime << ",\n";
        response << "\t\"tick_duration\": " << tick_duration << ",\n";
        response << "\t\"webui_req_int\": " << WebUIRequestInterval() << ",\n";
        response << "\t\"cpu_cores\": " << cpu_cores << ",\n";
    
        switch(run_mode)
        {
            case run_mode_stop:
                response << "\t\"tick\": \"-\",\n";
                response << "\t\"time\": \"-\",\n";
                response << "\t\"ticks_per_s\": \"-\",\n";
                response << "\t\"actual_duration\": \"-\",\n";
                response << "\t\"lag\": \"-\",\n";
                response << "\t\"time_usage\": 0,\n";
                response << "\t\"cpu_usage\": 0,\n";
                break;

            case run_mode_pause:
                response << "\t\"tick\": " << GetTick() << ",\n";
                response << "\t\"time\": " << GetTime() << ",\n";
                response << "\t\"ticks_per_s\": \"-\",\n";
                response << "\t\"actual_duration\": \"-\",\n";
                response << "\t\"lag\": \"-\",\n";
                response << "\t\"time_usage\": " << (actual_tick_duration> 0 ? tick_time_usage/actual_tick_duration : 0) << ",\n";
                response << "\t\"cpu_usage\": " << cpu_usage << ",\n";
                break;

            case run_mode_realtime:
            default:
                response << "\t\"tick\": " << GetTick() << ",\n";
                response << "\t\"time\": " << GetTime() << ",\n";
                response << "\t\"ticks_per_s\": " << (tick>0 ? double(tick)/total_time: 0) << ",\n";
                response << "\t\"actual_duration\": " << actual_tick_duration << ",\n";
                response << "\t\"lag\": " << lag << ",\n";
                response << "\t\"time_usage\": " << (actual_tick_duration> 0 ? tick_time_usage/actual_tick_duration : 0) << ",\n";
                response << "\t\"cpu_usage\": " << cpu_usage << ",\n";
                break;
        }

        response << "\t\"async\": {";
        std::string sep;
        for(auto & [path, component] : components)
        {
            if(!component->async_mode)
                continue;

            response << sep
                     << "\"" << escape_json_string(path) << "\": {"
                     << "\"running\": " << (component->IsAsyncRunning() ? "true" : "false") << ", "
                     << "\"failed\": " << (component->IsAsyncFailed() ? "true" : "false") << ", "
                     << "\"pending\": " << (component->IsAsyncPending() ? "true" : "false") << ", "
                     << "\"started_tick\": " << component->async_started_tick.load() << ", "
                     << "\"completed_tick\": " << component->async_completed_tick.load()
                     << "}";
            sep = ", ";
        }
        response << "},\n";

        return response.str();
    }


    std::string
    Kernel::NormalizeUIRoot(const std::string & component_path) const
    {
        std::string root = component_path;
        if(!root.empty() && root[0] == '.')
            root = root.substr(1);
        return root;
    }


    const parameter *
    Kernel::FindTopGroupParameter(const std::string & name) const
    {
        if(top_group_path.empty())
            return nullptr;

        auto it = parameters.find(top_group_path + "." + name);
        return it == parameters.end() ? nullptr : &it->second;
    }


    double
    Kernel::WebUIRequestInterval() const
    {
        if(const parameter * request_interval = FindTopGroupParameter("webui_req_int"))
        {
            try
            {
                return std::max(0.001, request_interval->as_double());
            }
            catch(const std::exception &)
            {
            }
        }
        return 0.1;
    }


    double
    Kernel::SnapshotInterval() const
    {
        if(const parameter * snapshot_interval = FindTopGroupParameter("snapshot_interval"))
        {
            try
            {
                return std::max(0.0, snapshot_interval->as_double());
            }
            catch(const std::exception &)
            {
            }
        }
        return 0.1;
    }


    size_t
    Kernel::MaxRetainedWebUILogMessages() const
    {
        if(const parameter * buffer_limit = FindTopGroupParameter("webui_log_buffer_limit"))
        {
            try
            {
                return static_cast<size_t>(std::max(1, buffer_limit->as_int()));
            }
            catch(const std::exception &)
            {
            }
        }
        return default_max_retained_webui_log_messages;
    }


    int
    Kernel::SnapshotJPEGQualityForFormat(const std::string & format) const
    {
        const char * parameter_name = format == "rgb" ? "rgb_quality" : "gray_quality";
        if(const parameter * quality_parameter = FindTopGroupParameter(parameter_name))
        {
            try
            {
                int quality = quality_parameter->as_int();
                return std::clamp(quality, 1, 100);
            }
            catch(const std::exception &)
            {
            }
        }
        return snapshot_jpeg_quality_for_format(format);
    }


    std::vector<Kernel::RequestedUIValue>
    Kernel::ParseRequestedUIValues(Request & request)
    {
        std::vector<RequestedUIValue> requested_values;
        std::string data;
        if(request.parameters.contains("data"))
            data = std::string(request.parameters["data"]);

        std::string root = NormalizeUIRoot(request.component_path);
        while(!data.empty())
        {
            std::string token = head(data, ",");
            if(token.empty())
                continue;

            std::string source = token;
            std::string format = rtail(source, ":");

            requested_values.push_back({
                root,
                token,
                token,
                source,
                format
            });
        }

        return requested_values;
    }


    Kernel::RequestedUIValue
    Kernel::ParseSubscribedUIValue(const std::string & subscription_key) const
    {
        RequestedUIValue requested_value;
        auto separator = subscription_key.find(ui_subscription_separator);
        if(separator == std::string::npos)
            requested_value.token = subscription_key;
        else
        {
            requested_value.root = subscription_key.substr(0, separator);
            requested_value.token = subscription_key.substr(separator + 1);
        }

        requested_value.key = requested_value.token;
        requested_value.source = requested_value.token;
        requested_value.format = rtail(requested_value.source, ":");
        return requested_value;
    }


    std::string
    Kernel::SubscriptionKeyFor(const RequestedUIValue & requested_value) const
    {
        return requested_value.root + ui_subscription_separator + requested_value.token;
    }


    bool
    Kernel::SerializeRequestedValue(RequestedUIValue requested_value, std::string & serialized_value, long long * compute_us, long long * value_us)
    {
        auto value_start = steady_clock::now();
        if((requested_value.source.find('@') != std::string::npos || requested_value.source.find('{') != std::string::npos) && components.count(requested_value.root) > 0)
        {
            auto compute_start = steady_clock::now();
            Component * component = components[requested_value.root].get();
            requested_value.source = component->ComputeValue(requested_value.source);
            if(compute_us)
                *compute_us += duration_cast<microseconds>(steady_clock::now() - compute_start).count();
        }

        std::string source_with_root = requested_value.root + "." + requested_value.source;
        if(!requested_value.source.empty() && requested_value.source[0] == '.')
            source_with_root = requested_value.source.substr(1);

        std::string component_path = peek_rhead(source_with_root, ".");
        std::string attribute = peek_rtail(source_with_root, ".");

        bool found_value = false;
        if(buffers.count(source_with_root) && !state_buffers.count(source_with_root))
        {
            if(ValueOwnedByRunningAsyncComponent(source_with_root))
                return false;

            if(requested_value.format.empty())
                serialized_value = buffers[source_with_root].json();
            else if(requested_value.format == "metadata")
                serialized_value = buffers[source_with_root].metadata_json();
            else if(is_snapshot_image_format(requested_value.format))
                serialized_value = SendImage(buffers[source_with_root], requested_value.format, SnapshotJPEGQualityForFormat(requested_value.format));
            found_value = !serialized_value.empty();
        }
        else if(parameters.count(source_with_root))
        {
            parameter & parameter_value = parameters[source_with_root];
            if(requested_value.format == "metadata" && parameter_value.get_type() == matrix_type)
            {
                const matrix & matrix_value = parameter_value.matrix_ref();
                serialized_value = matrix_value.metadata_json();
            }
            else
                serialized_value = parameter_value.json();
            found_value = true;
        }
        else if(components.count(component_path))
        {
            serialized_value = components[component_path]->json(attribute);
            found_value = !serialized_value.empty();
        }

        if(value_us)
            *value_us += duration_cast<microseconds>(steady_clock::now() - value_start).count();
        return found_value;
    }


    std::string
    Kernel::ConsumeLogForClient(long ui_client_id)
    {
        std::string response = ",\n\"log\": [";
        std::string sep;
        std::lock_guard<std::mutex> client_lock(ui_client_mutex);
        std::lock_guard<std::mutex> log_lock(log_mutex);

        auto & client_state = ui_client_states[ui_client_id];
        const uint64_t latest_sequence = next_webui_log_sequence - 1;

        if(!client_state.log_delivery_initialized)
        {
            client_state.delivered_log_sequence = first_webui_log_sequence - 1;
            client_state.log_delivery_initialized = true;
        }

        uint64_t next_sequence = client_state.delivered_log_sequence + 1;
        if(next_sequence < first_webui_log_sequence && next_sequence <= latest_sequence)
        {
            const uint64_t dropped_count = first_webui_log_sequence - next_sequence;
            const std::string dropped_message =
                "WebUI log truncated. Dropped " + std::to_string(dropped_count) +
                " older log message" + (dropped_count == 1 ? "" : "s") + " for this client.";
            response += Message(msg_warning, dropped_message).json();
            sep = ",";
            next_sequence = first_webui_log_sequence;
        }

        for(uint64_t sequence = next_sequence; sequence <= latest_sequence; ++sequence)
        {
            const size_t index = static_cast<size_t>(sequence - first_webui_log_sequence);
            response += sep + log[index].json();
            sep = ",";
        }

        client_state.delivered_log_sequence =
            std::max(client_state.delivered_log_sequence, latest_sequence);
        response += "]";
        return response;
    }

    void
    Kernel::ResetUISnapshotCache()
    {
        {
            std::lock_guard<std::mutex> lock(ui_snapshot_mutex);
            current_ui_snapshot.reset();
        }
        {
            const auto now = steady_clock::now();
            std::lock_guard<std::mutex> lock(ui_client_mutex);
            for(auto & client_entry : ui_client_states)
            {
                auto & client_state = client_entry.second;
                client_state.keys.clear();
                client_state.last_seen_time = now;
            }
            ++ui_subscription_revision;
        }
    }


    void
    Kernel::BuildUISnapshot(bool respect_rate_limit)
    {
        std::unordered_set<std::string> subscriptions;
        const auto now = steady_clock::now();
        std::shared_ptr<const UISnapshot> previous_snapshot;
        {
            std::lock_guard<std::mutex> lock(ui_snapshot_mutex);
            previous_snapshot = current_ui_snapshot;
        }

        bool has_active_clients = false;
        bool snapshot_due = !respect_rate_limit || previous_snapshot == nullptr;
        uint64_t subscription_revision = 0;
        {
            std::lock_guard<std::mutex> lock(ui_client_mutex);
            bool removed_client = false;
            for(auto it = ui_client_states.begin(); it != ui_client_states.end();)
            {
                if(now - it->second.last_seen_time > duration<double>(ui_subscription_timeout_seconds))
                {
                    it = ui_client_states.erase(it);
                    removed_client = true;
                }
                else
                    ++it;
            }

            if(removed_client)
                ++ui_subscription_revision;

            has_active_clients = !ui_client_states.empty();
            subscription_revision = ui_subscription_revision;
            const bool subscriptions_changed = previous_snapshot == nullptr ||
                previous_snapshot->subscription_revision != subscription_revision;
            if(subscriptions_changed)
                snapshot_due = true;
            else if(!snapshot_due)
                snapshot_due = now - previous_snapshot->timestamp >=
                    duration<double>(WebUIRequestInterval());

            if(snapshot_due)
                for(const auto & client_entry : ui_client_states)
                    subscriptions.insert(client_entry.second.keys.begin(), client_entry.second.keys.end());
        }

        if(!has_active_clients)
        {
            std::lock_guard<std::mutex> lock(ui_snapshot_mutex);
            current_ui_snapshot.reset();
            return;
        }

        if(!snapshot_due)
            return;

        bool refresh_images = previous_snapshot == nullptr ||
            now - previous_snapshot->image_timestamp >= duration<double>(SnapshotInterval());
        auto snapshot = std::make_shared<UISnapshot>();
        snapshot->snapshot_id = next_ui_snapshot_id++;
        snapshot->subscription_revision = subscription_revision;
        snapshot->session_id = session_id;
        snapshot->tick = tick;
        snapshot->image_timestamp = refresh_images ? now : (previous_snapshot ? previous_snapshot->image_timestamp : now);
        snapshot->status_json = DoSendDataStatus();

        std::vector<std::future<std::pair<std::string, std::string>>> image_futures;
        for(const auto & subscription_key : subscriptions)
        {
            RequestedUIValue requested_value = ParseSubscribedUIValue(subscription_key);
            if(is_snapshot_image_format(requested_value.format))
            {
                if(!refresh_images && previous_snapshot != nullptr)
                {
                    auto it = previous_snapshot->serialized_values.find(subscription_key);
                    if(it != previous_snapshot->serialized_values.end())
                    {
                        snapshot->serialized_values[subscription_key] = it->second;
                        continue;
                    }
                }

                image_futures.push_back(std::async(std::launch::async, [this, subscription_key, requested_value, previous_snapshot]() mutable
                {
                    std::string serialized_value;
                    if(SerializeRequestedValue(requested_value, serialized_value))
                        return std::make_pair(subscription_key, std::move(serialized_value));
                    if(previous_snapshot != nullptr)
                    {
                        auto it = previous_snapshot->serialized_values.find(subscription_key);
                        if(it != previous_snapshot->serialized_values.end())
                            return std::make_pair(subscription_key, it->second);
                    }
                    return std::make_pair(std::string(), std::string());
                }));
            }
            else
            {
                try
                {
                    std::string serialized_value;
                    if(SerializeRequestedValue(requested_value, serialized_value))
                        snapshot->serialized_values[subscription_key] = std::move(serialized_value);
                    else if(previous_snapshot != nullptr)
                    {
                        auto it = previous_snapshot->serialized_values.find(subscription_key);
                        if(it != previous_snapshot->serialized_values.end())
                            snapshot->serialized_values[subscription_key] = it->second;
                    }
                }
                catch(const std::exception & e)
                {
                    Notify(msg_warning, "Could not build UI snapshot for \"" + requested_value.token + "\": " + std::string(e.what()));
                }
            }
        }

        for(auto & future : image_futures)
        {
            try
            {
                auto result = future.get();
                if(!result.first.empty())
                    snapshot->serialized_values[result.first] = std::move(result.second);
            }
            catch(const std::exception & e)
            {
                Notify(msg_warning, "Could not build UI image snapshot: " + std::string(e.what()));
            }
        }

        snapshot->timestamp = steady_clock::now();
        std::lock_guard<std::mutex> lock(ui_snapshot_mutex);
        current_ui_snapshot = std::move(snapshot);
    }


    std::string
    Kernel::DoSendLog(Request & request)
    {
        return ConsumeLogForClient(request.client_id);
    }


    void
    Kernel::DoSendData(Request & request, bool refresh_paused_snapshot, bool use_snapshot_status)
    {
        auto requested_values = ParseRequestedUIValues(request);
        std::unordered_set<std::string> requested_subscriptions;
        requested_subscriptions.reserve(requested_values.size());
        for(const auto & requested_value : requested_values)
            requested_subscriptions.insert(SubscriptionKeyFor(requested_value));
        bool client_subscriptions_changed = false;
        {
            std::lock_guard<std::mutex> lock(ui_client_mutex);
            auto & client_state = ui_client_states[request.client_id];
            if(client_state.keys != requested_subscriptions)
            {
                ++ui_subscription_revision;
                client_subscriptions_changed = true;
            }
            client_state.keys = std::move(requested_subscriptions);
            client_state.last_seen_time = steady_clock::now();
        }

        if((refresh_paused_snapshot && run_mode.load() == run_mode_pause) ||
           (use_snapshot_status && client_subscriptions_changed))
        {
            std::lock_guard<std::recursive_mutex> lock(kernelLock);
            BuildUISnapshot();
        }

        std::shared_ptr<const UISnapshot> snapshot;
        {
            std::lock_guard<std::mutex> lock(ui_snapshot_mutex);
            snapshot = current_ui_snapshot;
        }

        long response_session_id = 0;
        std::string status;
        std::string log_json = ConsumeLogForClient(request.client_id);
        if(snapshot != nullptr)
        {
            response_session_id = snapshot->session_id;
            if(use_snapshot_status)
                status = snapshot->status_json;
        }

        std::vector<DataSnapshotItem> response_items;
        std::vector<std::pair<size_t, RequestedUIValue>> fallback_items;
        response_items.reserve(requested_values.size());

        for(const auto & requested_value : requested_values)
        {
            response_items.push_back({
                "\t\t\"" + escape_json_string(requested_value.key) + "\": ",
                ""
            });

            if(snapshot != nullptr)
            {
                auto it = snapshot->serialized_values.find(SubscriptionKeyFor(requested_value));
                if(it != snapshot->serialized_values.end())
                {
                    response_items.back().value = it->second;
                    continue;
                }
            }

            fallback_items.emplace_back(response_items.size() - 1, requested_value);
        }

        bool serialize_live_status = snapshot == nullptr || !use_snapshot_status;
        if(!fallback_items.empty() || serialize_live_status)
        {
            std::lock_guard<std::recursive_mutex> lock(kernelLock);
            if(snapshot == nullptr)
                response_session_id = session_id;
            if(serialize_live_status)
                status = DoSendDataStatus();

            for(const auto & fallback_item : fallback_items)
            {
                if(run_mode == run_mode_realtime && tick_duration > 0 && intra_tick_timer.GetTime() >= tick_duration)
                {
                    Notify(msg_debug, "Stopped sending data before next realtime tick.");
                    break;
                }

                try
                {
                    std::string serialized_value;
                    if(SerializeRequestedValue(fallback_item.second, serialized_value))
                        response_items[fallback_item.first].value = std::move(serialized_value);
                }
                catch(const std::exception & e)
                {
                    Notify(msg_warning, "Could not send data for \"" + fallback_item.second.key + "\": " + std::string(e.what()));
                }
            }
        }

        dictionary header({
            {"Session-Id", std::to_string(response_session_id)},
            {"Package-Type", "data"},
            {"Content-Type", "application/json"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"},
            {"Expires", "0"}
        });

        std::string response = "{\n";
        response += status;
        response += "\t\"data\":\n\t{\n";

        std::string sep;
        for(auto & item : response_items)
        {
            if(item.value.empty())
                continue;
            response += sep;
            response += item.prefix;
            response += item.value;
            sep = ",\n";
        }

        response += "\n\t}";
        response += log_json.empty() ? ",\n\"log\": []" : log_json;
        response += ",\n\t\"has_data\": 1\n";
        response += "}\n";
        SendStringResponse(header, response);
    }


    void
    Kernel::DoNew(Request & request)
    {
        New();
        DoUpdate(request); 
    }


    void
    Kernel::DoOpen(Request & request)
    {
        if(!request.parameters.contains("file"))
        {
            Notify(msg_warning, "No file specified.");
            DoSendNetwork(request);
            return;
        }

        std::string file = request.parameters["file"];
        std::string where = request.parameters.contains("where") ? std::string(request.parameters["where"]) : "";
        auto & files =
            where == "system" ? system_files :
            where == "examples" ? examples_files :
            user_files;
        auto file_path = files.find(file);
        std::filesystem::path open_path;
        if(file_path == files.end())
        {
            const std::filesystem::path root =
                where == "system" ? std::filesystem::path(options_.ikaros_root) / "Source/Modules" :
                where == "examples" ? std::filesystem::path(options_.ikaros_root) / "Examples" :
                std::filesystem::path(user_dir);
            if(!SanitizePathUnderRoot(root, add_extension(file, ".ikg"), open_path))
            {
                Notify(msg_warning, "File \""+file+"\" could not be found.");
                DoSendNetwork(request);
                return;
            }
        }
        else
        {
            open_path = file_path->second;
        }

        bool should_start = false;
        try
        {
            dictionary requested_file_info;
            LoadXMLWithRestrictedIncludes(requested_file_info, open_path);
            should_start = requested_file_info.is_set("start") || requested_file_info.is_set("real_time");
        }
        catch(...)
        {
            // LoadFile will report the detailed error below.
        }

        try
        {
            Stop();
            std::lock_guard<std::recursive_mutex> lock(kernelLock);
            if(components.size() > 0)
                Clear();
            options_.path_ = open_path.string();
            LoadFile();
            if(should_start && info_.is_set("real_time"))
                Realtime();
            else if(should_start)
                Play();
            else
            {
                info_.erase("start");
                info_.erase("real_time");
                run_mode = run_mode_stop;
                timer.Pause();
                timer.SetPauseTime(0);
            }
        }
        catch(const setup_failed& e)
        {
            Notify(msg_warning, "Could not set up file \""+file+"\": "+e.message(), e.path());
        }
        catch(const load_failed& e)
        {
            Notify(msg_warning, "Could not load file \""+file+"\": "+e.message(), e.path());
            New();
        }
        catch(const std::exception& e)
        {
            Notify(msg_warning, "Could not open file \""+file+"\": "+std::string(e.what()));
            New();
        }

        DoSendNetwork(request);
    }


    void
    Kernel::DoSave(Request & request) // Save data received from WebUI
    {
        dictionary d;
        try
        {
            if(!request.HasJsonBody())
                throw exception("Save request must include a JSON body.");
            if(!request.json_body.is_dictionary())
                throw exception("Save request body must be a JSON object.");
            d = dictionary(request.json_body).copy();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            std::cout << "INTERNAL ERROR: Could not parse json.\n" << request.body << '\n';
            DoSendError("400 Bad Request", "Save request body is not valid JSON.");
            return;
        }

        // Sanitize file name

        if(!d.contains_non_null("filename") || std::string(d["filename"]).empty())
        {
            DoSendError("400 Bad Request", "Save request must include a filename.");
            return;
        }

        try
        {
            std::filesystem::path path = add_extension(std::string(d["filename"]), ".ikg");
            std::filesystem::path filename = path.filename();
            if(filename.empty() || filename == "." || filename == ".." || filename.stem().empty())
            {
                DoSendError("400 Bad Request", "Save request filename is invalid.");
                return;
            }
            std::filesystem::path target_path = std::filesystem::path(user_dir) / filename;

            d.erase("filename");
            std::string data = canonicalize_shape_aliases(d.xml("group", {"module/parameters","module/inputs","module/outputs","module/states", "module/authors","module/descriptions", "group/views", "module.description"}));
            std::error_code ec;
            std::filesystem::create_directories(target_path.parent_path(), ec);
            if(ec)
            {
                DoSendError("500 Internal Server Error", "Could not create save directory \"" + target_path.parent_path().string() + "\".");
                return;
            }

            std::ofstream file(target_path);
            if(!file)
            {
                DoSendError("500 Internal Server Error", "Could not save file \"" + target_path.string() + "\".");
                return;
            }
            file << data;
            file.close();
            if(!file)
            {
                DoSendError("500 Internal Server Error", "Could not finish writing file \"" + target_path.string() + "\".");
                return;
            }

            options_.path_ = target_path.string();
            needs_reload = true;

            d["filename"] = filename.stem().string();
            info_ = d;

            std::cout << "Saved file \"" << target_path.string() << "\".\n";
            std::string response = "{\n";
            response += "\t\"ok\": true,\n";
            response += "\t\"filename\": " + value(filename.stem().string()).json() + "\n";
            response += "}\n";
            dictionary header({
                {"Session-Id", std::to_string(session_id)},
                {"Package-Type", "save"},
                {"Content-Type", "application/json"},
                {"Cache-Control", "no-cache, no-store"},
                {"Pragma", "no-cache"}
            });
            SendStringResponse(header, response);
        }
        catch(const std::exception & e)
        {
            DoSendError("500 Internal Server Error", "Could not save file: " + std::string(e.what()));
        }
    }


    void
    Kernel::DoSaveState(Request & request)
    {
        try
        {
            std::string filename = resolve_state_filename_from_request(request, user_dir, options_, "save_state");
            std::string component_path = component_path_from_request(request);
            {
                std::lock_guard<std::recursive_mutex> lock(kernelLock);
                if(components.empty())
                    throw exception("No network is loaded.");
                SaveState(filename, component_path);
            }

            std::string response = "{\n";
            response += "\t\"ok\": true,\n";
            response += "\t\"filename\": " + value(filename).json() + ",\n";
            response += "\t\"module\": " + value(component_path).json() + "\n";
            response += "}\n";
            dictionary header({
                {"Session-Id", std::to_string(session_id)},
                {"Package-Type", "savestate"},
                {"Content-Type", "application/json"},
                {"Cache-Control", "no-cache, no-store"},
                {"Pragma", "no-cache"}
            });
            SendStringResponse(header, response);
        }
        catch(const std::exception & e)
        {
            DoSendError("500 Internal Server Error", "Could not save state: " + std::string(e.what()));
        }
    }


    void
    Kernel::DoLoadState(Request & request)
    {
        try
        {
            std::string filename = resolve_state_filename_from_request(request, user_dir, options_, "load_state");
            std::string component_path = component_path_from_request(request);
            {
                std::lock_guard<std::recursive_mutex> lock(kernelLock);
                if(components.empty())
                    throw exception("No network is loaded.");
                LoadState(filename, component_path);
                BuildUISnapshot();
            }

            std::string response = "{\n";
            response += "\t\"ok\": true,\n";
            response += "\t\"filename\": " + value(filename).json() + ",\n";
            response += "\t\"module\": " + value(component_path).json() + "\n";
            response += "}\n";
            dictionary header({
                {"Session-Id", std::to_string(session_id)},
                {"Package-Type", "loadstate"},
                {"Content-Type", "application/json"},
                {"Cache-Control", "no-cache, no-store"},
                {"Pragma", "no-cache"}
            });
            SendStringResponse(header, response);
        }
        catch(const std::exception & e)
        {
            DoSendError("500 Internal Server Error", "Could not load state: " + std::string(e.what()));
        }
    }


    void
    Kernel::DoResetState(Request & request)
    {
        try
        {
            std::string component_path = component_path_from_request(request);

            {
                std::lock_guard<std::recursive_mutex> lock(kernelLock);
                if(components.empty())
                    throw exception("No network is loaded.");
                ResetState(component_path);
                BuildUISnapshot();
            }

            std::string response = "{\n";
            response += "\t\"ok\": true,\n";
            response += "\t\"module\": " + value(component_path).json() + "\n";
            response += "}\n";
            dictionary header({
                {"Session-Id", std::to_string(session_id)},
                {"Package-Type", "resetstate"},
                {"Content-Type", "application/json"},
                {"Cache-Control", "no-cache, no-store"},
                {"Pragma", "no-cache"}
            });
            SendStringResponse(header, response);
        }
        catch(const std::exception & e)
        {
            DoSendError("500 Internal Server Error", "Could not reset state: " + std::string(e.what()));
        }
    }


    void
    Kernel::DoQuit(Request & request)
    {
        Notify(msg_print, "quit");
        Stop();
        {
            std::lock_guard<std::recursive_mutex> lock(kernelLock);
            run_mode = run_mode_quit;
        }
        DoUpdate(request);
    }


    void
    Kernel::DoStop(Request & request)
    {
        // td::cout << "Kernel::DoStop" << std::endl;
        Notify(msg_print, "stop");
        Stop();
        DoUpdate(request);
    }


    bool
    Kernel::SanitizeProjectPath(const std::filesystem::path & candidate_path, std::filesystem::path & sanitized_path) const
    {
        if(candidate_path.empty())
            return false;

        std::error_code ec;
        std::filesystem::path project_root = std::filesystem::weakly_canonical(options_.ikaros_root, ec);
        if(ec)
            return false;

        std::filesystem::path resolved_path = candidate_path.is_absolute() ? candidate_path : project_root / candidate_path;
        resolved_path = std::filesystem::weakly_canonical(resolved_path, ec);
        if(ec)
            return false;

        auto root_it = project_root.begin();
        auto root_end = project_root.end();
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


    bool
    Kernel::SanitizePathUnderRoot(const std::filesystem::path & root, const std::filesystem::path & candidate_path, std::filesystem::path & sanitized_path) const
    {
        if(root.empty() || candidate_path.empty() || candidate_path.is_absolute())
            return false;

        std::error_code ec;
        std::filesystem::path resolved_root = std::filesystem::weakly_canonical(root, ec);
        if(ec)
            return false;

        std::filesystem::path resolved_path = std::filesystem::weakly_canonical(resolved_root / candidate_path, ec);
        if(ec)
            return false;

        auto root_it = resolved_root.begin();
        auto root_end = resolved_root.end();
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


    bool
    Kernel::SanitizeImportPath(const std::filesystem::path & candidate_path, std::filesystem::path & sanitized_path) const
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
        {
            std::filesystem::path current_network = options_.full_path();
            if(!current_network.empty())
                base_path = current_network.parent_path() / candidate_path;
            else
                base_path = std::filesystem::current_path() / candidate_path;
        }

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


    void
    Kernel::LoadXMLWithRestrictedIncludes(dictionary & d, const std::filesystem::path & filename) const
    {
        std::vector<std::filesystem::path> include_roots;
        include_roots.push_back(options_.ikaros_root);
        include_roots.push_back(user_dir);

        std::error_code ec;
        std::filesystem::path resolved_file = std::filesystem::weakly_canonical(filename, ec);
        if(!ec && !resolved_file.parent_path().empty())
            include_roots.push_back(resolved_file.parent_path());

        d.load_xml(filename.string(), include_roots);
    }


    Kernel::SendFileResult
    Kernel::SendFileIfSafe(const std::filesystem::path & root, const std::string & file)
    {
        std::filesystem::path sanitized_path;
        if(!SanitizeProjectPath(root / file, sanitized_path))
            return SendFileResult::forbidden;

        std::error_code ec;
        if(!std::filesystem::is_regular_file(sanitized_path, ec) || ec)
            return SendFileResult::not_found;

        return socket->SendFile(sanitized_path) ? SendFileResult::sent : SendFileResult::not_found;
    }


    Kernel::SendFileResult
    Kernel::SendPublicWebUIFileIfSafe(const std::filesystem::path & root, const std::string & file)
    {
        std::filesystem::path sanitized_path;
        if(!SanitizePathUnderRoot(root, std::filesystem::path(file), sanitized_path))
            return SendFileResult::forbidden;

        std::error_code ec;
        if(!std::filesystem::is_regular_file(sanitized_path, ec) || ec)
            return SendFileResult::not_found;

        return socket->SendFile(sanitized_path) ? SendFileResult::sent : SendFileResult::not_found;
    }


    void
    Kernel::DoSendFile(std::string file)
    {
        if(file.empty())
            return;

        if(file[0] == '/')
            file = file.erase(0,1); // Remove initial slash

        // if(socket->SendFile(file, ikc_dir))  // Check IKC-directory first to allow files to be overriden
        //    return;

        //std::cout << "Sending file: " << file << std::endl;

        bool forbidden = false;
        auto try_send_file = [this, &file, &forbidden](const std::filesystem::path & root)
        {
            SendFileResult result = SendFileIfSafe(root, file);
            if(result == SendFileResult::forbidden)
                forbidden = true;
            return result == SendFileResult::sent;
        };

        if(try_send_file(user_dir))   // Look in user directory
            return;

        if(try_send_file(webui_dir))   // Now look in WebUI directory
            return;

        if(try_send_file(std::filesystem::path(webui_dir) / "Images"))   // Now look in WebUI/Images directory
            return;

        if(try_send_file(std::filesystem::path(webui_dir) / ".."))   // Now look in Source directory
            return;

        if(forbidden)
            DoSendError("403 Forbidden", "403 Forbidden\n");
        else
            DoSendError("404 Not Found", "404 Not Found\n");

    

        /*
 
        
        file = "error." + rcut(file, ".");
        if(socket->SendFile("error." + rcut(file, "."), webui_dir)) // Try to send error file
            return;

        DoSendError();
        */
    }


    void
    Kernel::DoSendPublicWebUIFile(std::string file)
    {
        if(file.empty() || file == "/")
            file = "index.html";

        if(file[0] == '/')
            file = file.erase(0,1);

        bool forbidden = false;
        auto try_send_public_file = [this, &forbidden](const std::filesystem::path & root, const std::string & requested_file)
        {
            SendFileResult result = SendPublicWebUIFileIfSafe(root, requested_file);
            if(result == SendFileResult::forbidden)
                forbidden = true;
            return result == SendFileResult::sent;
        };

        if(try_send_public_file(webui_dir, file))
            return;

        if(starts_with(file, "images/"))
        {
            std::string rewritten = "Images/" + file.substr(7);
            if(try_send_public_file(webui_dir, rewritten))
                return;
        }
        else if(starts_with(file, "models/"))
        {
            std::string rewritten = "Models/" + file.substr(7);
            if(try_send_public_file(webui_dir, rewritten))
                return;
        }

        if(forbidden)
            DoSendError("403 Forbidden", "403 Forbidden\n");
        else
            DoSendError("404 Not Found", "404 Not Found\n");
    }


    void
    Kernel::DoSendNetwork(Request & request)
    {
        std::string s = json();
        {
            std::lock_guard<std::mutex> lock(ui_client_mutex);
            ui_client_states[request.client_id].last_seen_time = steady_clock::now();
        }

        std::string log_json = ConsumeLogForClient(request.client_id);
        if(s.size() > 0 && s.back() == '}')
        {
            s.pop_back();
            s += log_json;
            s += "\n}";
        }

        //std::cout << s << std::endl;

        dictionary rtheader({
            {"Session-Id", std::to_string(session_id)},
            {"Package-Type", "network"},
            {"Content-Type", "application/json"},
            {"Content-Length", std::to_string(s.size())}
        });
        SendStringResponse(rtheader, s);
    }


    void
    Kernel::DoPause(Request & request)
    {
        Notify(msg_print, "pause");
        try
        {
            Pause();
        }
        catch(const exception& e)
        {
            Notify(msg_warning, e.what(), e.path());
        }
        DoSendData(request);
    }



    void
    Kernel::DoStep(Request & request)
    {
        Notify(msg_print, "step");
        bool tick_succeeded = false;
        try
        {
            Pause();
            run_mode = run_mode_pause;
            tick_succeeded = Tick();
            if(tick_succeeded)
                BuildUISnapshot();
            timer.SetPauseTime(GetTime()+tick_duration);
        }
        catch(const fatal_runtime_error & e)
        {
            needs_reload = true;
            Notify(msg_fatal_error, e.what(), e.path());
            Stop();
        }
        catch(const exception& e)
        {
            Notify(msg_warning, e.what(), e.path());
        }
        DoSendData(request, tick_succeeded);
    }



    void
    Kernel::DoRealtime(Request & request)
    {
        Notify(msg_print, "realtime");
        try
        {
            Realtime();
        }
        catch(const exception& e)
        {
             Notify(msg_warning, e.what(), e.path());
        }
        DoSendData(request);
    }


    void
    Kernel::DoPlay(Request & request)
    {
        Notify(msg_print, "play");
        try
        {
            Play();

        }
        catch(const exception& e)
        {
            Notify(msg_warning, e.what(), e.path());
        }
        DoSendData(request);
    }



    void
    Kernel::DoData(Request & request)
    {
        dictionary header({
            {"Content-Type", "text/plain"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        std::string key = normalize_request_value_path(request.component_path);

        std::string body;
        if(!buffers.count(key) || state_buffers.count(key))
        {
            body = "Buffer \""+request.component_path+"\" can not be found";
            SendStringResponse(header, body);
            return;
        }
        if(ValueOwnedByRunningAsyncComponent(key))
        {
            body = "Buffer \"" + request.component_path + "\" is currently being updated asynchronously";
            SendStringResponse(header, body);
            return;
        }
        if(buffers[key].rank() > 2)
        {
            body = "Rank of matrix != 2. Cannot be displayed as CSV";
            SendStringResponse(header, body);
            return;
        }
        SendStringResponse(header, buffers[key].csv());
    }



    void
    Kernel::DoJSON(Request & request)
    {
        std::string key = normalize_request_value_path(request.component_path);
        std::string format = rtail(key, ":");

        dictionary header({
            {"Content-Type", "application/json; charset=utf-8"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        std::string body;

        auto send_json_response = [&](const std::string & json_value, const std::vector<int> & shape)
        {
            body = "{\"path\": ";
            body += value(request.component_path).json();
            body += ", \"shape\": [";

            std::string sep;
            for(int dim : shape)
            {
                body += sep;
                body += std::to_string(dim);
                sep = ", ";
            }

            body += "], \"value\": ";
            body += json_value;
            body += "}";
        };

        if(buffers.count(key) && !state_buffers.count(key))
        {
            if(ValueOwnedByRunningAsyncComponent(key))
            {
                dictionary error;
                error["error"] = "Value \"" + request.component_path + "\" is currently being updated asynchronously";
                SendStringResponse(header, error.json());
                return;
            }
            send_json_response(format == "metadata" ? buffers[key].metadata_json() : buffers[key].json(), buffers[key].shape());
            SendStringResponse(header, body);
            return;
        }

        if(parameters.count(key))
        {
            parameter & parameter_value = parameters[key];
            if(parameter_value.get_type() == matrix_type)
            {
                const matrix & matrix_value = parameter_value.matrix_ref();
                send_json_response(format == "metadata" ? matrix_value.metadata_json() : parameter_value.json(), matrix_value.shape());
            }
            else
                send_json_response(parameter_value.json(), {});
            SendStringResponse(header, body);
            return;
        }

        std::string component_path = peek_rhead(key, ".");
        std::string attribute = peek_rtail(key, ".");

        if(components.count(component_path))
        {
            std::string json_data = components[component_path]->json(attribute);
            if(!json_data.empty())
            {
                send_json_response(json_data, {});
                SendStringResponse(header, body);
                return;
            }
        }

        dictionary error;
        error["error"] = "Value \"" + request.component_path + "\" can not be found";
        SendStringResponse(header, error.json());
    }



    void
    Kernel::DoCSV(Request & request)
    {
        dictionary header({
            {"Content-Type", "text/csv; charset=utf-8"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        std::string key = normalize_request_value_path(request.component_path);

        if(!buffers.count(key) || state_buffers.count(key))
        {
            SendStringResponse(header, "Buffer \""+request.component_path+"\" can not be found");
            return;
        }
        if(ValueOwnedByRunningAsyncComponent(key))
        {
            SendStringResponse(header, "Buffer \"" + request.component_path + "\" is currently being updated asynchronously");
            return;
        }
        if(buffers[key].rank() > 2)
        {
            SendStringResponse(header, "Rank of matrix != 2. Cannot be displayed as CSV");
            return;
        }
        SendStringResponse(header, buffers[key].csv());
    }



    void
    Kernel::DoImage(Request & request)
    {
        std::string key = normalize_request_value_path(request.component_path);

        matrix * image = nullptr;

        if(buffers.count(key) && !state_buffers.count(key))
        {
            if(ValueOwnedByRunningAsyncComponent(key))
            {
                dictionary header({
                    {"Content-Type", "text/plain"},
                    {"Cache-Control", "no-cache, no-store"},
                    {"Pragma", "no-cache"}
                });
                SendStringResponse(header, "Matrix \"" + request.component_path + "\" is currently being updated asynchronously");
                return;
            }
            image = &buffers[key];
        }
        else if(parameters.count(key) && parameters[key].get_type() == matrix_type)
        {
            image = &parameters[key].matrix_ref();
        }

        if(!image)
        {
            dictionary header({
                {"Content-Type", "text/plain"},
                {"Cache-Control", "no-cache, no-store"},
                {"Pragma", "no-cache"}
            });
            SendStringResponse(header, "Matrix \"" + request.component_path + "\" can not be found");
            return;
        }

        long size = 0;
        unsigned char * jpeg = nullptr;

        if(image->rank() == 2)
            jpeg = create_gray_jpeg(size, *image, 0, 1, 90);
        else if(image->rank() == 3 && image->size(0) == 3)
            jpeg = create_color_jpeg(size, *image, 90);

        if(!jpeg)
        {
            dictionary header({
                {"Content-Type", "text/plain"},
                {"Cache-Control", "no-cache, no-store"},
                {"Pragma", "no-cache"}
            });
            SendStringResponse(header, "Matrix \"" + request.component_path + "\" can not be converted to JPEG");
            return;
        }

        dictionary header({
            {"Content-Type", "image/jpeg"},
            {"Content-Length", std::to_string(size)},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        socket->SendHTTPHeader(header);
        socket->Flush();
        socket->SendData(reinterpret_cast<const char *>(jpeg), size);
        destroy_jpeg(jpeg);
    }



    void
    Kernel::DoProfiling(Request & request)
    {
        bool active = true;
        if(request.parameters.contains("active"))
            active = request.parameters.is_set("active");
        SetProfilingClientActive(request.client_id, active);

        dictionary header({
            {"Content-Type", "application/json; charset=utf-8"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        SendStringResponse(header, GetProfilingJSON());
    }


    void
    Kernel::DoStartupSteps(Request &)
    {
        dictionary header({
            {"Content-Type", "application/json; charset=utf-8"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        SendStringResponse(header, GetStartupStepsJSON());
    }



    void
    Kernel::DoCommand(Request & request)
    {
        try
        {
            request.MergeJsonBodyIntoParameters();

            std::string key = normalize_request_value_path(request.component_path);

            std::lock_guard<std::recursive_mutex> lock(kernelLock);

            if(!components.count(key))
            {
                Notify(msg_warning, "Component '"+request.component_path+"' could not be found.");
            }
            else if(!request.parameters.contains("command"))
            {
                Notify(msg_warning, "No command specified for  '"+request.component_path+"'.");
            }
            else if(components.at(key)->IsAsyncRunning())
            {
                std::string command_name = request.parameters["command"];
                components.at(key)->QueueDeferredCommand(command_name, request.parameters);
                Notify(msg_print, "Queued command \"" + command_name + "\" for asynchronous component \"" + key + "\".", key);
            }
            else
            {
                components.at(key)->Command(request.parameters["command"], request.parameters);
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        DoSendData(request);
    }



    void
    Kernel::DoControl(Request & request)
    {
        try
        {
            request.MergeJsonBodyIntoParameters();

            std::string key = normalize_request_value_path(request.component_path);

            std::lock_guard<std::recursive_mutex> lock(kernelLock);

            if(!parameters.count(key))
            {
                Notify(msg_warning, "Parameter '"+request.component_path+"' could not be found.");
            }
            else
            {
                if(Component * component = ComponentForValuePath(key); component != nullptr && component->IsAsyncRunning())
                {
                    Component::DeferredParameterChange change;
                    change.parameter_path = key;
                    change.value = "1";
                    if(request.parameters.contains("value"))
                        change.value = std::string(request.parameters["value"]);

                    parameter & p = parameters.at(key);
                    if(p.get_type() == matrix_type)
                    {
                        change.is_matrix_cell = true;
                        if(request.parameters.contains("x"))
                            change.x = request.parameters["x"];
                        if(request.parameters.contains("y"))
                            change.y = request.parameters["y"];
                    }

                    component->QueueDeferredParameterChange(change);
                    Notify(msg_print, "Queued parameter change for asynchronous component \"" + component->path_ + "\".", component->path_);
                    DoSendData(request);
                    return;
                }

                parameter & p = parameters.at(key);
                if(p.get_type() == matrix_type)
                {
                    int x = 0;
                    int y = 0;
                    double value = 1;

                    if(request.parameters.contains("x"))
                        x = request.parameters["x"];

                    if(request.parameters.contains("y"))
                        y = request.parameters["y"];
                    
                    if(request.parameters.contains("value"))
                        value = request.parameters["value"];

                    matrix & matrix_value = p.matrix_ref();
                    if(matrix_value.rank() == 1)
                        matrix_value(x)= value;
                    else if(matrix_value.rank() == 2)
                        matrix_value(y,x)= value; // Is this correct?
                    else
                        throw exception("Higher-dimensional matrix parameters are not supported by /control.");
                }
                else
                {
                    SetParameter(key, std::string(request.parameters["value"]));
                }
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';

        }
        DoSendData(request);
    }


    void
    Kernel::DoUpdate(Request & request)
    {
        long current_session_id = 0;
        {
            std::lock_guard<std::mutex> lock(ui_snapshot_mutex);
            if(current_ui_snapshot)
                current_session_id = current_ui_snapshot->session_id;
        }
        if(current_session_id == 0)
        {
            std::lock_guard<std::recursive_mutex> lock(kernelLock);
            current_session_id = session_id;
        }

        if(request.session_id != current_session_id)
        {
            std::lock_guard<std::recursive_mutex> lock(kernelLock);
            DoSendNetwork(request);
        }
        else
            DoSendData(request, true, true);
    }


    void
    Kernel::DoNetwork(Request & request)
    {
        DoSendNetwork(request);
    }


    void
    Kernel::DoSendClasses(Request &)
    {
        dictionary header({
            {"Content-Type", "application/json; charset=utf-8"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        std::string body = "{\"classes\":[\n\t\"";
        std::string s = "";
        for(auto & [name, component_class]: classes)
        {
            if(is_internal(component_class.info_))
                continue;

            body += s;
            body += escape_json_string(name);
            s = "\",\n\t\"";
        }
        body += "\"\n]\n}\n";
        SendStringResponse(header, body);
    }



    void
    Kernel::DoSendClassInfo(Request &)
    {
        dictionary header({
            {"Content-Type", "application/json; charset=utf-8"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        std::string body = "{\n";
        std::string s = "";
        for(auto & [name, component_class]: classes)
        {
            if(is_internal(component_class.info_))
                continue;

            body += s;
            body += "\""+escape_json_string(name)+"\": ";
            dictionary class_info = component_class.info_.copy();
            class_info["path"] = std::filesystem::path(component_class.path).parent_path().string();
            body += class_info.json();
            s = ",\n\t";
        }
        body += "\n}\n";
        SendStringResponse(header, body);
    }



    void
    Kernel::DoSendClassReadMe(Request & request)
    {
        dictionary header({
            {"Content-Type", "text/plain; charset=utf-8"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        std::string body;

        if(!request.parameters.contains("class"))
        {
            body = "No class selected.";
            SendStringResponse(header, body);
            return;
        }

        std::string class_name = request.parameters["class"];
        if(!classes.count(class_name))
        {
            body = "Class not found: " + class_name;
            SendStringResponse(header, body);
            return;
        }

        std::filesystem::path class_path = classes[class_name].path;
        if(class_path.empty())
        {
            body = "No class path available for: " + class_name;
            SendStringResponse(header, body);
            return;
        }

        std::filesystem::path readme_path = class_path.parent_path() / "ReadMe.md";
        std::ifstream readme_file(readme_path);
        if(!readme_file.is_open())
        {
            body = "No ReadMe.md found for: " + class_name;
            SendStringResponse(header, body);
            return;
        }

        body.assign((std::istreambuf_iterator<char>(readme_file)), std::istreambuf_iterator<char>());
        SendStringResponse(header, body);
    }



    void
    Kernel::DoSendFileList(Request &)
    {
        // Scan for files

        system_files.clear();
        examples_files.clear();
        user_files.clear();
        user_state_files.clear();
        ScanFiles(options_.ikaros_root+"/Source/Modules");
        ScanFiles(options_.ikaros_root+"/Examples", false, true);
        ScanFiles(user_dir, false);

        // Send result

        dictionary header({
            {"Content-Type", "application/json; charset=utf-8"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        std::string sep;
        std::string body = "{\"system_files\":[\n\t\"";
        for(auto & [name, path]: system_files)
        {
            (void)path;
            body += sep;
            body += escape_json_string(name);
            sep = "\",\n\t\"";
        }
        body += "\"\n],\n";
    
        sep = "";
        body += "\"examples_files\":[\n\t\"";
        for(auto & [name, path]: examples_files)
        {
            (void)path;
            body += sep;
            body += escape_json_string(name);
            sep = "\",\n\t\"";
        }
        body += "\"\n],\n";

        sep = "";
        body += "\"user_files\":[\n\t\"";
        for(auto & [name, path]: user_files)
        {
            (void)path;
            body += sep;
            body += escape_json_string(name);
            sep = "\",\n\t\"";
        }
        body += "\"\n],\n";

        sep = "";
        body += "\"user_state_files\":[\n\t\"";
        for(auto & [name, path]: user_state_files)
        {
            (void)path;
            body += sep;
            body += escape_json_string(name);
            sep = "\",\n\t\"";
        }
        body += "\"\n]\n";
        body += "}\n";
        SendStringResponse(header, body);
    }


    void
    Kernel::DoSendError(const std::string & status, const std::string & message)
    {
    dictionary header({
        {"Content-Type", "text/plain"},
    });
    SendStringResponse(header, message, status.c_str());
    }


    void
    Kernel::DoUnauthorized()
    {
        dictionary header({
            {"Content-Type", "text/plain"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"},
            {"Expires", "0"}
        });
        SendStringResponse(header, "401 Unauthorized\n", "401 Unauthorized");
    }


    void
    Kernel::DoAuthStatus()
    {
        dictionary header({
            {"Content-Type", "application/json"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"},
            {"Expires", "0"}
        });
        std::string body =
            "{\n"
            "\t\"enabled\": " + std::string(auth_enabled_ ? "true" : "false") + ",\n"
            "\t\"authenticated\": " + std::string((!auth_enabled_ || IsRequestAuthenticated()) ? "true" : "false") + "\n"
            "}\n";
        SendStringResponse(header, body);
    }


    void
    Kernel::DoLogin(Request & request)
    {
        if(!auth_enabled_)
        {
            DoAuthStatus();
            return;
        }

        try
        {
            if(!request.HasJsonBody())
                throw exception("Login request must include a JSON body.");
            if(!request.json_body.is_dictionary())
                throw exception("Login request body must be a JSON object.");

            dictionary login_request(request.json_body);
            std::string candidate_password =
                login_request.contains_non_null("password") ? std::string(login_request["password"]) : "";
            if(!CheckPassword(candidate_password))
            {
                DoUnauthorized();
                return;
            }

            std::string token = CreateSessionToken();
            if(token.empty())
            {
                DoUnauthorized();
                return;
            }

            dictionary header({
                {"Content-Type", "application/json"},
                {"Set-Cookie", "ikaros_session=" + token + "; Path=/; Max-Age=2592000; HttpOnly; SameSite=Strict"},
                {"Cache-Control", "no-cache, no-store"},
                {"Pragma", "no-cache"},
                {"Expires", "0"}
            });
            SendStringResponse(header, "{\n\t\"authenticated\": true\n}\n");
        }
        catch(const std::exception &)
        {
            DoUnauthorized();
        }
    }


    void
    Kernel::HandleHTTPRequest()
    {
        long sid = 0;
        if(socket->header.contains_non_null("session-id"))
            sid = atol(std::string(socket->header["session-id"]).c_str());

        long cid = 0;
        if(socket->header.contains_non_null("client-id"))
            cid = atol(std::string(socket->header["client-id"]).c_str());

        std::string content_type;
        if(socket->header.contains_non_null("content-type"))
            content_type = std::string(socket->header["content-type"]);

        std::optional<Request> parsed_request;
        try
        {
            parsed_request.emplace(std::string(socket->header["uri"]), sid, socket->body, content_type, cid);
        }
        catch(const std::exception & e)
        {
            DoSendError("400 Bad Request", e.what());
            return;
        }
        Request & request = *parsed_request;

        if(request.parameters.contains("proxy"))
            request.component_path = std::string(request.parameters["proxy"]);

        if(request == "auth")
        {
            DoAuthStatus();
            return;
        }
        else if(request == "login")
        {
            DoLogin(request);
            return;
        }
        else if(auth_enabled_ && !IsRequestAuthenticated())
        {
            if(IsPublicRequest(request))
            {
                DoSendPublicWebUIFile(request.url);
                return;
            }

            DoUnauthorized();
            return;
        }

        // std::cout << "Request: " << request.url << std::endl;

        if(request == "network")
            DoNetwork(request);

        else if(request == "update")
            DoUpdate(request);

        // Run mode commands

        else if(request == "quit")
            DoQuit(request);
        else if(request == "stop")
            DoStop(request);
        else if(request == "pause")
            DoPause(request);
        else if(request == "step")
            DoStep(request);
        else if(request == "play")
            DoPlay(request);
        else if(request == "realtime")
            DoRealtime(request);

        // File handling commands

        else if(request == "new")
            DoNew(request);
        else if(request == "open")
            DoOpen(request);
        else if(request == "save")
            DoSave(request);
        else if(request == "savestate" || request == "save_state")
            DoSaveState(request);
        else if(request == "loadstate" || request == "load_state")
            DoLoadState(request);
        else if(request == "resetstate" || request == "reset_state")
            DoResetState(request);

        // Start up commands

        else if(request == "classes") 
            DoSendClasses(request);
        else if(request == "classinfo") 
            DoSendClassInfo(request);
        else if(request == "classreadme")
            DoSendClassReadMe(request);
        else if(request == "files") 
            DoSendFileList(request);
        else if(request == "")
            DoSendFile("index.html");

        else if(request == "data")
            DoData(request);
        else if(request == "json")
            DoJSON(request);
        else if(request == "csv")
            DoCSV(request);
        else if(request == "image")
            DoImage(request);
        else if(request == "profiling")
            DoProfiling(request);
        else if(request == "startupsteps")
            DoStartupSteps(request);

        // Control commands

        else if(request == "command")
            DoCommand(request);
        else if(request == "control")
            DoControl(request);
        else 
            DoSendFile(request.url);
    }



double
Kernel::GetTimeOfDay()
{
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


    void
    Kernel::HandleHTTPThread()
    {
        while(!shutdown)
        {
            try
            {
                if(socket != nullptr && socket->QueueRequest(true))
                {
                    ServerSocket::QueuedRequest queued_request;
                    if(!socket->PopRequest(queued_request, false))
                        continue;

                    socket->ActivateRequest(queued_request);
                    std::string method = queued_request.header.contains_non_null("method") ? std::string(queued_request.header["method"]) : "";
                    std::string uri = queued_request.header.contains_non_null("uri") ? std::string(queued_request.header["uri"]) : "";
                    bool is_update_request = uri.find("/update") != std::string::npos;
                    bool waits_before_locking =
                        request_path_matches_command(uri, "quit") ||
                        request_path_matches_command(uri, "stop") ||
                        request_path_matches_command(uri, "new") ||
                        request_path_matches_command(uri, "open");

                    if((is_update_request || waits_before_locking) && (method == "GET" || method == "PUT"))
                    {
                        HandleHTTPRequest();
                    }
                    else
                    {
                        std::lock_guard<std::recursive_mutex> lock(kernelLock); // Lock the mutex to ensure thread safety
                        if(socket->header.contains_non_null("method") && std::string(socket->header["method"]) == "GET")
                        {
                            HandleHTTPRequest();
                        }
                        else if(socket->header.contains_non_null("method") && std::string(socket->header["method"]) == "PUT") // JSON Data
                        {
                            HandleHTTPRequest();
                        }
                        auto flush_start = steady_clock::now();
                        socket->Flush();
                        auto finish_start = steady_clock::now();
                        socket->FinishActiveRequest();
                        continue;
                    }
                    socket->Flush();
                    socket->FinishActiveRequest();
                }
            }
            catch(const std::exception& e)
            {
                if(shutdown.load(std::memory_order_acquire))
                    break;
                std::cerr << "HTTP request failed: " << e.what() << '\n';
                if(socket != nullptr)
                    socket->Close();
            }
        }
    }


    void *
    Kernel::StartHTTPThread(Kernel * k)
    {
    k->HandleHTTPThread();
    return nullptr;
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
