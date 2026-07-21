#pragma once

#include <chrono>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include "statistics.h"



class Profiler 
{
public:
    using clock = std::chrono::steady_clock;
    static constexpr std::size_t default_history_limit = 1000;

    explicit Profiler(std::size_t history_limit = default_history_limit)
        : wall_time_(history_limit), cpu_time_(history_limit)
    {
    }

    [[nodiscard]]
    std::size_t
    history_limit() const noexcept
    {
        return wall_time_.sample_limit();
    }

    void 
    reset()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
        wall_duration_ = clock::duration::zero();
        cpu_duration_sec_ = 0.0;
        wall_start_ = clock::time_point{};
        cpu_start_sec_ = 0.0;
        running_thread_ = std::thread::id{};

        wall_time_.reset();
        cpu_time_.reset();
    }

    void 
    begin() 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(running_)
            throw std::logic_error("Profiler::begin() called while a sample is already active");

        const double cpu_start_sec = thread_cpu_seconds();
        running_ = true;
        wall_start_ = clock::now();
        cpu_start_sec_ = cpu_start_sec;
        running_thread_ = std::this_thread::get_id();
    }

    void 
    end() 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!running_)
            throw std::logic_error("Profiler::end() called without an active sample");
        if(running_thread_ != std::this_thread::get_id())
            throw std::logic_error("Profiler::end() must run on the thread that called begin()");

        const auto wall_end = clock::now();
        double cpu_end_sec;
        try
        {
            cpu_end_sec = thread_cpu_seconds();
        }
        catch(...)
        {
            cancel_active_sample();
            throw;
        }

        if(wall_end < wall_start_ || cpu_end_sec < cpu_start_sec_)
        {
            cancel_active_sample();
            throw std::runtime_error("Profiler clock moved backwards during a sample");
        }

        wall_duration_ = wall_end - wall_start_;
        cpu_duration_sec_ = cpu_end_sec - cpu_start_sec_;

        try
        {
            wall_time_.push(std::chrono::duration<double>(wall_duration_).count());
            cpu_time_.push(cpu_duration_sec_);
        }
        catch(...)
        {
            cancel_active_sample();
            throw;
        }
        cancel_active_sample();
    }

    // Results (in seconds)
    [[nodiscard]]
    double 
    wall_seconds() const 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return std::chrono::duration<double>(wall_duration_).count();
    }

    [[nodiscard]]
    double 
    cpu_seconds() const 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return cpu_duration_sec_;
    }

    [[nodiscard]]
    bool 
    running() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return running_;
    }


    void print(const std::string & msg="") const
    {
        const Snapshot data = snapshot();
        std::ostringstream out;

        if (!msg.empty())
            out << msg << "\t";

        if(data.cpu_time.no_data())
        {
            std::cout << out.str() << "-\n";
            return;
        }

        out << std::fixed << std::setprecision(4)
            << 1000 * data.cpu_time.mean() << " ms\n";
        std::cout << out.str();
    }

    [[nodiscard]]
    std::string json() const
    {
        const Snapshot data = snapshot();

        std::ostringstream out;
        out << "{";
        out << "\"running\": " << (data.running ? "true" : "false") << ", ";
        out << "\"last_wall_seconds\": "
            << format_json_number(std::chrono::duration<double>(data.wall_duration).count()) << ", ";
        out << "\"last_cpu_seconds\": " << format_json_number(data.cpu_duration_sec) << ", ";
        out << "\"wall\": " << statistics_json(data.wall_time) << ", ";
        out << "\"cpu\": " << statistics_json(data.cpu_time);
        out << "}";
        return out.str();
    }



private:
    struct Snapshot
    {
        bool running;
        clock::duration wall_duration;
        double cpu_duration_sec;
        statistics wall_time;
        statistics cpu_time;
    };

    static double thread_cpu_seconds();

    void
    cancel_active_sample() noexcept
    {
        running_ = false;
        running_thread_ = std::thread::id{};
    }

    Snapshot
    snapshot() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return {running_, wall_duration_, cpu_duration_sec_, wall_time_, cpu_time_};
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
    std::thread::id    running_thread_{};

    statistics wall_time_;
    statistics cpu_time_;
    mutable std::mutex mutex_;
};
