// Ikaros 3.0

#include "ikaros.h"
#include "component_runtime.h"

#include <ctime>
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
        constexpr auto cpu_usage_sample_interval = 100ms;

        int default_thread_pool_size(unsigned int cpu_cores)
        {
            return cpu_cores > 1 ? static_cast<int>(cpu_cores) - 1 : 1;
        }

    }

    long
    Kernel::NewSessionID()
    {
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }




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

        async_components.clear();
        components.clear();

        post_task_connection_spans.clear();
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
        session_id = NewSessionID();
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


    Kernel::Kernel():
        session_id(NewSessionID()),
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


bool
Kernel::CalculateCPUUsage() // Fraction of total CPU capacity
{
    const auto sample_time = std::chrono::steady_clock::now();
    if(cpu_usage_initialized &&
       sample_time - cpu_usage_sample_time < cpu_usage_sample_interval)
        return false;

    struct rusage usage{};
    if(getrusage(RUSAGE_SELF, &usage) != 0)
    {
        cpu_usage = 0;
        cpu_usage_initialized = false;
        return false;
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
        return true;
    }

    const double wall_time_delta = std::chrono::duration<double>(sample_time - cpu_usage_sample_time).count();
    cpu_usage = CPUUsageFraction(cpu - last_cpu, wall_time_delta, cpu_cores);
    last_cpu = cpu;
    cpu_usage_sample_time = sample_time;
    return true;
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
