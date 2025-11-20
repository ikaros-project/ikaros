//
//	timing.h		Timing utilities for the IKAROS project - Copyright (C) 2023  Christian Balkenius

#pragma once

#include <chrono>
#include <thread>
#include <string>
#include <mutex>


// All parameters are time in seconds represented by a double

std::string TimeString(double time);    // Convert double time to formatted string           
long        GetTimeStamp();             // Get long representation of clock time
std::string GetClockTimeString();       // Get string representation of clock time
void        Sleep(double d);		    // Sleep for this duration (in seconds)


class Timer
{
private:
    std::chrono::time_point<std::chrono::steady_clock> start_time;
    std::chrono::time_point<std::chrono::steady_clock> pause_time;  
    std::atomic<bool> paused;
    std::mutex mtx;
public: 
    void        Pause();                    // Pause the timer
    void        Continue();                 // Start the timer at the pause time
    void        Stop();                     // Pause and set time to zero

    void        SetPauseTime(double t);     // Change the time where the timer start when Continue is called

    void		Restart();					// Start the timer from time 0
    void        SetStartTime(double t);     // Set start time of the timer
    double      GetTime();					// Get the time (in seconds) since the timer was created or restarted
    void        SetTime(double t);          // Set the time (in seconds) of the timer
    std::string GetTimeString();            // Get time since the timer was started as a formated string
    double      WaitUntil(double time);     // Suspend execution until time; return timing lag

    void Lock() { mtx.lock(); }
    void Unlock() { mtx.unlock(); }

    Timer();
};


