// Ikaros 3.0

#include "ikaros.h"
#include "kernel_parsing.h"
#include "compute_engine.h"
#include "session_logging.h"

#include <cctype>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <sys/resource.h>

using namespace ikaros;
using namespace std::chrono;
using namespace std::literals;

std::atomic<bool> global_terminate(false);

namespace ikaros
{
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

        int default_thread_pool_size(unsigned int cpu_cores)
        {
            return cpu_cores > 1 ? static_cast<int>(cpu_cores) - 1 : 1;
        }

        constexpr double profiling_subscription_timeout_seconds = 3.0;
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


    //

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
