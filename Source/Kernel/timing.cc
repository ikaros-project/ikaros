// timing.cc		Timing utilities for the IKAROS project

//  TODO: Use only chrono functions
//  std::this_thread::sleep_for(std::chrono::milliseconds(x));

#include "timing.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <stdexcept>

using namespace std::chrono;

namespace
{
steady_clock::duration
checked_steady_duration(double seconds, const char * operation)
{
    if(!std::isfinite(seconds))
        throw std::invalid_argument(std::string(operation) + " requires a finite time in seconds");

    const long double value = static_cast<long double>(seconds);
    const long double minimum = duration<long double>(steady_clock::duration::min()).count();
    const long double maximum = duration<long double>(steady_clock::duration::max()).count();
    if(value < minimum || value > maximum)
        throw std::out_of_range(std::string(operation) + " is outside the steady-clock range");

    return duration_cast<steady_clock::duration>(duration<long double>(value));
}


steady_clock::time_point
checked_add(steady_clock::time_point base,
            steady_clock::duration offset,
            const char * operation)
{
    using representation = steady_clock::duration::rep;
    const representation base_count = base.time_since_epoch().count();
    const representation offset_count = offset.count();
    const representation minimum = steady_clock::duration::min().count();
    const representation maximum = steady_clock::duration::max().count();

    if((offset_count > 0 && base_count > maximum - offset_count) ||
       (offset_count < 0 && base_count < minimum - offset_count))
        throw std::out_of_range(std::string(operation) + " is outside the steady-clock range");

    return steady_clock::time_point(steady_clock::duration(base_count + offset_count));
}


steady_clock::time_point
checked_subtract(steady_clock::time_point base,
                 steady_clock::duration offset,
                 const char * operation)
{
    using representation = steady_clock::duration::rep;
    const representation base_count = base.time_since_epoch().count();
    const representation offset_count = offset.count();
    const representation minimum = steady_clock::duration::min().count();
    const representation maximum = steady_clock::duration::max().count();

    if((offset_count > 0 && base_count < minimum + offset_count) ||
       (offset_count < 0 && base_count > maximum + offset_count))
        throw std::out_of_range(std::string(operation) + " is outside the steady-clock range");

    return steady_clock::time_point(steady_clock::duration(base_count - offset_count));
}
}


double
CPUUsageFraction(double cpu_time_delta, double wall_time_delta, int cpu_cores)
{
    if(!std::isfinite(cpu_time_delta) || !std::isfinite(wall_time_delta) ||
       cpu_time_delta <= 0 || wall_time_delta <= 0 || cpu_cores < 1)
        return 0;

    return std::clamp(cpu_time_delta / (wall_time_delta * double(cpu_cores)), 0.0, 1.0);
}


std::string TimeString(double time)
{
    if(time < 0)
        return "--:--:--.---";

    int days = time / 86400;
    time -= (double(days) * 86400.0);

    int hours = time / 3600;
    time -= (double(hours) * 3600.0);

    int minutes = time / 60;
    double seconds = time - (double(minutes) * 60.0);

    std::ostringstream oss;
    if (days > 0)
        oss << days << " " << std::setw(2) << std::setfill('0') << hours << ":"
            << std::setw(2) << std::setfill('0') << minutes << ":"
            << std::fixed << std::setprecision(3) << seconds;
    else
        oss << std::setw(2) << std::setfill('0') << hours << ":"
            << std::setw(2) << std::setfill('0') << minutes << ":"
            << std::fixed << std::setprecision(3) << seconds;

    return oss.str();
}


void
Sleep(double time)
{
    std::this_thread::sleep_for(duration<double>(time));
}


long
GetTimeStamp()
{
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}



std::string 
GetClockTimeString()
{
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}


void
Timer::Pause()
{
    std::lock_guard<std::mutex> lock(mtx);

    if(!paused)
    {
        pause_time = steady_clock::now();
        paused = true;
    }
}


void
Timer::Continue()
{
    std::lock_guard<std::mutex> lock(mtx);

    if (paused)
    {
        auto offset = steady_clock::now() - pause_time;
        start_time += offset;
        paused = false;
    }
}

void
Timer::Stop()
{
    std::lock_guard<std::mutex> lock(mtx);

    pause_time = start_time;
    paused = true;
}

void
Timer::SetPauseTime(double t)
{
    const steady_clock::duration duration = checked_steady_duration(t, "Timer::SetPauseTime()");
    std::lock_guard<std::mutex> lock(mtx);

    if(paused)
        pause_time = checked_add(start_time, duration, "Timer::SetPauseTime()");
}


void
Timer::Restart()
{
    std::lock_guard<std::mutex> lock(mtx);

    start_time = steady_clock::now();
    pause_time = start_time;
    paused = false;
}


void 
Timer::SetTime(double t)
{
    const steady_clock::duration duration = checked_steady_duration(t, "Timer::SetTime()");
    const steady_clock::time_point now = steady_clock::now();
    const steady_clock::time_point new_start = checked_subtract(now, duration, "Timer::SetTime()");

    std::lock_guard<std::mutex> lock(mtx);

    start_time = new_start;
    if (paused)
        pause_time = now;
}



double
Timer::GetTime() const
{
    std::lock_guard<std::mutex> lock(mtx);
    const auto end_time = paused ? pause_time : steady_clock::now();
    return duration<double>(end_time - start_time).count();
}


double
Timer::WaitUntil(double time)
{
    checked_steady_duration(time, "Timer::WaitUntil()");

    steady_clock::time_point wait_start;
    double remaining;
    {
        std::lock_guard<std::mutex> lock(mtx);
        wait_start = steady_clock::now();
        const auto timer_time = paused ? pause_time : wait_start;
        remaining = time - duration<double>(timer_time - start_time).count();
    }

    const auto lag = [wait_start, remaining]() {
        return duration<double>(steady_clock::now() - wait_start).count() - remaining;
    };

    while (lag() < -0.128)
        std::this_thread::sleep_for(microseconds(127000));
    while (lag() < -0.004)
        std::this_thread::sleep_for(microseconds(3000));
    while (lag() < -0.001)
        std::this_thread::sleep_for(microseconds(100));
    while (lag() < 0.0)
        std::this_thread::sleep_for(microseconds(1)); // minimal sleep to avoid busy-waiting
    return lag();
}



Timer::Timer():
    paused(false)
{
    Restart();
}



std::string
Timer::GetTimeString() const
{
    return TimeString(GetTime());
}
