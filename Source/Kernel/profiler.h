#pragma once

#include <chrono>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <time.h>

#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/thread_act.h>
#else
#include <sys/resource.h>
#include <sys/time.h>
#endif

#include "statistics.h"



class Profiler 
{
public:
    using clock = std::chrono::steady_clock;

    Profiler() = default;

    void 
    reset() noexcept
    {
        running_ = false;
        wall_duration_ = clock::duration::zero();
        cpu_duration_sec_ = 0.0;
        wall_start_ = clock::time_point{};
        cpu_start_sec_ = 0.0;

        wall_time_.reset();
        cpu_time_.reset();
    }

    void 
    begin() 
    {
        running_ = true;
        wall_start_ = clock::now();
        cpu_start_sec_ = thread_cpu_seconds();
    }

    void 
    end() 
    {
        if (!running_) return;
        auto wall_end = clock::now();
        double cpu_end_sec = thread_cpu_seconds();

        wall_duration_ = wall_end - wall_start_;
        cpu_duration_sec_ = cpu_end_sec - cpu_start_sec_;

        wall_time_.push(std::chrono::duration<double>(wall_duration_).count());
        cpu_time_.push(cpu_duration_sec_);
        running_ = false;
    }

    // Results (in seconds)
    [[nodiscard]]
    double 
    wall_seconds() const 
    {
        return std::chrono::duration<double>(wall_duration_).count();
    }

    [[nodiscard]]
    double 
    cpu_seconds() const 
    {
        return cpu_duration_sec_;
    }

    [[nodiscard]]
    bool 
    running() const noexcept
    { 
            return running_; 
    }


    void print(const std::string & msg="") const
    {
        if (!msg.empty())
            std::cout << msg << "\t";

        if(cpu_time_.no_data())
        {
            std::cout << "-\n";
            return;
        }
    
        std::cout  << std::fixed << std::setprecision(4) << 1000*cpu_time_.mean()  << " ms\n";

       // std::cout << " [" << wall_time_.count() << "] ";
        //std::cout << "Wall: " << wall_seconds() << " s (mean: " << wall_time_.mean() << " s, sd: " << wall_time_.standard_deviation() << " s) ";
       // std::cout << "CPU: " << cpu_seconds() << " s (mean: " << cpu_time_.mean() << " s, sd: " << cpu_time_.standard_deviation() << " s)" << std::endl;
    }

    [[nodiscard]]
    std::string json() const
    {
        std::ostringstream out;
        out << "{";
        out << "\"running\": " << (running_ ? "true" : "false") << ", ";
        out << "\"last_wall_seconds\": " << format_json_number(wall_seconds()) << ", ";
        out << "\"last_cpu_seconds\": " << format_json_number(cpu_seconds()) << ", ";
        out << "\"wall\": " << statistics_json(wall_time_) << ", ";
        out << "\"cpu\": " << statistics_json(cpu_time_);
        out << "}";
        return out.str();
    }



private:
    static double 
    thread_cpu_seconds() 
    {
#if defined(CLOCK_THREAD_CPUTIME_ID)
        timespec ts{};
        if(clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) == 0)
            return double(ts.tv_sec) + 1e-9 * double(ts.tv_nsec);
#endif

#if defined(__APPLE__)
        mach_port_t thread_port = mach_thread_self();
        thread_basic_info_data_t info{};
        mach_msg_type_number_t count = THREAD_BASIC_INFO_COUNT;
        const kern_return_t kr = thread_info(thread_port,
                                             THREAD_BASIC_INFO,
                                             reinterpret_cast<thread_info_t>(&info),
                                             &count);
        mach_port_deallocate(mach_task_self(), thread_port);
        if(kr != KERN_SUCCESS)
            return 0.0;

        double user = double(info.user_time.seconds) + 1e-6 * double(info.user_time.microseconds);
        double sys  = double(info.system_time.seconds) + 1e-6 * double(info.system_time.microseconds);
        return user + sys;
#else
        rusage usage{};
        if(getrusage(RUSAGE_THREAD, &usage) != 0)
            return 0.0;

        double user = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec * 1e-6;
        double sys  = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec * 1e-6;
        return user + sys;
#endif
    }

    static std::string
    format_json_number(double value)
    {
        if(!std::isfinite(value))
            return "null";

        std::ostringstream out;
        out << std::setprecision(17) << value;
        return out.str();
    }

    static std::string
    statistics_json(const statistics & values)
    {
        std::ostringstream out;
        out << "{";
        out << "\"count\": " << values.count() << ", ";
        out << "\"mean\": " << format_json_number(values.mean()) << ", ";
        out << "\"median\": " << format_json_number(values.median()) << ", ";
        out << "\"min\": " << format_json_number(values.min()) << ", ";
        out << "\"max\": " << format_json_number(values.max()) << ", ";
        out << "\"standard_deviation\": " << format_json_number(values.standard_deviation());
        out << "}";
        return out.str();
    }

    bool running_{false};
    clock::time_point wall_start_{};
    clock::duration   wall_duration_{};
    double            cpu_start_sec_{0.0};
    double            cpu_duration_sec_{0.0};

    statistics wall_time_;
    statistics cpu_time_;
};
