// Ikaros 3.0

#include "ikaros.h"
#include "component_runtime.h"
#include "compute_engine.h"
#include "kernel_parsing.h"

#include <iostream>
#include <limits>
#include <utility>

namespace ikaros
{
    namespace
    {
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

        void ensure_component_collections(dictionary & info)
        {
            info.ensure_list("inputs");
            info.ensure_list("outputs");
            info.ensure_list("states");
            info.ensure_list("parameters");
            info.ensure_list("groups");
            info.ensure_list("modules");
        }
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
                    double value = kernel_detail::parse_parameter_number(change.value, "matrix parameter cell");
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
    Component::StartupStepString(int step)
    {
        return step == std::numeric_limits<int>::max() ? "unknown" : std::to_string(step);
    }


    std::string
    Component::StartupFirstRealInputStepString() const
    {
        return StartupStepString(startup_first_real_input_step);
    }


    std::string
    Component::StartupAllRealInputsStepString() const
    {
        return StartupStepString(startup_all_real_inputs_step);
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
            return kernel_detail::parse_strict_int(value);
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

        info_["parameters"].push_back(LogLevelParameterInfo().copy());
    }


    dictionary
    Component::LogLevelParameterInfo()
    {
        dictionary parameter;
        parameter["_tag"] = "parameter";
        parameter["name"] = "log_level";
        parameter["type"] = "number";
        parameter["control"] = "menu";
        parameter["options"] = "inherit,quiet,exception,end_of_file,terminate,fatal_error,warning,print,debug,trace";
        parameter["default"] = 0;
        return parameter;
    }


    dictionary
    Component::ModuleStartParameterInfo()
    {
        dictionary parameter;
        parameter["_tag"] = "parameter";
        parameter["name"] = "module_start";
        parameter["type"] = "number";
        parameter["control"] = "menu";
        parameter["options"] = "at_tick,first_data,all_data";
        parameter["default"] = 0;
        return parameter;
    }


    dictionary
    Component::StartTickParameterInfo()
    {
        dictionary parameter;
        parameter["_tag"] = "parameter";
        parameter["name"] = "start_tick";
        parameter["type"] = "number";
        parameter["default"] = 0;
        return parameter;
    }


    dictionary
    Component::AsyncParameterInfo()
    {
        dictionary parameter;
        parameter["_tag"] = "parameter";
        parameter["name"] = "async";
        parameter["type"] = "bool";
        parameter["default"] = "no";
        parameter["description"] = "Run this module asynchronously.";
        return parameter;
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
                    info_["parameters"].push_back(StartTickParameterInfo().copy());
                return;
            }

        info_["parameters"].push_back(ModuleStartParameterInfo().copy());
        info_["parameters"].push_back(StartTickParameterInfo().copy());
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
                scalar->second.Reset();
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


    std::string
    Component::json() const
    {
        return info_.json();
    }


    std::string
    Component::xml()
    {
        return info_.model_xml("group");
    }


}
