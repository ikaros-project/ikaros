#pragma once

#include <chrono>
#include <sys/resource.h>
#include <sys/time.h>

#include "statistics.h"


class Profiler 
{
public:
    using clock = std::chrono::steady_clock;

    Profiler()
    { 
        reset();
    }

    void 
    reset() 
    {
        running_ = false;
        wall_duration_ = clock::duration::zero();
        cpu_duration_sec_ = 0.0;
        wall_start_ = clock::time_point{};
        cpu_start_sec_ = 0.0;
    }

    void 
    start() 
    {
        running_ = true;
        wall_start_ = clock::now();
        cpu_start_sec_ = process_cpu_seconds();
        wall_time_.reset();
        cpu_time_.reset();
    }

    void 
    stop() 
    {
        if (!running_) return;
        auto wall_end = clock::now();
        double cpu_end_sec = process_cpu_seconds();

        wall_duration_ = wall_end - wall_start_;
        cpu_duration_sec_ = cpu_end_sec - cpu_start_sec_;

        wall_time_.push(std::chrono::duration<double>(wall_duration_).count());
        cpu_time_.push(cpu_duration_sec_);
        running_ = false;
    }

    // Results (in seconds)
    double 
    wall_seconds() const 
    {
        return std::chrono::duration<double>(wall_duration_).count();
    }

    double 
    cpu_seconds() const 
    {
        return cpu_duration_sec_;
    }

    bool 
    running() const 
    { 
            return running_; 
    }

private:
    static double 
    process_cpu_seconds() 
    {
        rusage usage{};
        getrusage(RUSAGE_SELF, &usage);
        double user = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec * 1e-6;
        double sys  = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec * 1e-6;
        return user + sys;
    }

    bool running_{false};
    clock::time_point wall_start_{};
    clock::duration   wall_duration_{};
    double            cpu_start_sec_{0.0};
    double            cpu_duration_sec_{0.0};

    statistics wall_time_;
    statistics cpu_time_;
};


