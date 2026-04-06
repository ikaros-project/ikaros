// Ikaros 3.0

#include "ikaros.h"
#include "session_logging.h"

using namespace ikaros;
using namespace std::chrono;
using namespace std::literals;

std::atomic<bool> global_terminate(false);

namespace ikaros
{
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
        index_(0),
        buffer_(std::vector<matrix>(size))
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
        type(no_type), 
        has_options(info_.contains("options")),
        resolved(std::make_shared<bool>(false))
    {
        std::string type_string = info_["type"];

        if(type_string=="float" || type_string=="int" || type_string=="double")  // Temporary
            type_string = "number";

        auto type_index = std::find(parameter_strings.begin(), parameter_strings.end(), type_string);
        if(type_index == parameter_strings.end())
            throw exception("Unknown parameter type: "+type_string+".");

        type = parameter_type(std::distance(parameter_strings.begin(), type_index));

        // Init shared pointers
        switch(type)
        {
            case number_type:
            case rate_type:
            case bool_type:
                number_value = std::make_shared<double>(0); 
                break;

            case string_type: 
                string_value = std::make_shared<std::string>(""); 
                break;

            case matrix_type: 
                {
                    if(info_.contains("size"))
                    {
                        std::cout << "HAS SIZE" << std::endl;
                        matrix_value = std::make_shared<matrix>();
                    }
                    else
                        matrix_value = std::make_shared<matrix>();
                }
                break;

            default: 
                break;
        } 
    }



    parameter::parameter(const std::string type, const std::string options):
        parameter(options.empty() ? dictionary({{"type", type}}) : dictionary({{"type", type},{"options", options}}))
    {}


    void 
    parameter::operator=(parameter & p) // this shares data with p 
    {
        info_ = p.info_;
        type = p.type;
        resolved = p.resolved;
        has_options = p.has_options;

        number_value = p.number_value;
        matrix_value = p.matrix_value;
        string_value = p.string_value;
    }



    double 
    parameter::operator=(double v) // FIXME: handle matrix type as well
    {
        if(has_options)
        {
            auto options = split(info_["options"],",");
            int c = options.size();
            if(v > c-1)
                v = c-1;
            switch(type)
            {
                case number_type:
                case rate_type:
                case bool_type:
                    *number_value = double(v);
                    break;
                case string_type:
                    *string_value = options[round(v)];
                    break;
                default:
                    break; // FIXME: error?
            }
            return v;
        }

        switch(type)
        {
            case number_type:
            case rate_type:
            case bool_type:
                *number_value = double(v);
                break;
            case string_type:
                    *string_value = std::to_string(v);
                break;
            default:
                break; // FIXME: error?
        }
        *resolved = true;
        return v;
    }

    std::string 
    parameter::operator=(std::string v) // FIXME: Check this function for all type combinations
    {
        double val = 0;
        if(has_options)
        {
            auto options = split(info_["options"],",");
            auto it = std::find(options.begin(), options.end(), v);
            if(it != options.end())
                val = std::distance(options.begin(), it);
            else if(is_number(v))
                {
                    val = stod(v);
                    v = options.at(int(val));
                }
            else
                throw exception("Invalid parameter value");
        }
        else if(is_number(v))
            val = stod(v);

        switch(type)
        {
            case number_type:
            case rate_type:
                *number_value = val;
                break;

            case bool_type:
                if(has_options)
                    *number_value = (val!=0 ? 1 : 0);
                else
                    *number_value = is_true(v);
                break;

            case string_type:
                *string_value = v;
                break;

            case matrix_type:
                *matrix_value = v;
                break;

            default:
                break; // FIXME: error? Handle matrix with single element
        }
        *resolved = true;
        return v;
    }


    parameter::operator matrix & ()
    {
        if(matrix_value) 
            return *matrix_value;
        else
            throw exception("Not a matrix value.");
    }


    parameter::operator std::string() const
    {
        if(string_value)
            return  *string_value;

        if(has_options)
        {
            int index = int(*number_value);
            auto options = split(info_["options"],","); // FIXME: Check trim in split
            if(index < 0 || index>= options.size())
                return std::to_string(index)+" (OUT-OF-RANGE)";
            else
                return options[index];
        } 

        switch(type)
        {
            case no_type: throw exception("Uninitialized or unbound parameter.");
            case number_type: if(number_value) return std::to_string(*number_value);
            case bool_type: if(number_value) return (*number_value>0 ? "true" : "false");
            case rate_type: if(number_value) return std::to_string(*number_value);
            case string_type: if(string_value) return *string_value;
            case matrix_type: return matrix_value->json();
            default:  throw exception("Type conversion error for parameter.");
        }
    }


    parameter::operator double() const
    {
        if(type==rate_type)
            return *number_value * kernel().GetTickDuration();
        if(number_value) 
            return *number_value;
        else if(string_value) 
        {
            if(has_options)
            {
                auto options = split(info_["options"],","); // FIXME: Check trim in split
            auto it = std::find(options.begin(), options.end(), *string_value);
            if(it != options.end())
                return std::distance(options.begin(), it);
            else
                return 0;
            }
            else
                return stod(*string_value); // FIXME: may fail ************
        }
        else if(matrix_value)
            return 0;// FIXME check 1x1 matrix ************
        else
            throw exception("Type conversion error. Parameter does not have a type Check spelling IKC and cc file.");
    }


    bool
    parameter::as_bool() const
    {
        if(type == string_type && string_value)
            return is_true(*string_value);
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


    int
    parameter::as_int() const
    {
        switch(type)
        {
            case no_type: throw exception("Uninitialized_parameter.");
            case number_type: if(number_value) return *number_value;
            case rate_type: if(number_value) return *number_value;    // FIXME: Take care of time base
            case bool_type: if(number_value) return *number_value;
            case string_type: if(string_value) return stoi(*string_value); // FIXME: Check that it is a number
            case matrix_type: throw exception("Could not convert matrix to int"); // FIXME check 1x1 matrix
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
        if(string_value)
            return string_value->c_str();
        else
            return NULL;
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
            std::cout <<"not initialized" << std::endl;
        else
            std::cout << std::string(*this) << std::endl;
    }


    void 
    parameter::info() const
    {
        std::cout << "name: " << info_["name"] << std::endl;
        std::cout << "type: " << type << std::endl;
        std::cout << "default: " << info_["default"] << std::endl;
        std::cout << "has_options: " << has_options << std::endl;
        std::cout << "options: " << info_["options"] << std::endl;
        std::cout << "value: " << std::string(*this) << std::endl;
    }

    std::string 
    parameter::json() const
    {
        switch(type)     // FIXME: remove if statements and use exception handling
        {
            case number_type:    if(number_value)   return "[["+std::to_string(*number_value)+"]]";
            case bool_type:     if(number_value)    return (*number_value!=0 ? "[[true]]" : "[[false]]");
            case rate_type:     if(number_value)    return "[["+std::to_string(*number_value)+"]]";
            case string_type:   if(string_value)    return "\""+*string_value+"\"";
            case matrix_type:   if(matrix_value)    return matrix_value->json();
            default:            throw exception("Cannot convert parameter to string");
        }
    }


    std::ostream& operator<<(std::ostream& os, parameter p) // FIXME: Handle options
    {
        switch(p.type)
        {
            case number_type:    if(p.number_value) os <<  *p.number_value; break; // FIXME: Is string conversion sufficient?
            case bool_type:     if(p.number_value) os <<  (*p.number_value == 0 ? "false" : "true"); break;
            case rate_type:    if(p.number_value) os <<  *p.number_value; break; 
            case string_type:   if(p.string_value) os <<  *p.string_value; break;
            case matrix_type:   if(p.matrix_value) os <<  *p.matrix_value; break;
            default:            throw exception("Cannot convert parameter to string for printing");
        }

        return os;
    }


// Component

    void 
    Component::print() const
    {
        std::cout << "Component: " << info_["name"]  << '\n';
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
        try
        {
            // Look for binding
            std::string bind_to = GetBind(name);
            if(!bind_to.empty())
            {
                if(LookupParameter(p, bind_to)) // FIXME: Not working
                    return true;
            }

            const Component * value_owner = GetValueOwner(name);
            std::string value = value_owner ? std::string(value_owner->info_[name]) : "";
            if(value.empty())
            {
                if(!p.info_.contains("default"))
                {
                    Error("Parameter \""+name+"\" has no default value in the ikc file.");   
                    return false;
                }

                SetParameter(name, p.info_["default"]); // FIXME: SHOULD EVALUATE DEFAULT VALUE - MAYBE
                return true;
            }

            // Evaluate if numerical expression

            if(p.type==number_type && !p.has_options)
            {
                SetParameter(name, formatNumber(const_cast<Component *>(value_owner ? value_owner : this)->ComputeDouble(value)));
                return true;
            }

            if(p.type==matrix_type)
            {
                SetParameter(name, const_cast<Component *>(value_owner ? value_owner : this)->Compute(value));
                return true;
            }

            if(p.type==bool_type && !p.has_options)
            {
                SetParameter(name, const_cast<Component *>(value_owner ? value_owner : this)->ComputeBool(value) ? "true" : "false");
                return true;
            }

            // Lookup normal value in current component-context

            if(value.empty())  // ****************** this does not work for string that are allowed to be empty // FIXME: USE EXCEPTIONS *****
            {
                SetParameter(name, p.info_["default"]);
                return true;
            }
                
            if(value.find('@') != std::string::npos || value.find('{') != std::string::npos)
                SetParameter(name, const_cast<Component *>(value_owner ? value_owner : this)->Compute(value));
            else
                SetParameter(name, value);
            return true;
        }
        catch(exception & e)
        {
            Notify(msg_warning, e.message());
            // FIXME:   THROW ???? 
        }
        catch(std::exception & e)
        {
            Notify(msg_warning, "ERROR: Could not resolve parameter \""s +name + "\": " + e.what(), name);  
            // FIXME:   THROW ???? 
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
        else
            return ""; // throw exception("Name not found"); // throw not_found_exception instead
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



    static std::string
    exchange_before_dot(const std::string& original, const std::string& replacement)
    {
        size_t pos = original.find('.');
        if(pos == std::string::npos) // No dot found, replace the whole string
            return replacement;
     else  // Replace up to the first dot
            return replacement + original.substr(pos);
    }



    static std::string
    before_dot(const std::string& original)
    {
        size_t pos = original.find('.');
        if(pos == std::string::npos)
            return original;
     else 
            return original.substr(0,pos);
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
                return kernel().components.at(path.substr(1));
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
            p = (parameter &)kernel().parameters[pname];
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
            else if(k.buffers.count(name))
                m = k.buffers[name];
            else if(k.parameters.count(name))
                m = (matrix &)(k.parameters[name]);
            else if(k.parameters.count(name))
                throw exception("Cannot bind to attribute \""+name+"\". Define it as a parameter!", path_);
            else
                throw exception("Input, output or parameter named \""+name+"\" does not exist", path_);
        }
        catch(exception e)
        {
            throw exception("Bind:\""+name+"\" failed. "+e.message(), path_);
        }
    }


    parameter &  
    Component::GetParameter(std::string name)
    {
               Kernel & k = kernel();
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


    bool Component::LookupParameter(parameter & p, const std::string & name)
    {
        Kernel & k = kernel();
        if(k.parameters.count(path_+"."+name))
        {
            p = k.parameters[path_+"."+name];
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
    Component::Compute(const std::string & s)
    {
        // Compute uses an explicit-evaluation style:
        // plain final segments are literals, while @ and {...} request lookup/substitution.
        // Math is evaluated recursively, but variable references inside math must use @.
        return ComputeMatrix(s, 0);
    }


    double
    Component::ComputeDouble(const std::string & s)
    {
        std::string value = trim(Compute(s));
        if(!is_number(value))
            throw exception("ComputeDouble could not convert \""+value+"\" to number.", path_);
        return std::stod(value);
    }


    int
    Component::ComputeInt(const std::string & s)
    {
        double value = ComputeDouble(s);
        double rounded = std::round(value);
        if(std::fabs(value-rounded) > 1e-9)
            throw exception("ComputeInt could not convert non-integer value \""+formatNumber(value)+"\".", path_);
        return static_cast<int>(rounded);
    }


    bool
    Component::ComputeBool(const std::string & s)
    {
        std::string value = trim(Compute(s));
        static std::vector<std::string> false_list = {"false", "False", "no", "NO", "off", "OFF", "0"};

        if(is_true(value))
            return true;
        if(std::find(false_list.begin(), false_list.end(), value) != false_list.end())
            return false;

        throw exception("ComputeBool could not convert \""+value+"\" to bool.", path_);
    }


    std::string
    Component::ComputeMatrix(const std::string & s, int depth)
    {
        ComputeCheckDepth(depth);

        auto rows = ComputeSplitTopLevel(s, ';');
        if(rows.size() <= 1)
            return ComputeList(trim(s), depth+1);

        std::vector<std::string> computed_rows;
        for(const auto & row : rows)
        {
            if(row.empty())
                continue;
            computed_rows.push_back(ComputeList(row, depth+1));
        }

        return join(";", computed_rows, false);
    }


    std::vector<std::string>
    Component::ComputeSplitTopLevel(const std::string & s, char separator) const
    {
        std::vector<std::string> items;
        std::string current;
        int paren_depth = 0;
        int brace_depth = 0;

        for(char c : s)
        {
            if(c == '(')
                paren_depth++;
            else if(c == ')')
                paren_depth--;
            else if(c == '{')
                brace_depth++;
            else if(c == '}')
                brace_depth--;

            if(c == separator && paren_depth == 0 && brace_depth == 0)
            {
                items.push_back(trim(current));
                current.clear();
            }
            else
                current.push_back(c);
        }

        items.push_back(trim(current));
        return items;
    }


    void
    Component::ComputeCheckDepth(int depth) const
    {
        if(depth > 64)
            throw exception("Maximum compute recursion depth exceeded.", path_);
    }


    bool
    Component::ComputeLooksLikeNumber(const std::string & s) const
    {
        return is_number(trim(s));
    }


    bool
    Component::ComputeHasExplicitSyntax(const std::string & s) const
    {
        return s.find('@') != std::string::npos || s.find('{') != std::string::npos;
    }


    bool
    Component::ComputeIsPathLike(const std::string & s) const
    {
        if(s.empty())
            return false;

        if(s[0] == '.')
            return true;

        return ComputeSplitTopLevel(s, '.').size() > 1 || ComputeIsFunction(s);
    }


    bool
    Component::ComputeShouldReturnLiteral(const std::string & s, bool evaluate_final) const
    {
        if(evaluate_final)
            return false;

        if(ComputeIsPathLike(s))
            return false;

        if(ComputeHasExplicitSyntax(s))
            return false;

        return std::any_of(s.begin(), s.end(), [](unsigned char c)
        {
            return std::isalpha(c);
        });
    }


    bool
    Component::ComputeIsFunction(const std::string & s) const
    {
        return ends_with(s, ".size_x") || ends_with(s, ".size_y") || ends_with(s, ".size_z")
            || ends_with(s, ".rows") || ends_with(s, ".cols");
    }


    bool
    Component::ComputeHasTopLevelMath(const std::string & s)
    {
        int paren_depth = 0;
        int brace_depth = 0;

        for(size_t i = 0; i < s.size(); i++)
        {
            char c = s[i];
            if(c == '(')
                paren_depth++;
            else if(c == ')')
                paren_depth--;
            else if(c == '{')
                brace_depth++;
            else if(c == '}')
                brace_depth--;

            if(paren_depth != 0 || brace_depth != 0)
                continue;

            if(c == '+' || c == '*' || c == '/')
                return true;

            if(c == '-')
            {
                if(i == 0)
                    return true;
                char prev = s[i-1];
                if(prev != '.' && prev != '_' && !std::isalnum(static_cast<unsigned char>(prev)) && prev != '@' && prev != '}')
                    return true;
            }
        }

        return false;
    }


    std::string
    Component::ComputeLookupLocal(const std::string & name) const
    {
        if(name.empty())
            return "";

        if(info_.contains(name))
            return std::string(info_[name]);

        if(kernel().parameters.count(path_+'.'+name) && kernel().parameters.at(path_+'.'+name).resolved)
        {
            std::string value = kernel().parameters.at(path_+'.'+name).as_string();
            if(!value.empty())
                return value;
        }

        if(const Component * owner = GetValueOwner(name))
        {
            if(owner == this)
                return std::string(info_[name]);

            return const_cast<Component *>(owner)->Compute(std::string(owner->info_[name]));
        }

        return "";
    }


    std::string
    Component::ComputeList(const std::string & s, int depth)
    {
        ComputeCheckDepth(depth);

        auto items = ComputeSplitTopLevel(s, ',');
        if(items.size() <= 1)
            return ComputeScalar(trim(s), depth+1);

        std::vector<std::string> values;
        for(const auto & item : items)
            values.push_back(ComputeScalar(item, depth+1));

        return join(",", values, false);
    }


    std::string
    Component::ComputeCurlySubstitutions(const std::string & s, int depth)
    {
        ComputeCheckDepth(depth);

        std::string out = s;
        while(true)
        {
            int start = -1;
            int end = -1;
            int brace_depth = 0;

            for(size_t i = 0; i < out.size(); i++)
            {
                if(out[i] == '{')
                {
                    if(brace_depth == 0)
                        start = static_cast<int>(i);
                    brace_depth++;
                }
                else if(out[i] == '}')
                {
                    brace_depth--;
                    if(brace_depth < 0)
                        throw exception("Unmatched closing brace in compute expression.", path_);
                    if(brace_depth == 0)
                    {
                        end = static_cast<int>(i);
                        break;
                    }
                }
            }

            if(start == -1 && end == -1)
                return out;
            if(start == -1 || end == -1)
                throw exception("Unmatched curly brace in compute expression.", path_);

            std::string inner = out.substr(start+1, end-start-1);
            std::string value = ComputeScalar(inner, depth+1, true);
            out = out.substr(0, start) + value + out.substr(end+1);
        }
    }


    std::string
    Component::ComputeExpandSegment(const std::string & s, int depth)
    {
        ComputeCheckDepth(depth);

        std::string segment = ComputeCurlySubstitutions(trim(s), depth+1);
        if(segment.empty())
            return segment;

        if(segment[0] == '@')
        {
            std::string operand = trim(segment.substr(1));
            bool operand_path_like = ComputeIsPathLike(operand);

            if(!operand_path_like && !ComputeHasTopLevelMath(operand) && !ComputeHasExplicitSyntax(operand))
                return ComputeFinalSegment(operand, depth+1, true);

            return ComputeScalar(operand, depth+1, true);
        }

        if(ComputeHasTopLevelMath(segment))
            return ComputeMath(segment, depth+1);

        return segment;
    }


    std::string
    Component::ComputeFunction(const std::string & s, int depth)
    {
        ComputeCheckDepth(depth);

        std::string ss = s;

        if(ends_with(s, ".size_x"))
            return std::to_string(GetBuffer(rhead(ss, ".")).size_x());

        if(ends_with(s, ".size_y"))
            return std::to_string(GetBuffer(rhead(ss, ".")).size_y());

        if(ends_with(s, ".size_z"))
            return std::to_string(GetBuffer(rhead(ss, ".")).size_z());

        if(ends_with(s, ".rows"))
            return std::to_string(GetBuffer(rhead(ss, ".")).rows());

        if(ends_with(s, ".cols"))
            return std::to_string(GetBuffer(rhead(ss, ".")).cols());

        throw exception("Unknown compute function \""+s+"\".", path_);
    }


    std::string
    Component::ComputeFinalSegment(const std::string & s, int depth, bool evaluate_final)
    {
        ComputeCheckDepth(depth);

        std::string original = trim(s);
        if(original.empty())
            return "";

        // Final segments are lookup-by-request rather than lookup-by-default.
        // This avoids collisions between literal strings and defined attribute names.
        bool explicit_evaluation = ComputeHasExplicitSyntax(original);

        std::string segment = ComputeExpandSegment(original, depth+1);
        if(ComputeIsFunction(segment))
            return ComputeFunction(segment, depth+1);

        if(!explicit_evaluation && !evaluate_final)
            return segment;

        std::string value = ComputeLookupLocal(segment);
        if(value.empty())
        {
            if(evaluate_final)
                return "";
            if(ComputeLooksLikeNumber(segment))
                return formatNumber(std::stod(segment));
            return segment;
        }

        bool has_explicit_syntax = ComputeHasExplicitSyntax(value);

        bool has_alpha = std::any_of(value.begin(), value.end(), [](unsigned char c)
        {
            return std::isalpha(c);
        });

        if(has_explicit_syntax || (!has_alpha && (ComputeLooksLikeNumber(value) || ComputeHasTopLevelMath(value))))
            return ComputeScalar(value, depth+1);

        return value;
    }


    std::string
    Component::ComputePath(const std::string & s, int depth, bool evaluate_final)
    {
        ComputeCheckDepth(depth);

        std::string path = trim(s);
        if(path.empty())
            return "";

        bool absolute = !path.empty() && path[0] == '.';
        if(absolute)
            path = path.substr(1);

        auto parts = ComputeSplitTopLevel(path, '.');
        std::vector<std::string> segments;
        for(const auto & part : parts)
            if(!part.empty())
                segments.push_back(part);

        if(segments.empty())
            return "";

        Component * current = this;
        if(absolute)
        {
            std::string root_path = peek_head(path_, ".");
            current = kernel().components.at(root_path);
        }

        while(true)
        {
            current->ComputeCheckDepth(depth);

            if(segments.size() == 1)
                return current->ComputeFinalSegment(segments[0], depth+1, evaluate_final);

            if(segments.size() >= 2)
            {
                std::string function_candidate = segments[0] + "." + segments[1];
                if(current->ComputeIsFunction(function_candidate))
                    return current->ComputeFunction(function_candidate, depth+1);
            }

            std::string expanded = current->ComputeExpandSegment(segments[0], depth+1);
            auto extra = current->ComputeSplitTopLevel(expanded, '.');
            std::vector<std::string> rewritten;
            for(const auto & piece : extra)
                if(!piece.empty())
                    rewritten.push_back(piece);
            rewritten.insert(rewritten.end(), segments.begin()+1, segments.end());
            segments = rewritten;

            if(segments.empty())
                return "";

            if(segments.size() == 1)
                return current->ComputeFinalSegment(segments[0], depth+1, evaluate_final);

            std::string next_component = segments[0];
            Component * ancestor = current;
            while(ancestor)
            {
                if(std::string(ancestor->info_["name"]) == next_component)
                {
                    current = ancestor;
                    break;
                }
                ancestor = ancestor->parent_;
            }

            if(!ancestor)
                current = current->GetComponent(next_component);

            segments.erase(segments.begin());
        }
    }


    std::string
    Component::ComputeMath(const std::string & s, int depth)
    {
        ComputeCheckDepth(depth);

        expression e(s);
        std::map<std::string, std::string> vars;
        for(const auto & v : e.variables())
        {
            // Keep math variable syntax explicit so bare tokens remain string literals elsewhere.
            if(v.empty() || v[0] != '@')
                throw exception("Variables in compute expressions must use @ indirection: \""+v+"\".", path_);

            std::string value = ComputeExpandSegment(v, depth+1);
            if(value.empty())
                throw exception("Variable \""+v+"\" not defined.", path_);
            if(!is_number(value))
                throw exception("Variable \""+v+"\" resolved to non-numeric value \""+value+"\".", path_);
            vars[v] = value;
        }

        return formatNumber(e.evaluate(vars));
    }


    std::string
    Component::ComputeScalar(const std::string & s, int depth, bool evaluate_final)
    {
        ComputeCheckDepth(depth);

        std::string current = trim(s);
        if(current.empty())
            return "";

        for(int i = 0; i < 64; i++)
        {
            std::string previous = current;

            if(ComputeLooksLikeNumber(current))
                return formatNumber(std::stod(current));

            if(ComputeShouldReturnLiteral(current, evaluate_final))
                return current;

            if(!ComputeIsPathLike(current))
                current = ComputeCurlySubstitutions(current, depth+1);

            if(ComputeHasTopLevelMath(current))
                current = ComputeMath(current, depth+1);
            else
                current = ComputePath(current, depth+1, evaluate_final);

            current = trim(current);

            if(current != previous && ComputeShouldReturnLiteral(current, evaluate_final))
                return current;

            if(current == previous)
                return current;
        }

        throw exception("Compute expression did not converge.", path_);
    }


    std::vector<int> 
    Component::EvaluateSizeList(std::string & s) // return list of size from size list in string
    {
        std::vector<int> shape;
        for(std::string e : ComputeSplitTopLevel(s, ','))
        {
            e = trim(e);
            if(e.empty())
                continue;

            if(ends_with(e, ".size")) // special case: use shape function on input and push each dimension on list
            {
                auto & x = rsplit(e, ".", 1);
                std::string buffer_name = Compute(x.at(0));
                matrix m;
                Bind(m, buffer_name);
                for(auto d : m.shape())
                    shape.push_back(d);
            }
            else
            {
                std::string computed = Compute(e);
                if(computed.find(';') != std::string::npos)
                    throw std::invalid_argument("Size expression \""+e+"\" evaluated to a matrix.");

                for(std::string item : ComputeSplitTopLevel(computed, ','))
                {
                    item = trim(item);
                    if(item.empty())
                        continue;

                    int d = static_cast<int>(ComputeDouble(item));
                    if(d>0)
                        shape.push_back(d);
                    // else
                    //     throw std::invalid_argument("Value of "+e+" is non-positive or not found."); // Does not work since function can be called multiple times duing SetSizes
                }
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

        dictionary log_param;
        log_param["_tag"] = "parameter";
        log_param["name"] = "log_level";
        log_param["type"] = "number";
        log_param["control"] = "menu";
        log_param["options"] = "inherit,quiet,exception,end_of_file,terminate,fatal_error,warning,print,debug,trace";
        log_param["default"] = 0;

        info_["parameters"].push_back(log_param); // FIXME: Do we need to copy the dict?
    }



    Component::Component():
        parent_(nullptr),
        info_(kernel().current_component_info),
        path_(kernel().current_component_path)
    {
              // FIXME: Make sure there are empty lists. None of this should be necessary when dictionary is fixed

        if(info_["inputs"].is_null())
            info_["inputs"] = list();

        if(info_["outputs"].is_null())
            info_["outputs"] = list();

        if(info_["parameters"].is_null())
            info_["parameters"] = list();

        if(info_["groups"].is_null())
            info_["groups"] = list();

        if(info_["modules"].is_null())
            info_["modules"] = list();

        // Add log_level parameter to all components

        AddLogLevel();

        for(auto p: info_["parameters"])
            AddParameter(p);

        for(auto input: info_["inputs"])
            AddInput(input);

        for(auto output: info_["outputs"])
            AddOutput(output);

    // Set parent

        auto p = path_.rfind('.');
        if(p != std::string::npos)
            parent_ = kernel().components.at(path_.substr(0, p));
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
               int x = 0;
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
    Component::InputsReady(dictionary d,  input_map ingoing_connections) // FIXME: Handle optional inputs
    {

        Trace("\t\t\tComponent::InputReady", path_  + "." +  std::string(d["name"]));
        Kernel& k = kernel();

        std::string n = d["name"];   // ["attributes"]
        if(ingoing_connections.count(path_))
            for(auto & c : ingoing_connections.at(path_))
                if(k.buffers.at(c->source).rank()==0)
                    return false;
        return true;
    }


    void 
    Component::SetSourceRanges(const std::string & name, const std::vector<Connection *> & ingoing_connections) // FIXME:REMOVE
    {
        for(auto & c : ingoing_connections) // Copy source size to source_range if not set
        {
            if(c->source_range.empty())
                c->source_range = kernel().buffers[c->source].get_range();
            else if(c->source_range.rank() != kernel().buffers[c->source].rank())
                throw exception("Explicitly set source range dimensionality does not match source.", path_);
        }
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

         range output_matrix = kernel().buffers[c->source].get_range();  //**NEW  // FIXME: automatic matrix to range conection
            c->Resolve(output_matrix);  //**NEW  

            int s = c->source_range.size() * c->delay_range_.trim().b_[0];
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

        if(d.is_set("use_alias"))
        {
            begin_index = 0;
            for(auto & c : ingoing_connections.at(full_name))
            {
                int s = c->source_range.size() * c->delay_range_.trim().b_[0];
                if(c->alias_.empty())
                    kernel().buffers[full_name].push_label(0, c->source, s); // WAS: kernel().buffers[d.at(full_name)].push_label(0, c->source, s);
                else
                    kernel().buffers[full_name].push_label(0, c->alias_, s); // WAS: kernel().buffers[d.at(full_name)].push_label(0, c->alias_, s);
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
            range output_matrix = kernel().buffers[ingoing_connections.begin()->second[0]->source].get_range();
            if(output_matrix.empty())
                return 0;
    
            kernel().buffers[full_name].realloc(output_matrix.extent());

            Trace("\t\t\tComponent::SetInputSize Simple Alloc" + std::string(input_size), full_name);

            return 1;
        }

        for(auto c : ingoing_connections.at(full_name))
        {
            range output_matrix = kernel().buffers[c->source].get_range();  // FIXME: automatic matrix to range conection
            if(output_matrix.empty())
                return 0;
            input_size.extend(c->Resolve(output_matrix));
        }
        kernel().buffers[full_name].realloc(input_size.extent());
        Trace("\t\t\tComponent::SetInputSize Alloc" + std::string(input_size), full_name);

        // Set alias if requested and there is only a single input

        if(d.is_set("use_alias"))
        {
            auto ic = ingoing_connections.at(full_name);
            if(ic.size() == 1 && !ic[0]->alias_.empty())
                kernel().buffers[name].set_name(ic[0]->alias_);
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

        range output_size; // FIXME: rename output_range
        std::string name = d.at("name");
        std::string full_name = path_ +"."+ name;

        if(!ingoing_connections.count(full_name))
            return 0;

        for(auto c : ingoing_connections.at(full_name))
        {
            range output_matrix = kernel().buffers[c->source].get_range();  // FIXME: automatic matrix to range conection
            
            if(output_matrix.empty())
                return 0;
            
            output_size.extend(c->Resolve(output_matrix));
        }
        kernel().buffers[full_name].realloc(output_size.extent()); // FIXME: realloc with range argument
      Trace("\t\t\t\t\tComponent:: Alloc" + std::string(output_size), path_);

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
    Module::SetOutputSize(dictionary d, input_map ingoing_connections)
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
            if(parameter_name == "log_level" || parameter_name == "color")
                continue;

            parameter p;
            Bind(p, parameter_name);
            //std::cout << "Parameter: " << d["name"] << std::endl;

            if(p.type == string_type)
                check_sum += prime_number.next() * character_sum(p);
            else
            if(p.type == matrix_type)
                check_sum += prime_number.next() * p.matrix_value->size();
            else
                check_sum += prime_number.next() *  p.as_int(); // FIXME: convert to long later 
        }
        // std::cout << "Check sum: " << check_sum << std::endl;
    }



    // Connection

    Connection::Connection(std::string s, std::string t, range & delay_range, std::string alias)
    {
        source = peek_head(s, "[");
        source_range = range(peek_tail(s, "[", true));
        target = peek_head(t, "[");
        target_range = range(peek_tail(t, "[", true));
        delay_range_ = delay_range;
        flatten_ = false;
        alias_ = alias;
    }


    range 
    Connection::Resolve(const range & source_output)
    {
        if(source_output.empty())
            return 0;
            // throw exception("Cannot resolve connection. Source output is empty."); //  FIXME: What is correct here? ************

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
        int delay_size = delay_range_.trim().b_[0];
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
            for(int i=delay_range_.a_[0]; i<delay_range_.b_[0]; i++)  // FIXME: assuming continous range (inc==1)
            {   
                matrix s = k.circular_buffers[source].get(i);

                for(auto ix=source_range; ix.more(); ix++)
                {
                    int source_index = s.compute_index(ix.index());
                    ctarget[target_offset++] = (*(s.data_))[source_index];
                }
            }
        }

        else if(delay_range_.a_[0]+1 == delay_range_.b_[0]) // Copy indexed delayed value with single delay
        {
            //std::cout << source << " =D=> " << target << std::endl;
            matrix s = k.circular_buffers[source].get(delay_range_.a_[0]);
            k.buffers[target].copy(s, target_range, source_range);
        }

        else // Copy indexed delayed values with more than one element
        {
            //std::cout << source << " =DD=> " << target << std::endl;
            for(int i=delay_range_.a_[0]; i<delay_range_.b_[0]; i++)  // FIXME: assuming continous range (inc==1)
            {   
                matrix s = k.circular_buffers[source].get(i);
                int target_ix = i - delay_range_.a_[0]; // trim!
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
        if(!alias_.empty())
            std::cout << " \"" << alias_ << "\"";
        std::cout << '\n'; 
    }


std::string
Connection::Info() const
{
    std::string s = source + delay_range_.curly() +  std::string(source_range) + " => " + target  + std::string(target_range);
        if(!alias_.empty())
            s+=  " \"" + alias_ + "\"";
    return s;
}

// Class

    Class::Class(std::string n, std::string p) : module_creator(nullptr), name(n), path(p), info_()
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

    Request::Request(std::string  uri, long sid, std::string b):
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

        for(auto [_,c] : components) // components contains pointers so nee need to be explcitly deleted
            // if(NOT PERSISTENT)
                delete c;
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
        session_id = new_session_id(); // FIXME: Probably not necessary - done in clear
        SetUp(); // FIXME: Catch exceptions if new fails
        needs_reload = false;
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
        return (stop_after!= -1 &&  tick >= stop_after) || global_terminate.load();
    }


    void 
    Kernel::ScanClasses(std::string path) // FIXME: Add error handling
    {
        if(!std::filesystem::exists(path))
        {
            std::cout << "Could not scan for classes \""+path+"\". Directory not found." << std::endl;
            return;
        }
        for(auto& p: std::filesystem::recursive_directory_iterator(path))
            if(std::string(p.path().extension())==".ikc")
            {
                std::string name = p.path().stem();
                classes[name].path = p.path();
                classes[name].info_.load_xml(p.path());

                // Inject default parameters

                if(classes[name].info_["parameters"].is_null())
                    classes[name].info_["parameters"] = list();

                bool has_log_level = false;
                bool has_color = false;
                for(auto parameter : classes[name].info_["parameters"])
                {
                    std::string parameter_name = parameter["name"];
                    if(parameter_name == "log_level")
                        has_log_level = true;
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
                    classes[name].info_["parameters"].push_back(log_param);
                }

                if(!has_color)
                {
                    dictionary color_param;
                    color_param["_tag"] = "parameter";
                    color_param["name"] = "color";
                    color_param["type"] = "string";
                    color_param["default"] = "black";
                    color_param["description"] = "Selected ui color";
                    color_param["control"] = "ui_color";
                    classes[name].info_["parameters"].push_back(color_param);
                }
            }
    }


    void 
    Kernel::ScanFiles(std::string path, bool system)
    {
        if(!std::filesystem::exists(path))
        {
            std::cout << "Could not scan for files in \""+path+"\". Directory not found." << std::endl;
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
        std::cout << "\nClasses:" << std::endl;
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
            return; // FIXME: Is this an error?

        auto c = components.at(name.substr(0, i));
        std::string parameter_name = name.substr(i+1, name.size());
        c->ResolveParameter(p, parameter_name);
    }


    void 
    Kernel::ResolveParameters() // Find and evaluate value or default // FIXME: return success
    {
        // All all componenets to initialize parameters programmatically

        for(auto & m : components)
            m.second->SetParameters();

        // resolve
        bool ok = true;
        for (auto p=parameters.rbegin(); p!=parameters.rend(); p++) // Reverse order equals outside in in groups
        {
            std::size_t i = p->first.rfind(".");
            // Find component and parameter_name and resolve
            i = p->first.rfind(".");
            if(i != std::string::npos)
            {
                auto c = components.at(p->first.substr(0, i));
                std::string parameter_name = p->first.substr(i+1, p->first.size());
                  ok &= c->ResolveParameter(p->second, parameter_name);
            }
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

        // Loop enough for all sizes to be calculated // FIXME: Restore progress calculation *******************
        for(int i=0; i<components.size(); i++)
            for(auto & [n, c] : components)
                c->SetSizes(ingoing_connections);

            for(auto & [n, c] : components)
                c->CheckRequiredInputs();
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
        std::cout << "\nComponents:" << std::endl;
        for(auto & m : components)
            m.second->print();
    }


    void 
    Kernel::ListConnections()
    {
        std::cout << "\nConnections:" << std::endl;
        for(auto & c : connections)
            c.Print();
    }


    void 
    Kernel::ListInputs()
    {
        std::cout << "\nInputs:" << std::endl;
        for(auto & i : buffers)
            std::cout << "\t" << i.first <<  i.second.shape() << std::endl;
    }


   void Kernel::ListOutputs()
    {
        std::cout << "\nOutputs:" << std::endl;
        for(auto & o : buffers)
            std::cout  << "\t" << o.first << o.second.shape() << std::endl;
    }


    void 
    Kernel::ListBuffers()
    {
        std::cout << "\nBuffers:" << std::endl;
        for(auto & i : buffers)
            std::cout << "\t" << i.first <<  i.second.shape() << std::endl;
    }


    void 
    Kernel::ListCircularBuffers()
    {
        if(circular_buffers.empty())
            return;

        std::cout << "\nCircularBuffers:" << std::endl;
        for(auto & i : circular_buffers)
            std::cout << "\t" << i.first <<  " " << i.second.buffer_.size() << " " << i.second.buffer_[0].rank() << i.second.buffer_[0].shape() <<  std::endl;
    }


    void
    Kernel::ListTasks()
    {
        for(auto & task_group : tasks)
        {
            std::cout << "\nTasks:" << std::endl;
            for(auto & task: task_group)
                std::cout << "\t" << task->Info() << std::endl;
        }
    }

   void 
   Kernel::ListParameters()
    {
        std::cout << "\nParameters:" << std::endl;
        for(auto & p : parameters)
            std::cout  << "\t" << p.first << ": " << p.second << std::endl;
    }


    void 
    Kernel::PrintLog()
    {
        for(auto & s : log)
            std::cout << "ikaros: " << s.level_ << ": " << s.message_ << std::endl;
        log.clear();
    }


    Kernel::Kernel():
        tick(0),
        run_mode(run_mode_pause),
        //(false),
        tick_time_usage(0),
        actual_tick_duration(0), // FIME: Use desired tick duration here
        idle_time(0),
        stop_after(-1),
        tick_duration(1),
        lag(0),
        lag_min(0),
        lag_max(0),
        lag_sum(0),
        shutdown(false),
        session_id(new_session_id()),
        needs_reload(true)
    {
        cpu_cores = std::thread::hardware_concurrency();
        thread_pool = new ThreadPool(cpu_cores > 1 ? cpu_cores-1 : 1); // FIXME: optionally use ikg parameters
        //thread_pool = new ThreadPool(1); // FIXME: optionally use ikg parameters
    }


    void
        Kernel::PrintProfiling()
        {
            for(auto & c : components)
                c.second->profiler_.print(c.second->path_);
        }



    // Functions for creating the network

    void 
    Kernel::AddInput(std::string name, dictionary parameters) // FIXME: use name as argument instead of parameters
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
    Kernel::AddGroup(dictionary info, std::string path)
    {
        current_component_info = info;
        current_component_path = path;

        if(components.count(current_component_path)> 0)
            throw build_failed("Module or group named \""+current_component_path+"\" already exists.", path);

        components[current_component_path] = new Group(); // Implicit argument passing as for components
    }


    void 
    Kernel::InstantiatePythonModule(dictionary & info, const std::string & path)
    {
        current_component_info = info;
        current_component_path = path+"."+std::string(info["name"]);

        if(!classes.count("PythonModule") || classes["PythonModule"].module_creator == nullptr)
            throw build_failed("Internal PythonModule runtime class is not installed.", path);

        components[current_component_path] = classes["PythonModule"].module_creator();
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
            BuildGroup(info, path); // FIXME: This is probably not working correctly
        }
        else
            components[current_component_path] = classes[classname].module_creator();
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
            Component * c = components.at(path);
            classname = c->Compute(classname);
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
        bool has_color = false;
        for(auto parameter : info["parameters"])
        {
            std::string parameter_name = parameter["name"];
            if(parameter_name == "log_level")
                has_log_level = true;
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
         std::string source = path+"."+std::string(info["source"]);   // FIXME: Look for global paths later - string conversion should not be necessary
         std::string target = path+"."+std::string(info["target"]);

         std::string delay_range = info.contains_non_null("delay") ? info["delay"] : "";// FIXME: return "" if name not in dict - or use contains *********
         std::string alias = info.contains_non_null("alias") ? info["alias"] : "";// FIXME: return "" if name not in dict - or use contains *********

        if(delay_range.empty() || delay_range=="null")  // FIXME: return "" if name not in dict - or use contains *********
            delay_range = "[1]";
        else if(delay_range[0] != '[')
            delay_range = "["+delay_range+"]";
        range r(delay_range);
        connections.push_back(Connection(source, target, r, alias));
    }



    void Kernel::LoadExternalGroup(dictionary & d)
    {
        std::string path = d["external"];
        dictionary external;
        external.load_xml(path);
        external["name"] = d["name"]; // FIXME: Just in case - check for errors later
        d.merge(external);
        d.erase("external");
    }



    void 
    Kernel::BuildGroup(dictionary d, std::string path) // Traverse dictionary and build all items at each level, FIXME: rename AddGroup later
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

        d["filename"] = options_.stem();

        if(d.contains("stop"))
            stop_after = d["stop"];

        if(d.contains("tick_duration"))
            tick_duration = d["tick_duration"];
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
                    BuildGroup(d);
                    info_ = d;
                    session_id = new_session_id(); 
                    Notify(msg_print, u8"Loaded "s+options_.full_path());
                    SetUp();
                    CalculateCheckSum();
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
            Notify(msg_warning, e.what(), e.path()); // Do not exit if not in batch mode // FIXME: Check this
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
            std::cout << "Correct Check Sum: " << calculated_check_sum << std::endl;
        else
        {
            std::string msg = "Incorrect Check Sum: "+std::to_string(calculated_check_sum)+" != "+std::to_string(correct_check_sum);
            Notify(msg_fatal_error, msg);
            if(info_.is_set("batch_mode"))
            {
                StopHTTPServer();
                delete thread_pool;
                thread_pool = nullptr;
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


    void 
    Kernel::Save() // Simple save function in present file from kernel data
    {
        std::cout << "ERROR: SAVE SHOULD NEVER BE CALLED" << std::endl;

        std::string data = xml();

        //std::cout << data << std::endl;

        std::ofstream file;
        std::string filename = add_extension(info_["filename"], ".ikg");        // FIXME: ADD DIRECTORY PATH – USER DATA ********
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
            socket =  new ServerSocket(port);
        }
        catch (const exception& e)
        {
            throw fatal_error("Ikaros is unable to start a webserver on port "+std::to_string(port)+". Make sure no other ikaros process is running and try again.");
        }

        httpThread = new std::thread(Kernel::StartHTTPThread, this);
    }


    void
    Kernel::StopHTTPServer()
    {
        shutdown.store(true, std::memory_order_release);

        if(socket != nullptr)
            socket->StopListening();

        if(httpThread != nullptr)
        {
            httpThread->join();
            delete httpThread;
            httpThread = nullptr;
        }

        if(socket != nullptr)
        {
            delete socket;
            socket = nullptr;
        }
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
    Kernel::dfsCycleCheck(const std::string& node, const std::unordered_map<std::string, std::vector<std::string>>& graph, std::unordered_set<std::string>& visited, std::unordered_set<std::string>& recStack) 
    {
        if(recStack.find(node) != recStack.end())
            return true;

        if(visited.find(node) != visited.end())
            return false;

        visited.insert(node);
        recStack.insert(node);

        if(graph.find(node) != graph.end()) 
            for (const std::string& neighbor : graph.at(node)) 
                if(dfsCycleCheck(neighbor, graph, visited, recStack)) 
                    return true;
        recStack.erase(node);
        return false;
    }



    bool 
    Kernel::hasCycle(const std::vector<std::string>& nodes, const std::vector<std::pair<std::string, std::string>>& edges) 
    {
        std::unordered_map<std::string, std::vector<std::string>> graph;
        for (const auto& edge : edges)
            graph[edge.first].push_back(edge.second);

        std::unordered_set<std::string> visited;
        std::unordered_set<std::string> recStack;

        for (const std::string& node : nodes)
            if(visited.find(node) == visited.end() &&  (dfsCycleCheck(node, graph, visited, recStack)))
                    return true;

        return false;
    }

    void 
    Kernel::dfsSubgroup(const std::string& node, const std::unordered_map<std::string, std::vector<std::string>>& graph, std::unordered_set<std::string>& visited, std::vector<std::string>& component) 
    {
        visited.insert(node);
        component.push_back(node);

        if(graph.find(node) != graph.end()) 
        {
            for (const std::string& neighbor : graph.at(node)) 
            {
                if(visited.find(neighbor) == visited.end()) 
                    dfsSubgroup(neighbor, graph, visited, component);
            }
        }
    }


    std::vector<std::vector<std::string>> 
    Kernel::findSubgraphs(const std::vector<std::string>& nodes, const std::vector<std::pair<std::string, std::string>>& edges) 
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
                dfsSubgroup(node, graph, visited, component);
                components.push_back(component);
            }
        }

        return components;
    }


    void 
    Kernel::topologicalSortUtil(const std::string& node, const std::unordered_map<std::string, std::vector<std::string>>& graph, std::unordered_set<std::string>& visited, std::stack<std::string>& Stack) 
    {
        visited.insert(node);

        if(graph.find(node) != graph.end()) 
        {
            for (const std::string& neighbor : graph.at(node)) 
            {
                if(visited.find(neighbor) == visited.end())
                    topologicalSortUtil(neighbor, graph, visited, Stack);
            }
        }
        Stack.push(node);
    }



    std::vector<std::string> 
    Kernel::topologicalSort(const std::vector<std::string>& component, const std::unordered_map<std::string, std::vector<std::string>>& graph) 
    {
        std::unordered_set<std::string> visited;
        std::stack<std::string> Stack;

        for (const std::string& node : component) 
            if(visited.find(node) == visited.end())
                topologicalSortUtil(node, graph, visited, Stack);

        std::vector<std::string> sortedSubgraph;
        while (!Stack.empty()) 
        {
            sortedSubgraph.push_back(Stack.top());
            Stack.pop();
        }

        return sortedSubgraph;
    }


    std::vector<std::vector<std::string>>
    Kernel::sort(std::vector<std::string> nodes, std::vector<std::pair<std::string, std::string>> edges)
    {
        if(hasCycle(nodes, edges)) 
            throw setup_failed("Network has zero-delay loops");
        else 
        {

            std::vector<std::vector<std::string>> components = findSubgraphs(nodes, edges);

            // Rebuild the original graph for directed edges
            std::unordered_map<std::string, std::vector<std::string>> graph;
            for (const auto& edge : edges) {
                graph[edge.first].push_back(edge.second);
            }

            std::vector<std::vector<std::string>>  result;

            for (const auto& component : components) {
                std::vector<std::string> sortedSubgraph = topologicalSort(component, graph);
                result.push_back(sortedSubgraph);
                for (const auto& node : sortedSubgraph) {
                }
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
            task_map[s] = c; // Save in task map
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

        auto r = sort(nodes, arcs);

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
                task->Tick();
    }



    void
    Kernel::SetUp()
    {
        try
        {
            PruneConnections();
            SortTasks();
            ResolveParameters();
            //ListParameters();
            CalculateDelays();
            CalculateSizes();
            //ListConnections(); // FIXME: Add flags for this
            //ListInputs();
            ListOutputs();

            InitCircularBuffers();
            InitComponents();

            if(info_.is_set("info"))
            {
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
                    timer.SetStartTime(double(tick+1)*tick_duration); // Fake time increase // FIXME: remove sleep in batch mode DOES NOT LOOK CORRECT
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
                            std::cout  << "Ikaros is lagging " << lag << " seconds behind real time." << std::endl;
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
                    }
                    catch(std::exception & e)
                    {
                        //std::cout << e.what() << std::endl;
                        Notify(msg_fatal_error, (e.what()));
                        return;                 // FIXME: THROW INSTEAD
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
            static std::mutex mtx;
            std::lock_guard<std::mutex> lock(mtx); // Lock the mutex

            const std::string timestamped_message = "[" + TimeString(GetTime()) + "] " + message;

            log.push_back(Message(msg, timestamped_message, path));

            std::cout << timestamped_message;
            if(!path.empty())
                std::cout  << " ("<<path << ")";
            std::cout << std::endl;

            if(msg <= msg_fatal_error)
                global_terminate = true;
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

    void
    Kernel::SendImage(matrix & image, std::string & format) // Compress image to jpg and send from memory after base64 encoding
    {
        long size = 0;
        unsigned char * jpeg = nullptr;
        size_t output_length = 0 ;
        
        if(format=="rgb" && image.rank() == 3 && image.size(0) == 3)
            jpeg = (unsigned char *)create_color_jpeg(size, image, 90);

        else if(format=="gray" && image.rank() == 2)
            jpeg = (unsigned char *)create_gray_jpeg(size, image, 0, 1, 90);

        else if(image.rank() == 2) // taking our chances with the format...
            jpeg = (unsigned char *)create_pseudocolor_jpeg(size, image, 0, 1, format, 90);

            if(!jpeg)
            {
                socket->Send("\"\"");
                return;
            }

        char * jpeg_base64 = base64_encode(jpeg, size, &output_length);
        socket->Send("\"data:image/jpeg;base64,");
        bool ok = socket->SendData(jpeg_base64, output_length);
        socket->Send("\"");
        destroy_jpeg(jpeg);
        free(jpeg_base64);
    }



    void
    Kernel::Stop()
    {
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
        PrintProfiling();
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
    Kernel::DoSendDataHeader()
    {
        dictionary header({
            {"Session-Id", std::to_string(session_id)},
            {"Package-Type", "data"},
            {"Content-Type", "application/json"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"},
            {"Expires", "0"}
        });
        socket->SendHTTPHeader(&header);
    }


    void
    Kernel::DoSendDataStatus()
    {
        std::string nm = std::string(info_["filename"]);    // FIXME ******************************
        if(nm.find("/") != std::string::npos)
            nm = rtail(nm,"/");

        socket->Send("\t\"file\": \"%s\",\n", nm.c_str());

#if DEBUG
        socket->Send("\t\"debug\": true,\n");
#else
        socket->Send("\t\"debug\": false,\n");
#endif

            socket->Send("\t\"state\": %d,\n", run_mode.load());
        if(stop_after != -1)
        {
            socket->Send("\t\"tick\": \"%d / %d\",\n", tick, stop_after);
            socket->Send("\t\"progress\": %f,\n", double(tick)/double(stop_after));
        }
        else
        {
            socket->Send("\t\"progress\": 0,\n");
        }

        // Timing information

        double uptime = uptime_timer.GetTime();
        double total_time = GetTime();

        socket->Send("\t\"timestamp\": %ld,\n", GetTimeStamp());
        socket->Send("\t\"uptime\": %.2f,\n", uptime);
        socket->Send("\t\"tick_duration\": %f,\n", tick_duration);
        socket->Send("\t\"cpu_cores\": %d,\n", cpu_cores);
    
        switch(run_mode)
        {
            case run_mode_stop:
                socket->Send("\t\"tick\": \"-\",\n");
                socket->Send("\t\"time\": \"-\",\n");
                socket->Send("\t\"ticks_per_s\": \"-\",\n");
                socket->Send("\t\"actual_duration\": \"-\",\n");
                socket->Send("\t\"lag\": \"-\",\n");
                socket->Send("\t\"time_usage\": 0,\n");
                socket->Send("\t\"cpu_usage\": 0,\n"); 
                break;

            case run_mode_pause:
                socket->Send("\t\"tick\": %lld,\n", GetTick());
                socket->Send("\t\"time\": %.2f,\n", GetTime());
                socket->Send("\t\"ticks_per_s\": \"-\",\n");
                socket->Send("\t\"actual_duration\": \"-\",\n");
                socket->Send("\t\"lag\": \"-\",\n");
                socket->Send("\t\"time_usage\": %f,\n", actual_tick_duration> 0 ? tick_time_usage/actual_tick_duration : 0);
                socket->Send("\t\"cpu_usage\": %f,\n", cpu_usage);  
                break;

            case run_mode_realtime:
            default:
                socket->Send("\t\"tick\": %lld,\n", GetTick());
                socket->Send("\t\"time\": %.2f,\n", GetTime());
                socket->Send("\t\"ticks_per_s\": %.2f,\n", tick>0 ? double(tick)/total_time: 0);
                socket->Send("\t\"actual_duration\": %f,\n", actual_tick_duration);
                socket->Send("\t\"lag\": %f,\n", lag);
                socket->Send("\t\"time_usage\": %f,\n", actual_tick_duration> 0 ? tick_time_usage/actual_tick_duration : 0);
                socket->Send("\t\"cpu_usage\": %f,\n", cpu_usage);
                break;
        }

    }


    void
    Kernel::DoSendLog(Request & request)
    {
        socket->Send(",\n\"log\": [");
        std::string sep;
        for(auto line : log)
        {
            std::string s = line.json();
            socket->Send(sep +line.json());
            sep = ",";
        }
        socket->Send("]");
        if(!log.empty() )
        log.clear();
    }


    void
    Kernel::DoSendData(Request & request)
    {    
        DoSendDataHeader();

        socket->Send("{\n");

        DoSendDataStatus();

        socket->Send("\t\"data\":\n\t{\n");

        std::string data = request.parameters["data"];  // FIXME: Check that it exists ******** or return ""
        std::string root = request.component_path;

        std::string sep = "";
        bool sent = false;

        while(!data.empty()) // FIXME: Check that we do not run out of time here and break if next tick is about to start.
        {
            std::string source = head(data, ",");
            std::string key = source;
            std::string format = rtail(source, ":");

            if((source.find('@') != std::string::npos || source.find('{') != std::string::npos) && components.count(root) > 0)
            {
                Component * c = components[root]; // FIXME: Handle exceptions
                source = c->Compute(source);
            }

            std::string source_with_root = root +"."+source;

            if(key[0] == '.')
                source_with_root = key.substr(1); // Global path (keep `key` intact)

            std::string component_path = peek_rhead(source_with_root, ".");
            
            std::string attribute = peek_rtail(source_with_root, ".");

            if(buffers.count(source_with_root))
            {
                if(format.empty())
                {
                    sent = socket->Send(sep + "\t\t\"" + key + "\": "+buffers[source_with_root].json());
                }
                else if(format=="rgb")
                { 
                        // sent = socket->Send(sep + "\t\t\"" + key + ":"+format+"\": ");
                        sent = socket->Send(sep + "\t\t\"" + key + "\": ");
                        SendImage(buffers[source_with_root], format);
                }
                else if(format=="gray" || format=="red" || format=="green" || format=="blue" || format=="spectrum" || format=="fire")
                { 
                    sent = socket->Send(sep + "\t\t\"" + key + "\": ");
                        SendImage(buffers[source_with_root], format);
                }
            }
            else if(parameters.count(source_with_root))
            {
                sent = socket->Send(sep + "\t\t\"" + key + "\": "+parameters[source_with_root].json());
            }

            else if(components.count(component_path)) // Use module function to get value
            {
                    std::string json_data = components[component_path]->json(attribute);

                if(!json_data.empty())
                {
                    socket->Send(sep);
                    std::string s = "\t\t\"" + source + "\": "+json_data;
                    socket->Send(s);
                    sep = ",\n";
                }
            }
            else
            {
                // ERROR: No such buffer or parameter
            }

            if(sent)
                sep = ",\n";
        }

        socket->Send("\n\t}");
        DoSendLog(request);
        socket->Send(",\n\t\"has_data\": 1\n"); // "+std::to_string(!tick_is_running)+" new tick has started during sending; there may be data but it cannot be trusted // FIXME: Never happens
        socket->Send("}\n");

        //sending_ui_data = false;
    }


    void
    Kernel::DoNew(Request & request)
    {
        New();
        DoUpdate(request);  // FIXME: OR SEND NETWORK
    }


    void
    Kernel::DoOpen(Request & request)
    {
        std::string file = request.parameters["file"];
        std::string where = request.parameters["where"];
        Stop();
        if(where == "system")
            options_.path_ = system_files.at(file);
        else
            options_.path_ = user_files.at(file);
            try
            {
                LoadFile();
            }
            catch(const setup_failed& e)
            {
                //std::cerr << e.what() << '\n';
                Notify(msg_warning, "File \""+file+"\" could not be set-up: "+e.message()); // FIXME: better error message - alert? HTTP reply with error code
                // New();
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
                Notify(msg_warning, "File \""+file+"\" could not be loaded, built or set-up"); // FIXME: better error message - alert? HTTP reply with error code
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
        
            std::cout << request.body.size() << std::endl;
             d = parse_json(request.body);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            std::cout << "INTERNAL ERROR: Could not parse json.\n" << request.body << std::endl;
            return;
        }

        // Sanitize file name

        std::filesystem::path path = add_extension(std::string(d["filename"]), ".ikg");
        std::string filename = path.filename();

        d["filename"] = ""; // Do not include filename in file // FIXME: Remove key from dict
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


    void
    Kernel::DoSendFile(std::string file)
    {
        if(file[0] == '/')
            file = file.erase(0,1); // Remove initial slash

        // if(socket->SendFile(file, ikc_dir))  // Check IKC-directory first to allow files to be overriden
        //    return;

        //std::cout << "Sending file: " << file << std::endl;

        if(socket->SendFile(file, user_dir))   // Look in user directory
            return;

        if(socket->SendFile(file, webui_dir))   // Now look in WebUI directory
            return;

        if(socket->SendFile(file, std::string(webui_dir)+"Images/"))   // Now look in WebUI/Images directory
            return;

        if(socket->SendFile(file, webui_dir+"../"))   // Now look in Source directory
            return;

    

        /*
 
        
        file = "error." + rcut(file, ".");
        if(socket->SendFile("error." + rcut(file, "."), webui_dir)) // Try to send error file
            return;

        DoSendError();
        */
    }


    void
    Kernel::DoSendNetwork(Request & request)
    {
        std::string s = json(); 

        //std::cout << s << std::endl;

        dictionary rtheader({
            {"Session-Id", std::to_string(session_id)},
            {"Package-Type", "network"},
            {"Content-Type", "application/json"},
            {"Content-Length", std::to_string(s.size())}
        });
        socket->SendHTTPHeader(&rtheader);
        socket->SendData(s.c_str(), int(s.size()));
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
            run_mode = run_mode_pause; // FIXME: Probably not necessary
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
         if(!buffers.count(request.component_path))
            {
                dictionary header({
                    {"Content-Type", "text/plain"},
                    {"Cache-Control", "no-cache, no-store"},
                    {"Pragma", "no-cache"}
                });
                socket->SendHTTPHeader(&header);
        
                socket->Send("Buffer \""+request.component_path+"\" can not be found");
                return;
            }

        if(buffers[request.component_path].rank() > 2)
        {
            dictionary header({
                {"Content-Type", "text/plain"},
                {"Cache-Control", "no-cache, no-store"},
                {"Pragma", "no-cache"}
            });
            socket->SendHTTPHeader(&header);

            socket->Send("Rank of matrix != 2. Cannot be displayed as CSV");
            return;
        }

        dictionary header({
            {"Content-Type", "text/plain"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        socket->SendHTTPHeader(&header);

        socket->Send(buffers[request.component_path].csv());
    }



    void
    Kernel::DoCommand(Request & request)
    {
        try
       {
            std::string root;
         
            if(request.parameters.contains("root"))
                root = std::string(request.parameters["root"]);

            std::string key = request.component_path;
            if(key[0] == '.')
                key = key.substr(1); // Global path


            if(!components.count(key))
            {
                Notify(msg_warning, "Component '"+request.component_path+"' could not be found.");
                DoSendData(request);
                return;
            }
   
            if(!request.body.empty()) // FIXME: Move to request and check content-type first
                request.parameters = parse_json(request.body);

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
                    
                if(p.matrix_value->rank() == 1)
                    (*p.matrix_value)(x)= value;
                else if(p.matrix_value->rank() == 2)
                    (*p.matrix_value)(y,x)= value; // Is this correct?
                else
                    ;   // FIXME: higher dimensional parameter
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
        if(request.session_id != session_id)
            DoSendNetwork(request);
        else 
            DoSendData(request);
    }


    void
    Kernel::DoNetwork(Request & request)
    {
        DoSendNetwork(request);
    }


    void
    Kernel::DoSendClasses(Request & request)
    {
        dictionary header({
            {"Content-Type", "text/json"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        socket->SendHTTPHeader(&header);
        socket->Send("{\"classes\":[\n\t\"");
        std::string s = "";
        for(auto & c: classes)
        {
            socket->Send(s.c_str());
            socket->Send(c.first.c_str());
            s = "\",\n\t\"";
        }
        socket->Send("\"\n]\n}\n");
    }



    void
    Kernel::DoSendClassInfo(Request & request)
    {
        dictionary header({
            {"Content-Type", "text/json"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        socket->SendHTTPHeader(&header);
        socket->Send("{\n");
        std::string s = "";
        for(auto & c: classes)
        {
            socket->Send(s);
            socket->Send("\""+c.first+"\": ");
            dictionary class_info = c.second.info_.copy();
            class_info["path"] = std::filesystem::path(c.second.path).parent_path().string();
            socket->Send(class_info.json());
            s = ",\n\t";
        }
        socket->Send("\n}\n");
    }



    void
    Kernel::DoSendClassReadMe(Request & request)
    {
        dictionary header({
            {"Content-Type", "text/plain; charset=utf-8"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        socket->SendHTTPHeader(&header);

        if(!request.parameters.contains("class"))
        {
            socket->Send("No class selected.");
            return;
        }

        std::string class_name = request.parameters["class"];
        if(!classes.count(class_name))
        {
            socket->Send("Class not found: " + class_name);
            return;
        }

        std::filesystem::path class_path = classes[class_name].path;
        if(class_path.empty())
        {
            socket->Send("No class path available for: " + class_name);
            return;
        }

        std::filesystem::path readme_path = class_path.parent_path() / "ReadMe.md";
        std::ifstream readme_file(readme_path);
        if(!readme_file.is_open())
        {
            socket->Send("No ReadMe.md found for: " + class_name);
            return;
        }

        std::string content((std::istreambuf_iterator<char>(readme_file)), std::istreambuf_iterator<char>());
        socket->Send(content);
    }



    void
    Kernel::DoSendFileList(Request & request)
    {
        // Scan for files

        system_files.clear();
        user_files.clear();
        ScanFiles(options_.ikaros_root+"/Source/Modules");
        ScanFiles(options_.ikaros_root+"/UserData", false);

        // Send result

        dictionary header({
            {"Content-Type", "text/json"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        socket->SendHTTPHeader(&header);
        std::string sep;
    socket->Send("{\"system_files\":[\n\t\"");
        for(auto & f: system_files)
        {
            socket->Send(sep);
            socket->Send(f.first);
            sep = "\",\n\t\"";
        }
    socket->Send("\"\n],\n");
    
    sep = "";
    socket->Send("\"user_files\":[\n\t\"");
        for(auto & f: user_files)
        {
            socket->Send(sep);
            socket->Send(f.first);
            sep = "\",\n\t\"";
        }
    socket->Send("\"\n]\n");

    socket->Send("}\n");
    }


    void
    Kernel::DoSendError()
    {
    dictionary header({
        {"Content-Type", "text/plain"}
    });
    socket->SendHTTPHeader(&header);
    socket->Send("ERROR\n");
    }


    void
    Kernel::HandleHTTPRequest()
    {
        long sid = 0;
        if(socket->header.contains_non_null("Session-Id"))
            sid = atol(std::string(socket->header["Session-Id"]).c_str());

        Request request(std::string(socket->header["URI"]), sid, socket->body);

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
            if(socket != nullptr && socket->GetRequest(true))
            {
                std::lock_guard<std::recursive_mutex> lock(kernelLock); // Lock the mutex to ensure thread safety
                if(socket->header.contains_non_null("Method") && std::string(socket->header["Method"]) == "GET")
                {
                    HandleHTTPRequest();
                }
                else if(socket->header.contains_non_null("Method") && std::string(socket->header["Method"]) == "PUT") // JSON Data
                {
                    HandleHTTPRequest();
                }
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
    if(thread_pool)
    {
        delete thread_pool;
        thread_pool = nullptr;
    }
}

}; // namespace ikaros
