// Ikaros 3.0

#include "ikaros.h"
#include "compute_engine.h"
#include "session_logging.h"

using namespace ikaros;
using namespace std::chrono;
using namespace std::literals;

std::atomic<bool> global_terminate(false);

namespace ikaros
{
    namespace
    {
        constexpr size_t default_max_pending_webui_log_messages = 500;

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

        std::vector<std::string> get_parameter_options(const dictionary & info)
        {
            return split(info["options"], ",");
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

        matrix * get_parameter_matrix_ptr(parameter::parameter_value * value)
        {
            return value ? std::get_if<matrix>(value) : nullptr;
        }

        const matrix * get_parameter_matrix_ptr(const parameter::parameter_value * value)
        {
            return value ? std::get_if<matrix>(value) : nullptr;
        }

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
            return (*value.data_)[value.compute_index(zero_index)];
        }

        double parse_parameter_number(const std::string & value, const std::string & conversion_name)
        {
            try
            {
                std::size_t consumed = 0;
                double parsed = std::stod(value, &consumed);
                if(consumed != value.size())
                    throw exception("Could not convert string \"" + value + "\" to " + conversion_name + ".");
                return parsed;
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
            interval_param["description"] = "WebUI update request interval in seconds.";
            return interval_param;
        }

        dictionary make_webui_log_buffer_limit_parameter()
        {
            dictionary limit_param;
            limit_param["_tag"] = "parameter";
            limit_param["name"] = "webui_log_buffer_limit";
            limit_param["type"] = "number";
            limit_param["default"] = static_cast<int>(default_max_pending_webui_log_messages);
            limit_param["description"] = "Maximum number of pending log messages kept for the next WebUI update before older messages are dropped.";
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

    CircularBuffer::CircularBuffer(matrix &  m,  int size):
        buffer_(std::vector<matrix>(size)),
        index_(0)
    {
        for(int i=0; i<size;i++)
        {
            buffer_[i].realloc(m.shape()); // copy(m);
            buffer_[i].reset(); 
        }
    }

    void 
    CircularBuffer::rotate(matrix &  m)
    {
        buffer_[index_].copy(m);
        index_ = ++index_ % buffer_.size();
    }

    matrix & 
    CircularBuffer::get(int i) // Get output with delay i
    {
        return buffer_[(buffer_.size()+index_-i) % buffer_.size()];
    }



// Parameter

    parameter::parameter(dictionary info):
        info_(info), 
        has_options(info_.contains("options")),
        resolved(std::make_shared<bool>(false)),
        type(no_type),
        value(std::make_shared<parameter_value>(std::monostate{}))
    {
        std::string type_string = info_["type"];

        if(type_string=="float" || type_string=="int" || type_string=="double")  // Temporary
            type_string = "number";

        auto type_index = std::find(parameter_strings.begin(), parameter_strings.end(), type_string);
        if(type_index == parameter_strings.end())
            throw exception("Unknown parameter type: "+type_string+".");

        type = parameter_type(std::distance(parameter_strings.begin(), type_index));

        if(has_options)
        {
            value = std::make_shared<parameter_value>(0);
            return;
        }

        switch(type)
        {
            case number_type:
            case rate_type:
                value = std::make_shared<parameter_value>(0.0);
                break;

            case bool_type:
                value = std::make_shared<parameter_value>(false);
                break;

            case string_type: 
                value = std::make_shared<parameter_value>(std::string(""));
                break;

            case matrix_type: 
                value = std::make_shared<parameter_value>(matrix());
                break;

            default: 
                break;
        } 
    }



    parameter::parameter(const std::string type, const std::string options):
        parameter(options.empty() ? dictionary({{"type", type}}) : dictionary({{"type", type},{"options", options}}))
    {}


    void 
    parameter::assign(const parameter & p) // this aliases data from p
    {
        info_ = p.info_;
        type = p.type;
        resolved = p.resolved;
        has_options = p.has_options;
        value = p.value;
    }



    double 
    parameter::operator=(double v)
    {
        if(has_options)
        {
            auto options = get_parameter_options(info_);
            *value = clamp_option_index(int(std::round(v)), options);
            *resolved = true;
            return v;
        }

        switch(type)
        {
            case number_type:
            case rate_type:
                *value = double(v);
                break;
            case bool_type:
                *value = (v != 0.0);
                break;
            case string_type:
                *value = std::to_string(v);
                break;
            case matrix_type:
                *value = scalar_parameter_matrix(v);
                break;
            default:
                throw exception("Invalid parameter type for numeric assignment.");
        }
        *resolved = true;
        return v;
    }

    std::string 
    parameter::operator=(std::string v)
    {
        double val = 0;
        bool has_numeric_value = false;
        if(has_options)
        {
            auto options = get_parameter_options(info_);
            auto it = std::find(options.begin(), options.end(), v);
            if(it != options.end())
                *value = int(std::distance(options.begin(), it));
            else if(is_number(v))
                *value = clamp_option_index(int(parse_parameter_number(v, "option index")), options);
            else
                throw exception("Invalid parameter value");

            *resolved = true;
            return v;
        }
        else if(is_number(v))
        {
            val = parse_parameter_number(v, "number");
            has_numeric_value = true;
        }

        switch(type)
        {
            case number_type:
            case rate_type:
                if(!has_numeric_value)
                    throw exception("Invalid numeric parameter value \"" + v + "\".");
                *value = val;
                break;

            case bool_type:
                *value = is_true(v);
                break;

            case string_type:
                *value = v;
                break;

            case matrix_type:
                if(auto matrix_value = get_parameter_matrix_ptr(value.get()))
                    *matrix_value = v;
                else
                    *value = matrix(v);
                break;

            default:
                throw exception("Invalid parameter type for string assignment.");
        }
        *resolved = true;
        return v;
    }


    void
    parameter::set_matrix(const matrix & v)
    {
        if(type != matrix_type)
            throw exception("Invalid parameter value");

        *value = v;
        *resolved = true;
    }


    parameter::operator matrix & ()
    {
        if(auto matrix_value = get_parameter_matrix_ptr(value.get()))
            return *matrix_value;
        throw exception("Not a matrix value.");
    }


    parameter::operator std::string() const
    {
        if(has_options)
        {
            auto option_index = std::get_if<int>(value.get());
            if(!option_index)
                throw exception("Option parameter missing index value.");
            int index = *option_index;
            auto options = get_parameter_options(info_);
            if(index < 0 || static_cast<std::size_t>(index) >= options.size())
                return std::to_string(index)+" (OUT-OF-RANGE)";
            else
                return options[index];
        } 

        switch(type)
        {
            case no_type: throw exception("Uninitialized or unbound parameter.");
            case number_type:
            case rate_type:
                if(auto number_value = std::get_if<double>(value.get()))
                    return std::to_string(*number_value);
                break;
            case bool_type:
                if(auto bool_value = std::get_if<bool>(value.get()))
                    return (*bool_value ? "true" : "false");
                break;
            case string_type:
                if(auto string_value = std::get_if<std::string>(value.get()))
                    return *string_value;
                break;
            case matrix_type:
                if(auto matrix_value = get_parameter_matrix_ptr(value.get()))
                    return matrix_value->json();
                break;
            default:
                break;
        }
        throw exception("Type conversion error for parameter.");
    }


    parameter::operator double() const
    {
        if(has_options)
        {
            if(auto option_index = std::get_if<int>(value.get()))
                return *option_index;
            throw exception("Option parameter missing index value.");
        }

        if(type==rate_type)
        {
            if(auto number_value = std::get_if<double>(value.get()))
                return *number_value * kernel().GetTickDuration();
        }
        if(auto number_value = std::get_if<double>(value.get()))
            return *number_value;
        else if(auto bool_value = std::get_if<bool>(value.get()))
            return *bool_value ? 1.0 : 0.0;
        else if(auto string_value = std::get_if<std::string>(value.get()))
            return parse_parameter_number(*string_value, "double");
        else if(get_parameter_matrix_ptr(value.get()))
            return get_scalar_matrix_value(*get_parameter_matrix_ptr(value.get()), "double");
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
        if(has_options)
            return as_int() != 0;
        if(type == bool_type)
        {
            if(auto bool_value = std::get_if<bool>(value.get()))
                return *bool_value;
        }
        if(type == string_type)
        {
            if(auto string_value = std::get_if<std::string>(value.get()))
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
        if(has_options)
        {
            if(auto option_index = std::get_if<int>(value.get()))
                return static_cast<long>(*option_index);
            throw exception("Option parameter missing index value.");
        }

        switch(type)
        {
            case no_type: throw exception("Uninitialized_parameter.");
            case number_type:
            case rate_type:
                if(auto number_value = std::get_if<double>(value.get()))
                    return type == rate_type ? static_cast<long>(*number_value * kernel().GetTickDuration()) : static_cast<long>(*number_value);
                break;
            case bool_type:
                if(auto bool_value = std::get_if<bool>(value.get()))
                    return *bool_value ? 1L : 0L;
                break;
            case string_type:
                if(auto string_value = std::get_if<std::string>(value.get()))
                    return static_cast<long>(parse_parameter_number(*string_value, "long"));
                break;
            case matrix_type:
                if(auto matrix_value = get_parameter_matrix_ptr(value.get()))
                    return static_cast<long>(get_scalar_matrix_value(*matrix_value, "long"));
                throw exception("Could not convert matrix to long");
            default: ;
        }
        throw exception("Type conversion error for parameter");
    }


    int
    parameter::as_int() const
    {
        if(has_options)
        {
            if(auto option_index = std::get_if<int>(value.get()))
                return *option_index;
            throw exception("Option parameter missing index value.");
        }

        switch(type)
        {
            case no_type: throw exception("Uninitialized_parameter.");
            case number_type:
            case rate_type:
                if(auto number_value = std::get_if<double>(value.get()))
                    return type == rate_type ? int(*number_value * kernel().GetTickDuration()) : int(*number_value);
                break;
            case bool_type:
                if(auto bool_value = std::get_if<bool>(value.get()))
                    return *bool_value ? 1 : 0;
                break;
            case string_type:
                if(auto string_value = std::get_if<std::string>(value.get()))
                    return int(parse_parameter_number(*string_value, "int"));
                break;
            case matrix_type:
                if(auto matrix_value = get_parameter_matrix_ptr(value.get()))
                    return int(get_scalar_matrix_value(*matrix_value, "int"));
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


    const char* 
    parameter::c_str() const noexcept
    {
        if(auto string_value = std::get_if<std::string>(value.get()))
            return string_value->c_str();
        else
            return nullptr;
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
        if(!name.empty())
            std::cout << name << " = ";
        if(type == no_type)
            std::cout <<"not initialized\n";
        else
            std::cout << std::string(*this) << '\n';
    }


    void 
    parameter::info() const
    {
        std::cout << "name: " << info_["name"] << '\n';
        std::cout << "type: " << type << '\n';
        std::cout << "default: " << info_["default"] << '\n';
        std::cout << "has_options: " << has_options << '\n';
        std::cout << "options: " << info_["options"] << '\n';
        std::cout << "value: " << std::string(*this) << '\n';
    }

    std::string 
    parameter::json() const
    {
        if(has_options)
        {
            if(type == number_type || type == rate_type)
                return "[["+format_json_number(as_double())+"]]";
            if(type == bool_type)
                return (as_bool() ? "[[true]]" : "[[false]]");
            if(type == string_type)
                return "\""+escape_json_string(as_string())+"\"";
            throw exception("Cannot convert parameter to string");
        }

        if((type == number_type || type == rate_type) && std::holds_alternative<double>(*value))
            return "[["+format_json_number(std::get<double>(*value))+"]]";
        if(type == bool_type && std::holds_alternative<bool>(*value))
            return (std::get<bool>(*value) ? "[[true]]" : "[[false]]");
        if(type == string_type && std::holds_alternative<std::string>(*value))
            return "\""+escape_json_string(std::get<std::string>(*value))+"\"";
        if(type == matrix_type)
            if(auto matrix_value = get_parameter_matrix_ptr(value.get()))
                return matrix_value->json();
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
        if(*(p.resolved))
            return true; // Already set from SetParameters

        auto resolve_value = [&](const std::string & raw_value, Component * owner)
        {
            Component * context = owner ? owner : this;

            if(p.type==number_type && !p.has_options)
            {
                SetParameter(name, formatNumber(context->ComputeDouble(raw_value)));
                return;
            }

            if(p.type==matrix_type)
            {
                matrix literal;
                if(try_parse_matrix_literal(literal, raw_value))
                    SetParameter(name, literal, raw_value);
                else
                    SetParameter(name, context->ComputeValue(raw_value));
                return;
            }

            if(p.type==bool_type && !p.has_options)
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
                if(!p.info_.contains("default"))
                {
                    Error("Parameter \""+name+"\" has no default value in the ikc file.");   
                    return false;
                }

                resolve_value(std::string(p.info_["default"]), this);
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
            p.assign((parameter &)kernel().parameters[pname]);
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
                m = (matrix &)(k.parameters[name]);
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
            p.assign(k.parameters[path_+"."+name]);
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


    std::vector<int> 
    Component::EvaluateSizeList(std::string & s) // return list of size from size list in string
    {
        std::vector<int> shape;
        auto resolve_matrix_size_function = [&](const std::string & token) -> std::optional<std::string>
        {
            static const std::vector<std::string> size_functions = {"size_x", "size_y", "size_z", "rows", "cols", "size"};
            std::string function_name;
            std::string matrix_path;

            for(const auto & fn : size_functions)
                if(ends_with(token, "." + fn))
                {
                    function_name = fn;
                    matrix_path = token.substr(0, token.size() - fn.size() - 1);
                    break;
                }

            if(function_name.empty() || matrix_path.empty())
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
            if(function_name == "size")
            {
                std::string result;
                for(size_t i = 0; i < m.shape().size(); ++i)
                {
                    if(i > 0)
                        result += ",";
                    result += std::to_string(m.shape()[i]);
                }
                return result;
            }

            return std::nullopt;
        };
        auto replace_all = [](std::string & target, const std::string & needle, const std::string & replacement)
        {
            if(needle.empty())
                return;

            size_t pos = 0;
            while((pos = target.find(needle, pos)) != std::string::npos)
            {
                target.replace(pos, needle.size(), replacement);
                pos += replacement.size();
            }
        };

        for(std::string e : ComputeEngine::SplitTopLevel(s, ','))
        {
            e = trim(e);
            if(e.empty())
                continue;

            std::string rewritten = e;
            expression expr(e);
            for(const auto & var : expr.variables())
            {
                if(auto replacement = resolve_matrix_size_function(var))
                    replace_all(rewritten, var, *replacement);
                else if(!var.empty() && var[0] == '@')
                {
                    std::string replacement = ComputeValue(var);
                    if(replacement == "true")
                        replacement = "1";
                    else if(replacement == "false")
                        replacement = "0";
                    replace_all(rewritten, var, replacement);
                }
            }

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

            for(std::string item : ComputeEngine::SplitTopLevel(computed, ','))
            {
                item = trim(item);
                if(item.empty())
                    continue;

                int d = ComputeInt(item);
                if(d>0)
                    shape.push_back(d);
                // else
                //     throw std::invalid_argument("Value of "+e+" is non-positive or not found."); // Does not work since function can be called multiple times duing SetSizes
            }
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



    Component::Component():
        parent_(nullptr),
        info_(kernel().current_component_info),
        path_(kernel().current_component_path),
        module_start(0),
        start_tick(0),
        startup_first_real_input_step(std::numeric_limits<int>::max()),
        startup_all_real_inputs_step(std::numeric_limits<int>::max())
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

    // Set parent

        auto p = path_.rfind('.');
        if(p != std::string::npos)
            parent_ = kernel().components.at(path_.substr(0, p)).get();
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


    Module::Module()
    {

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
    Component::SetInputSize_Flat(dictionary d, input_map ingoing_connections)
    {
        Trace("\t\t\t\t\tComponent::SetInputSize_Flat", path_);

        std::string name = d.at("name");
        std::string full_name = path_ +"."+ name;
        
        if(!ingoing_connections.count(full_name)) // Not connected
            return 1;

        int begin_index = 0;
        int end_index = 0;
        int flattened_input_size = 0;
        for(auto & c : ingoing_connections.at(full_name))
        {
            c->flatten_ = true;

         range output_matrix = kernel().buffers[c->source];
            c->Resolve(output_matrix);  //**NEW  

            int s = c->source_range.size() * std::max(1, c->delay_range_.trim().size());
            end_index = begin_index + s;
            c->target_range = range(begin_index, end_index);
            begin_index += s;
            flattened_input_size += s;
        }
    
        if(flattened_input_size != 0)
        {
            kernel().buffers[full_name].realloc(flattened_input_size); 
          Trace("\t\t\tComponent::SetInputSize_Index Alloc "+std::to_string(flattened_input_size), path_);
        }

        if(d.is_set("use_label"))
        {
            begin_index = 0;
            for(auto & c : ingoing_connections.at(full_name))
            {
                int s = c->source_range.size() * std::max(1, c->delay_range_.trim().size());
                if(c->label_.empty())
                    kernel().buffers[full_name].push_label(0, c->source, s); // WAS: kernel().buffers[d.at(full_name)].push_label(0, c->source, s);
                else
                    kernel().buffers[full_name].push_label(0, c->label_, s); // WAS: kernel().buffers[d.at(full_name)].push_label(0, c->label_, s);
            }
        }
        return 0;
    }


    int 
    Component::SetInputSize_Index(dictionary d, input_map ingoing_connections)
    {
       Trace("\t\t\tComponent::SetInputSize_Index ", path_ + "." + std::string(d["name"]));

        range input_size;
        std::string name = d.at("name");
        std::string full_name = path_ +"."+ name;

        if(!ingoing_connections.count(full_name)) // Not connected
            return 1;

        // Handle single connection without inidices - do not collapse dimensions

        if(ingoing_connections.size() == 1 && ingoing_connections.begin()->second[0]->source_range.empty() && ingoing_connections.begin()->second[0]->target_range.empty())
        {
            range output_matrix = kernel().buffers[ingoing_connections.begin()->second[0]->source];
            if(output_matrix.empty())
                return 0;
    
            kernel().buffers[full_name].realloc(output_matrix.extent());

            Trace("\t\t\tComponent::SetInputSize Simple Alloc" + std::string(input_size), full_name);

            return 1;
        }

        for(auto c : ingoing_connections.at(full_name))
        {
            range output_matrix = kernel().buffers[c->source];
            if(output_matrix.empty())
                return 0;
            input_size.extend(c->Resolve(output_matrix));
        }
        kernel().buffers[full_name].realloc(input_size.extent());
        Trace("\t\t\tComponent::SetInputSize Alloc" + std::string(input_size), full_name);

        // Set label if requested and there is only a single input

        if(d.is_set("use_label"))
        {
            auto ic = ingoing_connections.at(full_name);
            if(ic.size() == 1 && !ic[0]->label_.empty())
                kernel().buffers[name].set_name(ic[0]->label_);
        }

        return 1;
    }


// ****************************** COMPONENT Sizes ******************************


    int 
    Component::SetInputSize(dictionary d, input_map ingoing_connections)
    {
        Trace("\t\t\tComponent::SetInputSize ", path_ + "."+ std::string(d["name"]));

        if(d.is_set("flatten"))
            SetInputSize_Flat(d, ingoing_connections);
        else
            SetInputSize_Index(d, ingoing_connections);
        return 0;
    }



    int
    Component:: SetInputSizes(input_map ingoing_connections)
    {
        Kernel& k = kernel();

        Trace("\t\tComponent::SetInputSizes", path_);

        // Set input sizes (if possible)

        for(auto d : info_["inputs"])
            if(k.buffers[path_+"."+std::string(d["name"])].empty())
                if(InputsReady(d, ingoing_connections))
                    SetInputSize(d, ingoing_connections);
        return 0;
    }


    int 
    Component::SetOutputSize(dictionary d, input_map ingoing_connections)
    {
       Trace("\t\t\tComponent::SetOutputSize " , path_ + "." + std::string(d["name"]));

        if(d.contains("size"))
            throw setup_failed(u8"Output \""+std::string(d["name"])+"\"+in group \""+path_+"\" can not have size attribute.", path_);

        range output_range;
        std::string name = d.at("name");
        std::string full_name = path_ +"."+ name;

        if(!ingoing_connections.count(full_name))
            return 0;

        for(auto c : ingoing_connections.at(full_name))
        {
            range output_matrix = kernel().buffers[c->source];
            
            if(output_matrix.empty())
                return 0;
            
            output_range.extend(c->Resolve(output_matrix));
        }
        kernel().buffers[full_name].realloc(output_range);
      Trace("\t\t\t\t\tComponent:: Alloc" + std::string(output_range), path_);

        return 1;
    }


    int 
    Component::SetOutputSizes(input_map ingoing_connections)
    {
        Trace("\t\tComponent::SetOutputSizes", path_);
        for(auto & d : info_["outputs"])
            SetOutputSize(d, ingoing_connections);

        return 0;
    }


    int
    Component::SetSizes(input_map ingoing_connections)
    {
        
        Trace("\tComponent::SetSizes",path_);
        SetInputSizes(ingoing_connections);
        SetOutputSizes(ingoing_connections);

        return 0;
    }


    void
    Component::CheckRequiredInputs()
    {
        Kernel & k = kernel();
        for(dictionary d : info_["inputs"])
        if(!d.is_set("optional") && k.buffers[path_+"."+d["name"].as_string()].empty())
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
    Module::SetOutputSize(dictionary d, input_map)
    {
        try
        {
            std::string size;
            if(d.contains("size"))
                size = std::string(d.at("size"));
            else if(info_.contains("size"))
                size = std::string(info_.at("size"));
            else
                throw setup_failed("Output \""+std::string(d.at("name")) +"\" must have a value for \"size\".", path_);
            
            if(size.empty())
                throw setup_failed("Output \""+std::string(d.at("name")) +"\" must have a value for \"size\".", path_);
            std::vector<int> shape = EvaluateSizeList(size);
            matrix o;
            Bind(o, d.at("name"));
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
    Module::SetOutputSizes(input_map ingoing_connections)
    {
        if(!InputsReady(info_, ingoing_connections))
            return 0; // Cannot set size yet

        for(auto & d : info_["outputs"])
            SetOutputSize(d, ingoing_connections);

        return 0;
    }


    int 
    Module::SetSizes(input_map ingoing_connections)
    {
        SetInputSizes(ingoing_connections);
        SetOutputSizes(ingoing_connections);
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
            if(parameter_name == "log_level" || parameter_name == "module_start" || parameter_name == "start_tick" || parameter_name == "color" || parameter_name == "rgb_quality" || parameter_name == "gray_quality" || parameter_name == "snapshot_interval" || parameter_name == "webui_req_int" || parameter_name == "webui_log_buffer_limit")
                continue;

            parameter p;
            Bind(p, parameter_name);
            //std::cout << "Parameter: " << d["name"] << std::endl;

            if(p.type == string_type)
                check_sum += prime_number.next() * character_sum(p);
            else
            if(p.type == matrix_type)
            {
                if(auto matrix_value = get_parameter_matrix_ptr(p.value.get()))
                    check_sum += prime_number.next() * matrix_value->size();
            }
            else
                check_sum += prime_number.next() * p.as_long();
        }
        // std::cout << "Check sum: " << check_sum << std::endl;
    }



    // Connection

    Connection::Connection(std::string s, std::string t, range & delay_range, std::string label)
    {
        source = peek_head(s, "[");
        source_range = range(peek_tail(s, "[", true));
        target = peek_head(t, "[");
        target_range = range(peek_tail(t, "[", true));
        delay_range_ = delay_range;
        flatten_ = false;
        label_ = label;
    }


    range 
    Connection::Resolve(const range & source_output)
    {
        if(source_output.empty())
            return range();

        source_range.extend(source_output.rank());
        source_range.fill(source_output);
        range reduced_source = source_range.strip().trim();

        if(target_range.empty())
            target_range = reduced_source;
        else
        {
            int j=0;
            for(int i=0; i<target_range.rank()-1; i++)    // CHECK EMPTY DIMENSION
                if(target_range.empty(i) && j<reduced_source.rank())
                {
                    target_range.a_[i] = reduced_source.a_[j];
                    target_range.b_[i] = reduced_source.b_[j];
                    target_range.inc_[i] = reduced_source.inc_[j];

                    reduced_source.a_[j] = 0;  // mark as used
                    reduced_source.b_[j] = 0;
                    reduced_source.inc_[j] = 0;
                    j++;
                }

            int s = 1;
            for(int i=0; i<reduced_source.rank(); i++)
            {
                int si = reduced_source.size(i);
                s *= (si >0?si:1);
            }

            if(target_range.empty(target_range.rank()-1) && j<reduced_source.rank())
            {
                target_range.a_[target_range.rank()-1] = 0; // Check that dim is empty first
                target_range.b_[target_range.rank()-1] = s;
                target_range.inc_[target_range.rank()-1] = 1;
            }
        }
        range trimmed_delay = delay_range_.trim();
        int delay_size = trimmed_delay.empty() ? 1 : trimmed_delay.size();
        if(delay_size > 1)
            target_range.push_front(0, delay_size);
        if(delay_size*source_range.size() != target_range.size())
            throw exception("Connection could not be resolved: "+source+"."+std::string(source_range)+"=>"+target+"."+std::string(target_range));

        return target_range;
    }



    void 
    Connection::Tick()
    {
        auto & k = kernel();

        if(delay_range_.is_delay_0())
        {
            k.buffers[target].copy(k.buffers[source], target_range, source_range);
            //std::cout << source << " =0=> " << target << std::endl; 
        }
        else if(delay_range_.empty() || delay_range_.is_delay_1())
        {
            //std::cout << source << " =1=> " << target << std::endl; 
            k.buffers[target].copy(k.buffers[source], target_range, source_range);
        }

        else if(flatten_) // Copy flattened delayed values
        {
            //std::cout << source << " =F=> " << target << std::endl; 
            matrix ctarget = k.buffers[target];
            int target_offset = target_range.a_[0];
            for(auto delay = delay_range_; delay.more(); delay++)
            {   
                matrix s = k.circular_buffers[source].get(delay.index()[0]);

                for(auto ix=source_range; ix.more(); ix++)
                {
                    int source_index = s.compute_index(ix.index());
                    ctarget[target_offset++] = (*(s.data_))[source_index];
                }
            }
        }

        else if(delay_range_.size() == 1) // Copy indexed delayed value with single delay
        {
            //std::cout << source << " =D=> " << target << std::endl;
            matrix s = k.circular_buffers[source].get(delay_range_.a_[0]);
            k.buffers[target].copy(s, target_range, source_range);
        }

        else // Copy indexed delayed values with more than one element
        {
            //std::cout << source << " =DD=> " << target << std::endl;
            int target_ix = 0;
            for(auto delay = delay_range_; delay.more(); delay++, target_ix++)
            {   
                matrix s = k.circular_buffers[source].get(delay.index()[0]);
                range tr = target_range.tail();
                matrix t = k.buffers[target][target_ix];
                t.copy(s, tr, source_range);

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

    Request::Request(std::string  uri, long sid, std::string b, std::string content_type):
        body(b)
    {
        url = uri;
        session_id = sid;
        uri.erase(0, 1);
        std::string params = tail(uri, "?");
        //std::cout << params << std::endl;
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
        parameters.merge(dictionary(json_body), overwrite);
    }

bool operator==(Request & r, const std::string s)
{
    return r.command == s;
}

// Kernel

    void
    Kernel::Clear()
    {
        // FIXME: retain persistent components

        components.clear();

        connections.clear();
        buffers.clear();   
        max_delays.clear();
        circular_buffers.clear();
        parameters.clear();
        tasks.clear();

        clear_matrix_states();  // if(NOT PERSISTENT)

        tick = -1;
        //run_mode = run_mode_pause;
        //tick_is_running = false;
        tick_time_usage = 0;
        tick_duration = 1; // default value
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
        d["filename"] = ""; // Ignore filename at command line 
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


    void
    Kernel::Tick()
    {
        tick++;

        RunTasks();
        //RunTasksInSingleThread();

        save_matrix_states();
        RotateBuffers();
        Propagate();

        CalculateCPUUsage();
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
                    classes[name].info_.load_xml(p.path());

                    ensure_list(classes[name].info_, "parameters");

                    bool has_log_level = false;
                    bool has_module_start = false;
                    bool has_start_tick = false;
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
                        else if(parameter_name == "color")
                            has_color = true;
                    }

                    if(!has_log_level)
                        classes[name].info_["parameters"].push_back(make_log_level_parameter().copy());

                    if(!has_module_start)
                        classes[name].info_["parameters"].push_back(make_module_start_parameter().copy());

                    if(!has_start_tick)
                        classes[name].info_["parameters"].push_back(make_start_tick_parameter().copy());

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
    Kernel::ScanFiles(std::string path, bool system)
    {
        if(!std::filesystem::exists(path))
        {
            std::cout << "Could not scan for files in \"" + path + "\". Directory not found.\n";
            return;
        }
        for(auto& p: std::filesystem::recursive_directory_iterator(path))
            if(std::string(p.path().extension())==".ikg")
            {
                std::string name = p.path().stem();

                if(system)
                     system_files[name] = p.path();
                else
                     user_files[name] = p.path(); 
            }
    }


    void 
    Kernel::ListClasses()
    {
        std::cout << "\nClasses:\n";
        for(auto & c : classes)
            c.second.Print();
    }


    void 
    Kernel::ResolveParameter(parameter & p,  std::string & name)
    {
        if(*(p.resolved))
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

        for(auto & m : components)
            m.second->SetParameters();

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
        }

        if(!ok)
        {
            for(auto & p : parameters)
                if(!*(p.second.resolved))
                    throw setup_failed("Parameter \""+p.first+"\" could not be resolved.", p.first);
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
                        if(!d.is_set("optional") && ingoing_connections.count(full_name) && buffers[full_name].empty())
                            pending++;
                    }

                    bool is_module = dynamic_cast<Module *>(component.get()) != nullptr;
                    for(dictionary d : component->info_["outputs"])
                    {
                        std::string full_name = name + "." + d["name"].as_string();
                        if((is_module || ingoing_connections.count(full_name)) && buffers[full_name].empty())
                            pending++;
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
                throw setup_failed("Could not resolve all input and output sizes. " + std::to_string(pending) + " buffers remain unresolved.");
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

        catch(...)
        {
            throw setup_failed("Could not calculate input and output sizes.");
        }
    }


    void 
    Kernel::CalculateDelays()
    {
        for(auto & c : connections)
        {
            if(!max_delays.count(c.source))
                max_delays[c.source] = 0;
            if(c.delay_range_.extent()[0] > max_delays[c.source])
            {
                max_delays[c.source] = c.delay_range_.extent()[0];
            }
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
            range delays = connection.delay_range_;
            if(delays.empty())
                return 1;

            int min_delay = unresolved_startup_step;
            for(auto delay = delays; delay.more(); delay++)
                min_delay = std::min(min_delay, delay.index()[0]);

            return min_delay == unresolved_startup_step ? 1 : min_delay;
        }


        int
        ConnectionDelayMax(const Connection & connection)
        {
            range delays = connection.delay_range_;
            if(delays.empty())
                return 1;

            int max_delay = 0;
            for(auto delay = delays; delay.more(); delay++)
                max_delay = std::max(max_delay, delay.index()[0]);

            return max_delay;
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
        for(auto it : max_delays)
        {
            if(it.second <= 1)
                continue;
          if(buffers.count(it.first))
                circular_buffers.emplace(it.first, CircularBuffer(buffers[it.first], it.second));
        }
    }


    void 
    Kernel::RotateBuffers()
    {
        for(auto & it : circular_buffers)
            it.second.rotate(buffers[it.first]);
    }



    void 
    Kernel::ListComponents()
    {
        std::cout << "\nComponents:\n";
        for(auto & m : components)
            m.second->print();
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
        for(auto & i : buffers)
            std::cout << "\t" << i.first <<  i.second.shape() << '\n';
    }


   void Kernel::ListOutputs()
    {
        std::cout << "\nOutputs:\n";
        for(auto & o : buffers)
            std::cout  << "\t" << o.first << o.second.shape() << '\n';
    }


    void 
    Kernel::ListBuffers()
    {
        std::cout << "\nBuffers:\n";
        for(auto & i : buffers)
            std::cout << "\t" << i.first <<  i.second.shape() << '\n';
    }


    void 
    Kernel::ListCircularBuffers()
    {
        if(circular_buffers.empty())
            return;

        std::cout << "\nCircularBuffers:\n";
        for(auto & i : circular_buffers)
            std::cout << "\t" << i.first <<  " " << i.second.buffer_.size() << " " << i.second.buffer_[0].rank() << i.second.buffer_[0].shape() <<  '\n';
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
        for(auto & p : parameters)
            std::cout  << "\t" << p.first << ": " << p.second << '\n';
    }


    void 
    Kernel::PrintLog()
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        for(auto & s : log)
            std::cout << "ikaros: " << s.level_ << ": " << s.message_ << '\n';
        log.clear();
        dropped_webui_log_messages = 0;
    }


    Kernel::Kernel():
        session_id(new_session_id()),
        needs_reload(true),
        shutdown(false),
        run_mode(run_mode_pause),
        idle_time(0),
        tick_duration(1),
        actual_tick_duration(0), // FIME: Use desired tick duration here
        tick_time_usage(0),
        tick(0),
        stop_after(-1),
        lag(0),
        lag_min(0),
        lag_max(0),
        lag_sum(0)
    {
        cpu_cores = std::thread::hardware_concurrency();
        thread_pool = std::make_unique<ThreadPool>(default_thread_pool_size(cpu_cores));
    }


    void
        Kernel::PrintProfiling()
        {
            for(auto & c : components)
                c.second->profiler_.print(c.second->path_);
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
            parameters[name].info_["value"] = value;
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
            parameters[name].info_["value"] = source_value.empty() ? stored_value.json() : source_value;
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
    Kernel::AddGroup(dictionary info, std::string path)
    {
        if(info["parameters"].is_null())
            info["parameters"] = list();

        if(path.empty())
        {
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

        bool is_python_backed = info.contains_non_null("python") && !std::string(info["python"]).empty();
        if(!is_python_backed)
        {
            std::filesystem::path inferred_python_path = class_directory / (classname + ".py");
            if(std::filesystem::exists(inferred_python_path))
            {
                info["python"] = inferred_python_path.lexically_normal().string();
                is_python_backed = true;
            }
        }

        if(!is_python_backed)
            return false;

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

        std::filesystem::path python_path = std::string(info["python"]);
        if(python_path.is_relative())
            python_path = class_directory / python_path;
        info["python"] = python_path.lexically_normal().string();
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

         std::string delay_range = info.contains_non_null("delay") ? info["delay"] : "";
         std::string label = info.contains_non_null("label") ? info["label"] : "";

        if(delay_range.empty() || delay_range=="null")
            delay_range = "[1]";
        else if(delay_range[0] != '[')
            delay_range = "["+delay_range+"]";
        range r(delay_range);
        connections.push_back(Connection(source, target, r, label));
    }



    void Kernel::LoadExternalGroup(dictionary & d)
    {
        std::string path = d["external"];
        dictionary external;
        external.load_xml(path);
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

            AddGroup(d, name);

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
        for(auto & c : components)
            try
            {
                c.second->Init();
            }
            catch(const fatal_error& e)
            {
                throw init_error(u8"Fatal error. Init failed for \""+c.second->path_+"\": "+std::string(e.what()), c.second->path_);
            }
            catch(const std::exception& e)
            {
                throw init_error(u8"Init failed for "+c.second->path_+": "+std::string(e.what()), c.second->path_);
            }
            catch(...)
            {
                throw init_error(u8"Init failed");
            }
    }


    void 
    Kernel::SetCommandLineParameters(dictionary & d) // Add explicit command line overrides without clobbering file values with defaults
    {
        for(auto & x : options_.d)
            if(options_.is_explicitly_set(x.first))
                d[x.first] = x.second;

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
    Kernel::LoadFile()
    {
        std::lock_guard<std::recursive_mutex> lock(kernelLock);
        try
        {
            if(components.size() > 0)
                Clear();
            if(!std::filesystem::exists(options_.full_path()))
                throw load_failed(u8"File \""+options_.full_path()+"\" does not exist.");

                try
                {
                    dictionary d;
                    d.load_xml(options_.full_path());
                    SetCommandLineParameters(d);
                    d["filename"] = options_.stem();
                    BuildGroup(d);
                    info_ = d;
                    session_id = new_session_id(); 
                    ResetUISnapshotCache();
                    Notify(msg_print, u8"Loaded "s+options_.full_path());
                    SetUp();
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

        for(const auto & [path, component] : components)
        {
            (void)path;
            if(component == nullptr)
                continue;

            std::string class_name = component->info_.contains_non_null("class") ? std::string(component->info_["class"]) : "";
            if(class_name.empty())
                class_name = component->Info();
            if(class_name.empty())
                class_name = "unknown";

            class_counts[class_name]++;
        }

        std::vector<std::string> summary_entries;
        summary_entries.reserve(class_counts.size());
        for(const auto & [class_name, count] : class_counts)
            summary_entries.push_back(class_name + ":" + std::to_string(count));

        d["module_count"] = static_cast<int>(components.size());
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
            if(c.delay_range_.is_delay_0())
                continue;
            else
                try
                {
                    c.Tick();
                }
                catch(const std::exception& e)
                {
                    std::cerr << e.what() << '\n';
                }
                
                
    }



    void
    Kernel::InitSocket(long port)
    {
        try
        {
            shutdown.store(false, std::memory_order_release);
            socket = std::make_unique<ServerSocket>(port);
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

        for(auto & c : connections) // Only zero-delay connections are sorted into tasks
        if(c.delay_range_.is_delay_0())
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



    void Kernel::RunTasks()
    {
        std::vector<std::shared_ptr<TaskSequence>> sequences;
    
        try 
        {
            // Create and submit tasks using shared_ptr
            for (auto &task_sequence : tasks) 
            {
                auto ts = std::make_shared<TaskSequence>(task_sequence);
                thread_pool->submit(ts);
                sequences.push_back(ts);
            }
    
            // Wait for completion
            for (auto &ts : sequences) 
            {
                if(!ts->waitForCompletion(5)) // Timeout after 5 seconds
                    throw std::runtime_error("Task sequence timed out after 5 seconds.");

                ts->rethrowIfError();
            }
        } 
        catch (const std::exception &e) 
        {
            Notify(msg_fatal_error, "Error during task execution: " + std::string(e.what()));
        }
        catch (...) 
        {
            Notify(msg_fatal_error, "Error during task execution: Unknown error.");
        }
    }



    void
    Kernel::RunTasksInSingleThread()
    {
        for(auto & task_group : tasks)
            for(auto & task: task_group)
                if(task->ShouldTick())
                    task->Tick();
    }



    void
    Kernel::SetUp()
    {
        try
        {
            PruneConnections();
            SortTasks();
            CalculateStartupSteps();
            ResolveParameters();
            CalculateDelays();
            CalculateSizes();

            InitCircularBuffers();
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
                if(ends_with(name, ".execution_mode") && std::string(parameter) == "async")
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
                    lag = timer.WaitUntil(double(tick+1)*tick_duration);
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
                if(lag > 1.0)
                    {
                        Notify(msg_warning, "Performance warning: System is " + std::to_string(lag) +  " seconds behind real time. Consider increasing tick_duration.");
                        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Give HTTP thread more time to run
                    }
                    else  if(lag > 0.001)
                        {
                            std::cout  << "Ikaros is lagging " << lag << " seconds behind real time.\n";
                            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Give HTTP thread a chance to run
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
                        Tick();
                        BuildUISnapshot();
                    }
                    catch(std::exception & e)
                    {
                        //std::cout << e.what() << std::endl;
                        Notify(msg_fatal_error, (e.what()));
                        return;
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
                const size_t max_pending_webui_log_messages = MaxPendingWebUILogMessages();
                if(log.size() >= max_pending_webui_log_messages)
                {
                    log.erase(log.begin());
                    ++dropped_webui_log_messages;
                }
                log.push_back(Message(msg, timestamped_message, path));
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
        return info_.xml("group");
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
        run_mode.store(std::min(run_mode_stop, run_mode.load()));
        tick = -1;
        timer.Pause();
        timer.SetPauseTime(0);
#if !defined(LOGGING_OFF)
        if(session_logging_active)
        {
            LogStop();
            session_logging_active = false;
        }
#endif
        //PrintProfiling(); // FIXME: Use option to turn on and off 
        Clear(); // Delete all modules
        needs_reload = true;
    }



    void
    Kernel::Pause()
    {
        if(needs_reload)
        {
            LoadFile();
            run_mode = run_mode_pause;
        }
        else
        {
            run_mode = run_mode_pause;
            timer.Pause();
            timer.SetPauseTime(GetTime()+tick_duration);
        }
    }


    void
    Kernel::Realtime()
    {
        if(needs_reload)
            LoadFile();
    
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
            LoadFile();

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

        double webui_request_interval = 0.1;
        if(parameters.count("webui_req_int"))
        {
            try
            {
                webui_request_interval = std::max(0.001, parameters.at("webui_req_int").as_double());
            }
            catch(const std::exception &)
            {
            }
        }

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
            response << "\t\"progress\": " << double(tick)/double(stop_after) << ",\n";
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
        response << "\t\"webui_req_int\": " << webui_request_interval << ",\n";
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


    double
    Kernel::SnapshotInterval() const
    {
        if(parameters.count("snapshot_interval"))
        {
            try
            {
                return std::max(0.0, parameters.at("snapshot_interval").as_double());
            }
            catch(const std::exception &)
            {
            }
        }
        return 0.1;
    }


    size_t
    Kernel::MaxPendingWebUILogMessages() const
    {
        if(parameters.count("webui_log_buffer_limit"))
        {
            try
            {
                return std::max<size_t>(1, static_cast<size_t>(parameters.at("webui_log_buffer_limit").as_int()));
            }
            catch(const std::exception &)
            {
            }
        }
        return default_max_pending_webui_log_messages;
    }


    int
    Kernel::SnapshotJPEGQualityForFormat(const std::string & format) const
    {
        const char * parameter_name = format == "rgb" ? "rgb_quality" : "gray_quality";
        if(parameters.count(parameter_name))
        {
            try
            {
                int quality = parameters.at(parameter_name).as_int();
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
        if(!requested_value.key.empty() && requested_value.key[0] == '.')
            source_with_root = requested_value.key.substr(1);

        std::string component_path = peek_rhead(source_with_root, ".");
        std::string attribute = peek_rtail(source_with_root, ".");

        bool found_value = false;
        if(buffers.count(source_with_root))
        {
            if(requested_value.format.empty())
                serialized_value = buffers[source_with_root].json();
            else if(is_snapshot_image_format(requested_value.format))
                serialized_value = SendImage(buffers[source_with_root], requested_value.format, SnapshotJPEGQualityForFormat(requested_value.format));
            found_value = !serialized_value.empty();
        }
        else if(parameters.count(source_with_root))
        {
            serialized_value = parameters[source_with_root].json();
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
    Kernel::SerializePendingLog(bool clear_pending_log)
    {
        std::string response = ",\n\"log\": [";
        std::string sep;
        std::lock_guard<std::mutex> lock(log_mutex);
        if(dropped_webui_log_messages > 0)
        {
            const std::string dropped_message =
                "WebUI log truncated. Dropped " + std::to_string(dropped_webui_log_messages) +
                " older log message" + (dropped_webui_log_messages == 1 ? "" : "s") + ".";
            response += Message(msg_warning, dropped_message).json();
            sep = ",";
        }
        for(auto line : log)
        {
            response += sep + line.json();
            sep = ",";
        }
        response += "]";
        if(clear_pending_log)
        {
            log.clear();
            dropped_webui_log_messages = 0;
        }
        return response;
    }


    std::string
    Kernel::ConsumeSnapshotLogForSession(long ui_session_id, const UISnapshot & snapshot)
    {
        std::lock_guard<std::mutex> lock(ui_subscriptions_mutex);
        auto & session_subscription = ui_session_subscriptions[ui_session_id];
        if(session_subscription.delivered_log_snapshot_id == snapshot.snapshot_id)
            return "";
        session_subscription.delivered_log_snapshot_id =
            std::max(session_subscription.delivered_log_snapshot_id, snapshot.snapshot_id);
        return snapshot.log_json;
    }


    void
    Kernel::ResetUISnapshotCache()
    {
        {
            std::lock_guard<std::mutex> lock(ui_snapshot_mutex);
            current_ui_snapshot.reset();
        }
        {
            std::lock_guard<std::mutex> lock(ui_subscriptions_mutex);
            ui_session_subscriptions.clear();
        }
    }


    void
    Kernel::BuildUISnapshot()
    {
        std::unordered_set<std::string> subscriptions;
        double now = GetRealTime();
        {
            std::lock_guard<std::mutex> lock(ui_subscriptions_mutex);
            for(auto it = ui_session_subscriptions.begin(); it != ui_session_subscriptions.end();)
            {
                if(now - it->second.last_seen_time > ui_subscription_timeout_seconds)
                    it = ui_session_subscriptions.erase(it);
                else
                {
                    subscriptions.insert(it->second.keys.begin(), it->second.keys.end());
                    ++it;
                }
            }
        }

        std::shared_ptr<const UISnapshot> previous_snapshot;
        {
            std::lock_guard<std::mutex> lock(ui_snapshot_mutex);
            previous_snapshot = current_ui_snapshot;
        }

        bool refresh_images = previous_snapshot == nullptr || now - previous_snapshot->image_timestamp >= SnapshotInterval();
        auto snapshot = std::make_shared<UISnapshot>();
        snapshot->snapshot_id = next_ui_snapshot_id++;
        snapshot->session_id = session_id;
        snapshot->tick = tick;
        snapshot->image_timestamp = refresh_images ? now : (previous_snapshot ? previous_snapshot->image_timestamp : now);
        snapshot->status_json = DoSendDataStatus();
        snapshot->log_json = SerializePendingLog(true);

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

                image_futures.push_back(std::async(std::launch::async, [this, subscription_key, requested_value]() mutable
                {
                    std::string serialized_value;
                    if(SerializeRequestedValue(requested_value, serialized_value))
                        return std::make_pair(subscription_key, std::move(serialized_value));
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

        std::lock_guard<std::mutex> lock(ui_snapshot_mutex);
        current_ui_snapshot = std::move(snapshot);
    }


    std::string
    Kernel::DoSendLog(Request &)
    {
        return SerializePendingLog(true);
    }


    void
    Kernel::DoSendData(Request & request)
    {
        auto requested_values = ParseRequestedUIValues(request);
        std::unordered_set<std::string> requested_subscriptions;
        requested_subscriptions.reserve(requested_values.size());
        for(const auto & requested_value : requested_values)
            requested_subscriptions.insert(SubscriptionKeyFor(requested_value));
        {
            std::lock_guard<std::mutex> lock(ui_subscriptions_mutex);
            auto & session_subscription = ui_session_subscriptions[request.session_id];
            session_subscription.keys = std::move(requested_subscriptions);
            session_subscription.last_seen_time = GetRealTime();
        }

        std::shared_ptr<const UISnapshot> snapshot;
        {
            std::lock_guard<std::mutex> lock(ui_snapshot_mutex);
            snapshot = current_ui_snapshot;
        }

        long response_session_id = 0;
        std::string status;
        std::string log_json;
        if(snapshot != nullptr)
        {
            response_session_id = snapshot->session_id;
            status = snapshot->status_json;
            log_json = ConsumeSnapshotLogForSession(request.session_id, *snapshot);
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

        if(!fallback_items.empty() || snapshot == nullptr)
        {
            std::lock_guard<std::recursive_mutex> lock(kernelLock);
            response_session_id = session_id;
            if(snapshot == nullptr)
            {
                status = DoSendDataStatus();
                log_json = SerializePendingLog(true);
            }

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
        auto & files = where == "system" ? system_files : user_files;
        auto file_path = files.find(file);
        if(file_path == files.end())
        {
            Notify(msg_warning, "File \""+file+"\" could not be found.");
            DoSendNetwork(request);
            return;
        }

        Stop();
        options_.path_ = file_path->second;
            try
            {
                LoadFile();
            }
            catch(const setup_failed& e)
            {
                //std::cerr << e.what() << '\n';
                Notify(msg_warning, "Could not set up file \""+file+"\": "+e.message(), e.path());
                // New();
            }
            catch(const load_failed& e)
            {
                Notify(msg_warning, "Could not load file \""+file+"\": "+e.message(), e.path());
                New();
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
                Notify(msg_warning, "Could not open file \""+file+"\": "+std::string(e.what()));
                New();
            }
            
        // Great we loaded the file. Check the run_mode in the loaded file and set it accordingly?
        // if(std::string(info_["real_time"]) == "true")
        //     Realtime();

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
            return;
        }

        // Sanitize file name

        std::filesystem::path path = add_extension(std::string(d["filename"]), ".ikg");
        std::string filename = path.filename();

        d.erase("filename");
        std::string data = d.xml("group", {"module/parameters","module/inputs","module/outputs", "module/authors","module/descriptions", "group/views", "module.description"});
        std::ofstream file;
        file.open (filename);
        file << data;
        file.close();
        options_.path_ = filename;
        needs_reload = true;

        d["filename"] = options_.stem();
        info_ = d;

        DoUpdate(request);
    }


    void
    Kernel::DoQuit(Request & request)
    {
        Notify(msg_print, "quit");
        Stop();
        run_mode = run_mode_quit;
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
    Kernel::DoSendNetwork(Request &)
    {
        std::string s = json(); 

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
        try
        {
            Pause();
            run_mode = run_mode_pause;
            Tick();
            timer.SetPauseTime(GetTime()+tick_duration);
        }
        catch(const exception& e)
        {
            Notify(msg_warning, e.what(), e.path());
        }
        DoSendData(request);
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
        std::string body;
        if(!buffers.count(request.component_path))
        {
            body = "Buffer \""+request.component_path+"\" can not be found";
            SendStringResponse(header, body);
            return;
        }
        if(buffers[request.component_path].rank() > 2)
        {
            body = "Rank of matrix != 2. Cannot be displayed as CSV";
            SendStringResponse(header, body);
            return;
        }
        SendStringResponse(header, buffers[request.component_path].csv());
    }



    void
    Kernel::DoJSON(Request & request)
    {
        std::string key = request.component_path;
        if(!key.empty() && key[0] == '.')
            key = key.substr(1); // Global path

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

        if(buffers.count(key))
        {
            send_json_response(buffers[key].json(), buffers[key].shape());
            SendStringResponse(header, body);
            return;
        }

        if(parameters.count(key))
        {
            if(parameters[key].type == matrix_type)
            {
                if(auto matrix_value = get_parameter_matrix_ptr(parameters[key].value.get()))
                    send_json_response(parameters[key].json(), matrix_value->shape());
                else
                    send_json_response(parameters[key].json(), {});
            }
            else
                send_json_response(parameters[key].json(), {});
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
        if(!buffers.count(request.component_path))
        {
            SendStringResponse(header, "Buffer \""+request.component_path+"\" can not be found");
            return;
        }
        if(buffers[request.component_path].rank() > 2)
        {
            SendStringResponse(header, "Rank of matrix != 2. Cannot be displayed as CSV");
            return;
        }
        SendStringResponse(header, buffers[request.component_path].csv());
    }



    void
    Kernel::DoImage(Request & request)
    {
        std::string key = request.component_path;
        if(!key.empty() && key[0] == '.')
            key = key.substr(1); // Global path

        matrix * image = nullptr;

        if(buffers.count(key))
            image = &buffers[key];
        else if(parameters.count(key) && parameters[key].type == matrix_type)
        {
            if(auto matrix_value = get_parameter_matrix_ptr(parameters[key].value.get()))
                image = matrix_value;
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
    Kernel::DoProfiling(Request &)
    {
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

            std::string key = request.component_path;
            if(key[0] == '.')
                key = key.substr(1); // Global path


            if(!components.count(key))
            {
                Notify(msg_warning, "Component '"+request.component_path+"' could not be found.");
                DoSendData(request);
                return;
            }
   
            if(!request.parameters.contains("command"))
            {
                    Notify(msg_warning, "No command specified for  '"+request.component_path+"'.");
                    DoSendData(request);
                    return;
            }

            components.at(key)->Command(request.parameters["command"], request.parameters);
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

            std::string key = request.component_path;
            if(key[0] == '.')
                key = key.substr(1); // Global path

            if(!parameters.count(key))
            {
                Notify(msg_warning, "Parameter '"+request.component_path+"' could not be found.");
                DoSendData(request);
                return;
            }

            parameter & p = parameters.at(key);
            if(p.type == matrix_type)
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

                if(auto matrix_value = get_parameter_matrix_ptr(p.value.get()))
                {
                    if(matrix_value->rank() == 1)
                        (*matrix_value)(x)= value;
                    else if(matrix_value->rank() == 2)
                        (*matrix_value)(y,x)= value; // Is this correct?
                    else
                        throw exception("Higher-dimensional matrix parameters are not supported by /control.");
                }
                else
                    throw exception("Parameter is not a matrix.");
            }
            else
            {
                p = std::string(request.parameters["value"]);
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
            DoSendData(request);
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
        for(auto & c: classes)
        {
            body += s;
            body += escape_json_string(c.first);
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
        for(auto & c: classes)
        {
            body += s;
            body += "\""+escape_json_string(c.first)+"\": ";
            dictionary class_info = c.second.info_.copy();
            class_info["path"] = std::filesystem::path(c.second.path).parent_path().string();
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
        user_files.clear();
        ScanFiles(options_.ikaros_root+"/Source/Modules");
        ScanFiles(options_.ikaros_root+"/UserData", false);

        // Send result

        dictionary header({
            {"Content-Type", "application/json; charset=utf-8"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        std::string sep;
        std::string body = "{\"system_files\":[\n\t\"";
        for(auto & f: system_files)
        {
            body += sep;
            body += escape_json_string(f.first);
            sep = "\",\n\t\"";
        }
        body += "\"\n],\n";
    
        sep = "";
        body += "\"user_files\":[\n\t\"";
        for(auto & f: user_files)
        {
            body += sep;
            body += escape_json_string(f.first);
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
    Kernel::HandleHTTPRequest()
    {
        long sid = 0;
        if(socket->header.contains_non_null("session-id"))
            sid = atol(std::string(socket->header["session-id"]).c_str());

        std::string content_type;
        if(socket->header.contains_non_null("content-type"))
            content_type = std::string(socket->header["content-type"]);

        Request request(std::string(socket->header["uri"]), sid, socket->body, content_type);

        if(request.parameters.contains("proxy"))
            request.component_path = std::string(request.parameters["proxy"]);

        //std::cout << "Request: " << request.url << std::endl;

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
Kernel::CalculateCPUUsage() // In percent
{
    double cpu = 0;
    struct rusage rusage;
    if(getrusage(RUSAGE_SELF, &rusage) != -1)
        cpu = double(rusage.ru_utime.tv_sec) + double(rusage.ru_utime.tv_usec) / 1000000.0;
    if(actual_tick_duration > 0)
        cpu_usage = (cpu-last_cpu)/double(cpu_cores)*actual_tick_duration;
    last_cpu = cpu;
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

                    if(is_update_request && (method == "GET" || method == "PUT"))
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

Kernel::~Kernel()
{
    StopHTTPServer();
}

}; // namespace ikaros
