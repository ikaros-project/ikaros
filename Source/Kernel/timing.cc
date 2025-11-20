// timing.cc		Timing utilities for the IKAROS project

//  TODO: Use only chrono functions
//  std::this_thread::sleep_for(std::chrono::milliseconds(x));

#include "timing.h"

#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

using namespace std::chrono;


std::string TimeString(double time)
{
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
        Pause();
        SetPauseTime(0);
    }

void
Timer::SetPauseTime(double t)
{
    std::lock_guard<std::mutex> lock(mtx);

    if(paused)
    {
        auto dur = duration_cast<steady_clock::duration>(duration<double>(t));
        pause_time = start_time + dur;
    }
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
    std::lock_guard<std::mutex> lock(mtx);

    auto dur = duration_cast<steady_clock::duration>(duration<double>(t));
    start_time = steady_clock::now() - dur;
    if (paused)
        pause_time = start_time + dur;
}



double
Timer::GetTime()
{
    if(paused)
        return 0.001 * duration_cast<milliseconds>(pause_time - start_time).count();
    else
        return 0.001 * duration_cast<milliseconds>(steady_clock::now() - start_time).count();
}


void 
Timer::SetStartTime(double t)
{
    std::lock_guard<std::mutex> lock(mtx);

    auto d = duration<double>(t);
    start_time = steady_clock::time_point(duration_cast<steady_clock::duration>(d));
}


double
Timer::WaitUntil(double time)
{
    // locked = true; // prevent changes while waiting
    Lock();
    float dt;
	while ((dt = GetTime()-time) < -0.128){ std::this_thread::sleep_for(microseconds(127000)); };
	while ((dt = GetTime()-time) < -0.004){ std::this_thread::sleep_for(microseconds(3000)); };
	while ((dt = GetTime()-time) < -0.001){ std::this_thread::sleep_for(microseconds(100)); };
    while(GetTime() < time)
        std::this_thread::sleep_for(microseconds(1)); // minimal sleep to avoid busy-waiting
    //locked = false;
    Unlock();
    return GetTime() - time;
}



Timer::Timer():
    paused(false)
{
    Restart();
}



std::string
Timer::GetTimeString()
{
    return TimeString(GetTime());
}


