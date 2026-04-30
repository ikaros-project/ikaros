#include "ikaros.h"

using namespace ikaros;

class System: public Module
{
    matrix tick;
    matrix time;
    matrix nominal_time;
    matrix real_time;
    matrix uptime;
    matrix run_state;
    matrix state;
    matrix tick_duration;
    matrix actual_duration;
    matrix tick_time_usage;
    matrix lag;
    matrix ticks_per_s;
    matrix cpu_usage;
    matrix time_usage;
    matrix idle_ratio;
    matrix overload;
    matrix progress;
    matrix stop_after_known;
    matrix cpu_cores;
    matrix module_count;
    matrix class_count;

    void Init()
    {
        Bind(tick, "TICK");
        Bind(time, "TIME");
        Bind(nominal_time, "NOMINAL_TIME");
        Bind(real_time, "REAL_TIME");
        Bind(uptime, "UPTIME");
        Bind(run_state, "RUN_STATE");
        Bind(state, "STATE");
        Bind(tick_duration, "TICK_DURATION");
        Bind(actual_duration, "ACTUAL_DURATION");
        Bind(tick_time_usage, "TICK_TIME_USAGE");
        Bind(lag, "LAG");
        Bind(ticks_per_s, "TICKS_PER_S");
        Bind(cpu_usage, "CPU_USAGE");
        Bind(time_usage, "TIME_USAGE");
        Bind(idle_ratio, "IDLE_RATIO");
        Bind(overload, "OVERLOAD");
        Bind(progress, "PROGRESS");
        Bind(stop_after_known, "STOP_AFTER_KNOWN");
        Bind(cpu_cores, "CPU_CORES");
        Bind(module_count, "MODULE_COUNT");
        Bind(class_count, "CLASS_COUNT");
    }

    void Tick()
    {
        const double duration = GetTickDuration();
        const double actual = GetActualTickDuration();
        const double used = GetTickTimeUsage();
        const double current_time = GetTime();
        const tick_count current_tick = GetTick();
        const tick_count stop_after = GetStopAfter();
        const int mode = GetRunMode();

        tick[0] = static_cast<float>(current_tick);
        time[0] = static_cast<float>(current_time);
        nominal_time[0] = static_cast<float>(GetNominalTime());
        real_time[0] = static_cast<float>(GetRealTime());
        uptime[0] = static_cast<float>(GetUptime());

        run_state[0] = mode == run_mode_stop ? 1.0f : 0.0f;
        run_state[1] = mode == run_mode_pause ? 1.0f : 0.0f;
        run_state[2] = mode == run_mode_play ? 1.0f : 0.0f;
        run_state[3] = mode == run_mode_realtime ? 1.0f : 0.0f;
        state[0] = static_cast<float>(mode);

        tick_duration[0] = static_cast<float>(duration);
        actual_duration[0] = static_cast<float>(actual);
        tick_time_usage[0] = static_cast<float>(used);
        lag[0] = static_cast<float>(GetLag());
        ticks_per_s[0] = current_time > 0.0 ? static_cast<float>(static_cast<double>(current_tick) / current_time) : 0.0f;

        cpu_usage[0] = static_cast<float>(GetCPUUsage());
        time_usage[0] = actual > 0.0 ? static_cast<float>(used / actual) : 0.0f;
        idle_ratio[0] = duration > 0.0 ? static_cast<float>(GetIdleTime() / duration) : 0.0f;
        overload[0] = duration > 0.0 && used > duration ? 1.0f : 0.0f;

        stop_after_known[0] = stop_after >= 0 ? 1.0f : 0.0f;
        progress[0] = stop_after > 0 ? static_cast<float>(static_cast<double>(current_tick) / static_cast<double>(stop_after)) : 0.0f;
        cpu_cores[0] = static_cast<float>(GetCPUCoreCount());
        module_count[0] = static_cast<float>(GetModuleCount());
        class_count[0] = static_cast<float>(GetClassCount());
    }
};

INSTALL_CLASS(System)
