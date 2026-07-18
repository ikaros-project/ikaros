#pragma once

#include <chrono>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

#include "statistics.h"



class Profiler 
{
public:
    using clock = std::chrono::steady_clock;

    Profiler() = default;

    void 
    reset()
    {
        std::lock_guard<std::mutex> lock(mutex_);
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
        const auto wall_start = clock::now();
        const double cpu_start_sec = thread_cpu_seconds();

        std::lock_guard<std::mutex> lock(mutex_);
        running_ = true;
        wall_start_ = wall_start;
        cpu_start_sec_ = cpu_start_sec;
    }

    void 
    end() 
    {
        const auto wall_end = clock::now();
        const double cpu_end_sec = thread_cpu_seconds();

        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_)
            return;

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

        if (!msg.empty())
            std::cout << msg << "\t";

        if(data.cpu_time.no_data())
        {
            std::cout << "-\n";
            return;
        }
    
        std::cout << std::fixed << std::setprecision(4) << 1000 * data.cpu_time.mean() << " ms\n";

       // std::cout << " [" << wall_time_.count() << "] ";
        //std::cout << "Wall: " << wall_seconds() << " s (mean: " << wall_time_.mean() << " s, sd: " << wall_time_.standard_deviation() << " s) ";
       // std::cout << "CPU: " << cpu_seconds() << " s (mean: " << cpu_time_.mean() << " s, sd: " << cpu_time_.standard_deviation() << " s)" << std::endl;
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

    statistics wall_time_;
    statistics cpu_time_;
    mutable std::mutex mutex_;
};
