// Ikaros 3.0

#include "ikaros.h"

using namespace ikaros;
using namespace std::chrono;
using namespace std::literals;

bool global_terminate = false;

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
            throw exception("Unkown parameter type: "+type_string+".");

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


    parameter::operator std::string()
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


    parameter::operator double()
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
            throw exception("Type conversion error. Parameter does not have a type. Bind?");
    }


    int
    parameter::as_int()
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
    parameter::as_int_string()
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
    parameter::as_string()
    {
        return std::string(*this);
    }



    bool 
    parameter::empty()
    {
        return (*this).as_string().empty();
    }



    void 
        parameter::print(std::string name)
    {
        if(!name.empty())
            std::cout << name << " = ";
        if(type == no_type)
            std::cout <<"not initialized" << std::endl;
        else
            std::cout << std::string(*this) << std::endl;
    }


    void 
    parameter::info()
    {
        std::cout << "name: " << info_["name"] << std::endl;
        std::cout << "type: " << type << std::endl;
        std::cout << "default: " << info_["default"] << std::endl;
        std::cout << "has_options: " << has_options << std::endl;
        std::cout << "options: " << info_["options"] << std::endl;
        std::cout << "value: " << std::string(*this) << std::endl;
    }

    std::string 
    parameter::json()
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
    Component::print()
    {
        std::cout << "Component: " << info_["name"]  << '\n';
    }

        void 
        Component::info()
        {
            std::cout << "Component: " << info_["name"]  << '\n';
            std::cout << "Path: " << path_  << '\n';
            std::cout << "Path: " << info_  << '\n';
        }

    bool 
    Component::BindParameter(parameter & p,  std::string & name) // Handle parameter sharing
    {
        std::string bind_to = GetValue(name+".bind");
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

            std::string value = LookupKey(name);
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
                SetParameter(name, std::to_string(EvaluateNumericalExpression(value))); //FIXME: HANDLE DEFAULTVALUES ALSO
                return true;
            }

            // Lookup normal value in current component-context

            value = GetValue(name);
            if(value.empty())  // ****************** this does not work for string that are allowed to be empty // FIXME: USE EXCEPTIONS *****
            {
                SetParameter(name, p.info_["default"]);
                return true;
            }
                
            SetParameter(name, value);
            return true;
        }
        catch(exception & e)
        {
            Notify(msg_fatal_error, e.message());
        }
        catch(std::exception & e)
        {
            Notify(msg_fatal_error, "ERROR: Could not resolve parameter \""s +name + "\" .", name);  
        }
        return false;
    }



    bool 
    Component::KeyExists(const std::string & key)
    {        
        if(info_.contains(key))
            return true;
        if(parent_)
            return parent_->KeyExists(key);
        else
            return false;
    }



    std::string 
    Component::LookupKey(const std::string & key)
    {        
        if(info_.contains(key))
            return info_[key];
        if(parent_)
            return parent_->LookupKey(key);
        else
            return ""; // throw exception("Name not found"); // throw not_found_exception instead
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
// sensitive to variables and indirection
// does local substitution of vaiables unlike GetValue() / FIXME: is this correct?
//
 
    Component * 
    Component::GetComponent(const std::string & s) 
    {
        std::string path = SubstituteVariables(s);
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


    std::string 
    Component::GetValue(const std::string & path) 
    {     
        if(path.empty())
            return ""; // throw exception("Name not found"); // throw not_found_exception instead

        if(path[0]=='@')
        {
            if(path.find('.') == std::string::npos)
                return GetValue(path.substr(1));
            else
                return GetValue(exchange_before_dot(path, GetValue( before_dot(path).substr(1))));
        }

        if(path[0]=='.')
        {
            auto c = kernel().components.begin()->second;
            return kernel().components.begin()->second->GetValue(path.substr(1)); // Absolute path
        }

        size_t pos = path.find('.');
        if(pos != std::string::npos)
        {
            std::string head = path.substr(0, pos);
            std::string tail = path.substr(pos + 1);

            if(head[0]=='@')
                head = LookupKey(head.substr(1));

            std::string local_path = path_+'.'+head;
            if(kernel().components.count(local_path))
                return kernel().components[local_path]->GetValue(tail);
            else if(std::string(parent_->info_["name"]) == head)
                return parent_->GetValue(tail);
            else
                return ""; // throw exception("Name not found"); // throw not_found_exception instead 
        }

        std::string value = LookupKey(path);

        if(value.empty())
        {
            // Check if we have a resolved paramater for this value and use that in that case
            if(kernel().parameters.count(path_+'.'+path) && kernel().parameters[path_+'.'+path].resolved)
            return kernel().parameters[path_+'.'+path];
        }

        if(value.find('@') != std::string::npos && value.find('.') != std::string::npos) // A new indirect 'path' - start over
            return GetValue(value);
        else if(value.find('@') != std::string::npos) // A new indirect 'key' - start over
            return GetValue(value.substr(1));
        else
            return value;
    }



    int 
    Component::GetIntValue(const std::string & name, int d)
    {
        std::string value = GetValue(name);
        if(value.empty())
            return d;
        return std::stoi(value);
    }


    std::string 
    Component::GetBind(const std::string & name)
    {
        if(info_.contains(name))
            return ""; // Value set in attribute - do not bind
        if(info_.contains(name+".bind")) 
            return info_[name+".bind"];
        if(parent_)
            return parent_->GetBind(name);
        return "";
    }



    std::string 
    Component::SubstituteVariables(const std::string & s)
    {
        std::string var; 
        std::string sep;
        for(auto c : split(s, "."))
        {
            if(c[0] == '@')
                var += sep + GetValue(c.substr(1));
            else
                var += sep + c;
            sep = ".";
        }
        return var;
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
    Component::Evaluate(const std::string & s, bool is_string)
    {
        if(s.empty())
            return "";

    if(!expression::is_expression(s) || is_string)
    {
        if(s[0]=='@') // Handle indirection (unless expression)
        {
            if(s.find('.')==std::string::npos)
                return GetValue(s.substr(1));
            
            std::string component_path = peek_rhead(s.substr(1), ".");
            std::string variable_name = SubstituteVariables(peek_rtail(s, "."));
            return GetComponent(component_path)->GetValue(variable_name);
        }
        else
            return s;
    }

        // Handle mathematical expression

        if(!expression::is_expression(s) || is_string)
            return s;

         expression e = expression(s);
            std::map<std::string, std::string> vars;
            for(auto v : e.variables())
            {
                std::string value = Evaluate(v);
                if(value.empty())
                    throw exception("Variable \""+v+"\" not defined.", path_);
                vars[v] = value;
        }
        return std::to_string(expression(s).evaluate(vars));
    }



    std::string
    Component::EvaluateVariableOrFunction(const std::string & s)
    {
        std::string ss = s;

        // Check functions

        if(ends_with(s, ".size_x"))
            return std::to_string(GetBuffer(rhead(ss,".")).size_x()); // FIXME: Add nondestructive rhead with string instead of string &

        if(ends_with(s, ".size_y"))
            return std::to_string(GetBuffer(rhead(ss,".")).size_y());

        if(ends_with(s, ".size_z"))
            return std::to_string(GetBuffer(rhead(ss,".")).size_z());

        if(ends_with(s, ".rows"))
            return std::to_string(GetBuffer(rhead(ss,".")).rows());

        if(ends_with(s, ".cols"))
            return std::to_string(GetBuffer(rhead(ss,".")).cols());
            
        // Get or evaluate variables

        parameter p;
        if(LookupParameter(p, s.substr(1))) // CHECK @ sign
            return p;
        else
            return Evaluate(s); // FIXME: Should probably not use full evaluation
    }


    double 
    Component::EvaluateNumericalExpression(std::string & s)
    {
        expression e = expression(s);
        std::map<std::string, std::string> vars;
        for(auto v : e.variables())
            vars[v] = EvaluateVariableOrFunction(v);
        return expression(s).evaluate(vars);
    }


    std::vector<int> 
    Component::EvaluateSizeList(std::string & s) // return list of size from size list in string
    {
        std::vector<int> shape;
        for(std::string e : split(s, ","))
        {
            if(ends_with(e, ".size")) // special case: use shape function on input and push each dimension on list
            {
                auto & x = rsplit(e, ".", 1);
                matrix m;
                Bind(m, x.at(0));
                for(auto d : m.shape())
                    shape.push_back(d);
            }
            else
            {
                int d = EvaluateNumericalExpression(e);
                if(d>0)
                    shape.push_back(d);
                // else
                //     throw std::invalid_argument("Value of "+e+" is non-positive or not found."); // Does not work since function can be called multiple times duing SetSizes
            }
        }
        return shape;
    }

    
    bool 
    Component::EvaluateBool(std::string v)
    {
        return false;
    }


    std::string 
    Component::EvaluateString(std::string v)
    {
        return "";
    }



    std::string 
    Component::EvaluateMatrix(std::string v)
    {
        return "";
    }



    int 
    Component::EvaluateOptions(std::string v, std::vector<std::string> & options)
    {
        return 0;
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
        //log_param["options"] = "inherit,quiet,exception,end_of_file,terminate,fatal_error,warning,print,debug,trace";
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
        int log_level = GetIntValue("log_level", 0);
        if(kernel().parameters.count(path_+".log_level")==1)
            log_level = kernel().parameters.at(path_+".log_level").as_int();

        if(msg <= log_level)
            return kernel().Notify(msg, message, path);
        return true;
    }


    tick_count Module::GetTick()        { return kernel().GetTick(); }
    double Module::GetTickDuration()    { return kernel().GetTickDuration(); } // Time for each tick in seconds (s)
    double Module::GetTime()            { return kernel().GetTime(); }
    double Module::GetRealTime()        { return kernel().GetRealTime(); }
    double Module::GetNominalTime()     { return kernel().GetNominalTime(); }
    double Module::GetTimeOfDay()       { return kernel().GetTimeOfDay(); }
    double Module::GetLag()             { return kernel().GetLag(); }


    Module::Module()
    {

    }

    INSTALL_CLASS(Module)

// The following lines will create the kernel the first time it is accessed by one of the components

    Kernel& kernel()
    {
        static Kernel * kernelInstance = new Kernel();
        return *kernelInstance;
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

                std::cout << flattened_input_size << std::endl;
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
            std::cout << "Simple connection" << std::endl;

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
            throw setup_error(u8"Output \""+std::string(d["name"])+"\"+in group \""+path_+"\" can not have size attribute.", path_);

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
            throw setup_error("Component \""+info_["name"].as_string()+"\" has required input \""+d["name"].as_string()+"\" that is not connected.", path_);
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
            else
                throw setup_error("Output \""+std::string(d.at("name")) +"\" must have a value for \"size\".", path_);

            // FIX: special indirection
            while(!size.empty() && size[0]=='@' && size.find(',')==std::string::npos)
                size = GetValue(size.substr(1));
            
            if(size.empty())
                throw setup_error("Output \""+std::string(d.at("name")) +"\" must have a value for \"size\".", path_);
            std::vector<int> shape = EvaluateSizeList(size);
            matrix o;
            Bind(o, d.at("name"));
            o.realloc(shape);
            return 0;
        }
        catch(const std::invalid_argument & e)
        {
            //Notify(msg_fatal_error, e.what());
            throw setup_error("Size expression for output \""+std::string(d.at("name")) +"\" is invalid. "+e.what(), path_);
        }
        catch(...)
        {
            throw setup_error("Size expression for output \""+std::string(d.at("name")) +"\" is invalid.", path_);
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
            parameter p;
            Bind(p, d["name"]);
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
    Connection::Print()
    {
        std::cout << "\t" << source <<  delay_range_.curly() <<  std::string(source_range) << " => " << target  << std::string(target_range);
        if(!alias_.empty())
            std::cout << " \"" << alias_ << "\"";
        std::cout << '\n'; 
    }


std::string
Connection::Info()
{
    std::string s = source + delay_range_.curly() +  std::string(source_range) + " => " + target  + std::string(target_range);
        if(!alias_.empty())
            s+=  " \"" + alias_ + "\"";
    return s;
}

// Class

    Class::Class(std::string n, std::string p) : module_creator(nullptr), name(n), path(p),info_(p)
    {
    }

    Class::Class(std::string n, ModuleCreator mc) : module_creator(mc), name(n)
    {
    }


    void 
    Class::Print()
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
        tick_is_running = false;
        tick_time_usage = 0;
        tick_duration = 1; // default value
        actual_tick_duration = tick_duration;
        idle_time = 0;
        stop_after = -1;
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
    }


    void
    Kernel::Tick()
    {

        tick_is_running = true;
        tick++;

        RunTasks();

        save_matrix_states();
        RotateBuffers();
        Propagate();

        CalculateCPUUsage();

        tick_is_running = false; 
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
        return (stop_after!= -1 &&  tick >= stop_after) || global_terminate;
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
                classes[name].info_ = dictionary(p.path());
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
                    throw setup_error("Parameter \""+p.first+"\" could not be resolved.", p.first);
            throw setup_error("All parameters could not be resolved.");
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
            //Notify(msg_fatal_error, e.message()); // FIXME: Remove
            throw setup_error("Could not calculate input and output sizes. "+e.message(), e.path());
        }

        catch(setup_error & e)
        {
            Notify(msg_fatal_error, e.message()); // FIXME: Remove
            throw setup_error("Could not calculate input and output sizes. "+e.message(), e.path());
        }

        catch(...)
        {
            throw setup_error("Could not calculate input and output sizes.");
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
        tick_is_running(false),
        tick_time_usage(0),
        actual_tick_duration(0), // FIME: Use desired tick duration here
        idle_time(0),
        stop_after(-1),
        tick_duration(1),
        shutdown(false),
        session_id(new_session_id()),
        needs_reload(true)
    {
        cpu_cores = std::thread::hardware_concurrency();
        thread_pool = new ThreadPool(cpu_cores > 1 ? cpu_cores-1 : 1); // FIXME: optionally use ikg parameters
        //thread_pool = new ThreadPool(1); // FIXME: optionally use ikg parameters

    }

    // Functions for creating the network

    void 
    Kernel::AddInput(std::string name, dictionary parameters) // FIXME: use name as argument insteas of parameters
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
            throw exception("Parameter \""+name+"\" could not be set because it doees not exist.");

        try
        {
            parameters[name] = value;
            parameters[name].info_["value"] = value;
        }
        catch(...)
        {
            throw exception("Parameter \""+name+"\" could not be set. Check that parameter exist and that the data type and value is correct.");
        }
    }


    void 
    Kernel::AddGroup(dictionary info, std::string path)
    {
        current_component_info = info;
        current_component_path = path;

        if(components.count(current_component_path)> 0)
            throw exception("Module or group named \""+current_component_path+"\" already exists.", path);

        components[current_component_path] = new Group(); // Implicit argument passing as for components
    }


    void 
    Kernel::AddModule(dictionary info, std::string path)
    {
        current_component_info = info;
        current_component_path = path+"."+std::string(info["name"]);

        if(components.count(current_component_path)> 0)
            throw exception("Module or group with this name already exists. \""+std::string(info["name"])+"\".", path);

        std::string classname = info["class"];

        if(!classname.empty() && classname[0] == '@')
        {
            Component * c = components.at(path);
            classname = c->GetValue(classname.substr(1));
            info["class"] = classname;
        }

        if(!classes.count(classname))
            throw exception("Class \""+classname+"\" does not exist.", path);

if(classes[classname].path.empty())
        throw setup_error("Class file \""+classname+".ikc\" could not be found.", path);

         info.merge(dictionary(classes[classname].path));  // merge with class data structure

        if(classes[classname].module_creator == nullptr)
        {
            if(info.is_not_set("no_code"))
             std::cout << "Class \""<< classname << "\" has no installed code. Creating group." << std::endl; // throw exception("Class \""+classname+"\" has no installed code. Check that it is included in CMakeLists.txt."); // TODO: Check that this works for classes that are allowed to have no code
            info["_tag"]="module";
            BuildGroup(info, path); // FIXME: This is probably not working correctly
        }
        else
            components[current_component_path] = classes[classname].module_creator();
    }


    void 
    Kernel::AddConnection(dictionary info, std::string path)
    {
         std::string souce = path+"."+std::string(info["source"]);   // FIXME: Look for global paths later - string conversion should not be necessary
         std::string target = path+"."+std::string(info["target"]);

         std::string delay_range = info.contains("delay") ? info["delay"] : "";// FIXME: return "" if name not in dict - or use containts *********
         std::string alias = info.contains("alias") ? info["alias"] : "";// FIXME: return "" if name not in dict - or use containts *********

        if(delay_range.empty() || delay_range=="null")  // FIXME: return "" if name not in dict - or use contains *********
            delay_range = "[1]";
        else if(delay_range[0] != '[')
            delay_range = "["+delay_range+"]";
        range r(delay_range);
        connections.push_back(Connection(souce, target, r, alias));
    }



    void Kernel::LoadExternalGroup(dictionary d)
    {
        std::string path = d["external"];
        dictionary external(path);
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
                throw setup_error("Main element is '"+std::string(d["_tag"])+"' but must be 'group' for ikg-file.");

            if(!d.contains("name"))
                throw setup_error("Groups must have a name.", path);

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
            throw setup_error("Build group failed for "+path+": "+e.message());
        }
        catch(const std::exception& e)
        {
            throw setup_error("Build group failed for "+path+": "+std::string(e.what()), path);
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
    Kernel::SetCommandLineParameters(dictionary & d) // Add command line arguments - will override XML - probably not correct ******************
    {
    
        for(auto & x : options_.d)
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
                    dictionary d = dictionary(options_.full_path());
                    SetCommandLineParameters(d);
                    BuildGroup(d);
                    info_ = d;
                    session_id = new_session_id();
                    SetUp();
                    Notify(msg_print, u8"Loaded "s+options_.full_path());

                    CalculateCheckSum();
                    //ListBuffers();  // FIXME: remove
                    //ListConnections();
                    needs_reload = false;
                }
                catch(const exception& e)
                {
                    throw load_failed(u8"Load file failed for "s+options_.full_path()+". "+e.message(), e.path());
                }
                catch(const std::exception& e)
                {
                    throw load_failed(u8"Load file failed for "s+options_.full_path()+". "+e.what());
                }

        }
        catch(const exception& e)
        {
            Notify(msg_warning, e.what(), e.path()); // Do not exit if not in batch mode // FIXME: Check this
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
                delete socket;
                exit(1);
            }
        }
    }


     dictionary 
     Kernel::GetModuleInstantiationInfo()
     {
        dictionary d;
        d["Constant"] = "1";
        d["Print"] = "2";
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
        Socket socket;
        char b[2048];
        socket.Get("www.ikaros-project.org", 80, "GET /start3/ HTTP/1.1\r\nHost: www.ikaros-project.org\r\nConnection: close\r\n\r\n", b, 1024);
        socket.Close();
    }



    void
    Kernel::LogStop()
    {
        Socket socket;
        char b[2048];
        socket.Get("www.ikaros-project.org", 80, "GET /stop3/ HTTP/1.1\r\nHost: www.ikaros-project.org\r\nConnection: close\r\n\r\n", b, 1024);
        socket.Close();
    }

    void 
    Kernel::Propagate()
    {

         for(auto & c : connections)
            if(c.delay_range_.is_delay_0())
                continue;
            else
                c.Tick();
    }



    void
    Kernel::InitSocket(long port)
    {
        try
        {
            socket =  new ServerSocket(port);
        }
        catch (const exception& e)
        {
            throw fatal_error("Ikaros is unable to start a webserver on port "+std::to_string(port)+". Make sure no other ikaros process is running and try again.");
        }

        httpThread = new std::thread(Kernel::StartHTTPThread, this);
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
            throw setup_error("Network has zero-delay loops");
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
        std::vector<std::unique_ptr<TaskSequence>> sequences;
    
        try {
            // Create and submit tasks
            for (auto &task_sequence : tasks) {
                auto ts = std::make_unique<TaskSequence>(task_sequence);
                thread_pool->submit(ts.get());
                sequences.push_back(std::move(ts));
            }
    
            // Wait for completion using the blocking wait method
            for (auto &ts : sequences) {
                ts->waitForCompletion();
            }
        } catch (const std::exception &e) {
            Notify(msg_fatal_error, "Error during task execution: " + std::string(e.what()));
        }
    }


    /*
    void
    Kernel::RunTasks()
    {
        std::vector<TaskSequence *> sequences;

        for(auto & task_sequence : tasks)
        {
            auto ts = new TaskSequence(task_sequence);
            sequences.push_back(ts);
            thread_pool->submit(ts);
        }
        // WAIT FOR COMPLETION

        bool working = true;
        while(working)
        {
            working =false;
            for(auto ts: sequences)
                if(!ts->isCompleted())
                    working = true;     
        }

        for(auto ts: sequences)
            delete ts;
    }
*/


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

            LogStart();
            //PrintLog();
        }
        catch(exception & e)
        {
            throw setup_error("SetUp Failed. "+e.message(), e.path());
        }
        catch(std::exception & e)
        {
            throw setup_error("SetUp Failed. "+std::string(+e.what()));
        }
    }


    void
    Kernel::Run() // START-UP + RUN MAIN LOOP => Two functions?? *************
    {
        // Main loop
        while(run_mode > run_mode_quit && !global_terminate)  // Not quit
        {
            while (!Terminate() && run_mode > run_mode_quit)
            {
                if(run_mode == run_mode_realtime)
                    lag = timer.WaitUntil(double(tick+1)*tick_duration);
                else if(run_mode == run_mode_play)
                {
                    timer.SetTime(double(tick+1)*tick_duration); // Fake time increase // FIXME: remove sleep in batch mode
                    Sleep(0.01);
                }
                else
                    Sleep(0.01); // Wait 10 ms to avoid wasting cycles if there are no requests

                // Run_mode may have changed during the delay - needs to be checked again

                if(run_mode == run_mode_realtime || run_mode == run_mode_play) 
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
                    idle_time = tick_duration - tick_time_usage;
                }    
            }
            Stop();
            Sleep(0.1);
        }
    }

        bool
        Kernel::Notify(int msg, std::string message, std::string path)
        {
            static std::mutex mtx;
            std::lock_guard<std::mutex> lock(mtx); // Lock the mutex

            log.push_back(Message(msg, message, path));

            std::cout << "ikaros: " << message;
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
    Component::json()
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
        run_mode = std::min(run_mode_stop, run_mode);
        tick = -1;
        timer.Pause();
        timer.SetPauseTime(0);
        LogStop();
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
        timer.Continue(); 
        run_mode = run_mode_realtime;
    }



    void
    Kernel::Play()
    {
        if(needs_reload)
            LoadFile();
  
        run_mode = run_mode_play;
        timer.Continue();
    }


    void
    Kernel::DoSendDataHeader()
    {
        Dictionary header;
        header.Set("Session-Id", std::to_string(session_id).c_str());
        header.Set("Package-Type", "data");
        header.Set("Content-Type", "application/json");
        header.Set("Cache-Control", "no-cache");
        header.Set("Cache-Control", "no-store");
        header.Set("Pragma", "no-cache");
        header.Set("Expires", "0");
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

            socket->Send("\t\"state\": %d,\n", run_mode);
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
            socket->Send(sep +line.json());
            sep = ",";
        }
        socket->Send("]");
        log.clear();
    }


    void
    Kernel::DoSendData(Request & request)
    {    
        sending_ui_data = true; // must be set while main thread is still running


        DoSendDataHeader();

        socket->Send("{\n");

        DoSendDataStatus();

        socket->Send("\t\"data\":\n\t{\n");

        std::string data = request.parameters["data"];  // FIXME: Check that it exists ******** or return ""
        std::string root;
            if(request.parameters.contains("root"))
                root = std::string(request.parameters["root"]);

        std::string sep = "";
        bool sent = false;

        while(!data.empty()) // FIXME: CHeck the we do not run out of time here and break if next tick is about to start.
        {
            std::string source = head(data, ",");
            std::string key = source;


            if(source.find("@") != std::string::npos && components.count(root) > 0)
            {
                Component * c = components[root]; // FIXME: Handle exceptions
                source = c->GetValue(source);
            }

            std::string format = rtail(source, ":");
            std::string source_with_root = root +"."+source;

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

            else
            {
                // sent = socket->Send(sep + "\t\t\"" + key + "\": \""+source+"\""); // ERROR: Does not exist, do not send
            }

            if(sent)
                sep = ",\n";
        }

        socket->Send("\n\t}");
        DoSendLog(request);
        socket->Send(",\n\t\"has_data\": "+std::to_string(!tick_is_running)+"\n"); // new tick has started during sending; there may be data but it cannot be trusted // FIXME: Never happens
        socket->Send("}\n");

        sending_ui_data = false;
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
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
                Notify(msg_warning, "File \""+file+"\" could not be loaded"); // FIXME: better error message - alert? HTTP reply with error code
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

        if(socket->SendFile(file, webui_dir))   // Now look in WebUI directory
            return;
        /*
        if(socket->SendFile(file, std::string(webui_dir)+"Images/"))   // Now look in WebUI/Images directory
            return;
        
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
        Dictionary rtheader;
        rtheader.Set("Session-Id", std::to_string(session_id).c_str());
        rtheader.Set("Package-Type", "network");
        rtheader.Set("Content-Type", "application/json");
        rtheader.Set("Content-Length", int(s.size()));
        socket->SendHTTPHeader(&rtheader);
        socket->SendData(s.c_str(), int(s.size()));
    }


    void
    Kernel::DoPause(Request & request)
    {
        Notify(msg_print, "pause");
        Pause();
        DoSendData(request);
    }


    void
    Kernel::DoStep(Request & request)
    {
        Notify(msg_print, "step");
        Pause();
        run_mode = run_mode_pause; // FIXME: Probably not necessary
        Tick();
        timer.SetPauseTime(GetTime()+tick_duration);
        DoSendData(request);
    }



    void
    Kernel::DoRealtime(Request & request)
    {
        Notify(msg_print, "realtime");
        Realtime();
        DoSendData(request);
    }


    void
    Kernel::DoPlay(Request & request)
    {
        Notify(msg_print, "play");
        Play();
        DoSendData(request);
    }



    void
    Kernel::DoData(Request & request)
    {
         if(!buffers.count(request.component_path))
            {
                Dictionary header;
                header.Set("Content-Type", "text/plain");
                header.Set("Cache-Control", "no-cache");
                header.Set("Cache-Control", "no-store");
                header.Set("Pragma", "no-cache");
                socket->SendHTTPHeader(&header);
        
                socket->Send("Buffer \""+request.component_path+"\" can not be found");
                return;
            }

        if(buffers[request.component_path].rank() > 2)
        {
            Dictionary header;
            header.Set("Content-Type", "text/plain");
            header.Set("Cache-Control", "no-cache");
            header.Set("Cache-Control", "no-store");
            header.Set("Pragma", "no-cache");
            socket->SendHTTPHeader(&header);

            socket->Send("Rank of matrix != 2. Cannot be displayed as CSV");
            return;
        }

        Dictionary header;
        header.Set("Content-Type", "text/plain");
        header.Set("Cache-Control", "no-cache");
        header.Set("Cache-Control", "no-store");
        header.Set("Pragma", "no-cache");
        socket->SendHTTPHeader(&header);

        socket->Send(buffers[request.component_path].csv());
    }



    void
    Kernel::DoCommand(Request & request)
    {
        if(!components.count(request.component_path))
            {
                Notify(msg_warning, "Component '"+request.component_path+"' could not be found.");
                DoSendData(request);
                return;
            }

        if(!request.body.empty()) // FIXME: Move to request and check content-type first
        {
            request.parameters = parse_json(request.body);
        }

        if(!request.parameters.contains("command"))
        {
                Notify(msg_warning, "No command specified for  '"+request.component_path+"'.");
                DoSendData(request);
                return;
        }

        components.at(request.component_path)->Command(request.parameters["command"], request.parameters);
        DoSendData(request);
    }



    void
    Kernel::DoControl(Request & request)
    {
        try
        {
            std::string root;
         
            if(request.parameters.contains("root"))
                root= std::string(request.parameters["root"]);

            if(!parameters.count(request.component_path))
            {
                Notify(msg_warning, "Parameter '"+request.component_path+"' could not be found.");
                DoSendData(request);
                return;
            }

            parameter & p = parameters.at(request.component_path);
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
                    (*p.matrix_value)(x,y)= value;
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
        Dictionary header;
        header.Set("Content-Type", "text/json");
        header.Set("Cache-Control", "no-cache");
        header.Set("Cache-Control", "no-store");
        header.Set("Pragma", "no-cache");
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
        Dictionary header;
        header.Set("Content-Type", "text/json");
        header.Set("Cache-Control", "no-cache");
        header.Set("Cache-Control", "no-store");
        header.Set("Pragma", "no-cache");
        socket->SendHTTPHeader(&header);
        socket->Send("{\n");
        std::string s = "";
        for(auto & c: classes)
        {
            socket->Send(s);
            socket->Send("\""+c.first+"\": ");
            socket->Send(c.second.info_.json());
            s = ",\n\t";
        }
        socket->Send("\n}\n");
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

        Dictionary header;
        header.Set("Content-Type", "text/json");
        header.Set("Cache-Control", "no-cache");
        header.Set("Cache-Control", "no-store");
        header.Set("Pragma", "no-cache");
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
    Dictionary header;
    header.Set("Content-Type", "text/plain");
    socket->SendHTTPHeader(&header);
    socket->Send("ERROR\n");
    }


    void
    Kernel::HandleHTTPRequest()
    {
        long sid = 0;
        const char * sids = socket->header.Get("Session-Id");
        if(sids)
            sid = atol(sids);

        Request request(socket->header.Get("URI"), sid, socket->body);

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


                if(equal_strings(socket->header.Get("Method"), "GET"))
                {
                     handling_request = true;
                    HandleHTTPRequest();
                        handling_request = false;
                    }
                    else if(equal_strings(socket->header.Get("Method"), "PUT")) // JSON Data
                    {
                        handling_request = true;
                        HandleHTTPRequest();
                        handling_request = false;
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

}; // namespace ikaros



Kernel::~Kernel()
{
    if(socket)
    {
        shutdown=true;
        Sleep(0.1);
        delete socket;
        delete thread_pool;
        delete httpThread;
    }
}

