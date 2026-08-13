// Ikaros 3.0

#pragma once

#include "ikaros.h"

namespace ikaros
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

    inline thread_local const AsyncRuntimeSnapshot * active_async_runtime_snapshot = nullptr;

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

    inline AsyncRuntimeSnapshot
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
}
