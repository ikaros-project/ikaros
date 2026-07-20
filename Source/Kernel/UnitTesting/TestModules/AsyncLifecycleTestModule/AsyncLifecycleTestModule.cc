#include <chrono>
#include <stdexcept>
#include <thread>

#include "ikaros.h"

using namespace ikaros;

class AsyncLifecycleTestModule : public Module
{
    parameter gain;
    parameter duration;
    parameter reportRuntimeSnapshot;
    parameter reportFailureOnStop;
    int completedTicks;
    matrix input;
    matrix output;

    void Init() override
    {
        Bind(gain, "gain");
        Bind(duration, "duration");
        Bind(reportRuntimeSnapshot, "report_runtime_snapshot");
        Bind(reportFailureOnStop, "report_failure_on_stop");
        Bind(completedTicks, "completed_ticks");
        Bind(input, "X");
        Bind(output, "Y");
    }

    void Tick() override
    {
        const tick_count tick_at_start = GetTick();
        const double time_at_start = GetTime();
        const double nominal_time_at_start = GetNominalTime();
        const double actual_tick_duration_at_start = GetActualTickDuration();
        const double tick_time_usage_at_start = GetTickTimeUsage();
        const double cpu_usage_at_start = GetCPUUsage();
        const double idle_time_at_start = GetIdleTime();

        std::this_thread::sleep_for(std::chrono::duration<double>(duration.as_double()));

        if(reportRuntimeSnapshot.as_bool())
        {
            if(GetTick() != tick_at_start ||
               GetTime() != time_at_start ||
               GetNominalTime() != nominal_time_at_start ||
               GetActualTickDuration() != actual_tick_duration_at_start ||
               GetTickTimeUsage() != tick_time_usage_at_start ||
               GetCPUUsage() != cpu_usage_at_start ||
               GetIdleTime() != idle_time_at_start)
                throw std::runtime_error("Async runtime snapshot changed during Tick().");

            Notify(msg_warning, "ASYNC_RUNTIME_SNAPSHOT_STABLE tick=" + std::to_string(tick_at_start));
        }

        const double result = input.scalar() * gain.as_double();
        output(0) = result;
        ++completedTicks;
        Notify(msg_print, "CPP_ASYNC_LIFECYCLE_OUTPUT " + std::to_string(result) + " gain=" + gain.as_int_string());
    }

    void Command(std::string commandName, dictionary & parameters) override
    {
        if(commandName == "fail_deferred")
            throw std::runtime_error("Requested deferred command failure.");
        Module::Command(commandName, parameters);
    }

    void Stop() override
    {
        if(reportFailureOnStop.as_bool())
            Notify(msg_warning, "ASYNC_FAILURE_STATE running=" + std::to_string(IsAsyncRunning()) +
                                " failed=" + std::to_string(IsAsyncFailed()));
    }
};

INSTALL_CLASS(AsyncLifecycleTestModule)
