//
//	timing.h		Timing utilities for the IKAROS project
//
//    Copyright (C) 2023  Christian Balkenius
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    See http://www.ikaros-project.org/ for more information.
//

#pragma once

#include <chrono>
#include <thread>
#include <string>


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
    std::atomic<bool>                                  locked;
    bool paused;
public: 
    void        Pause();                    // Pause the timer
    void        Continue();                 // Start the timer at the pause time
    void        SetPauseTime(double t);     // Change the time wheere the timer start when Continue is called

    void		Restart();					// Start the timer from time 0
    void        SetTime(double t);          // Set start time of the timer
    double      GetTime();					// Get the time (in seconds) since the timer was created or restarted
    std::string GetTimeString();            // Get time since the timer was started as a formated string
    double      WaitUntil(double time); // Suspend execution until time; return timing lag

    Timer();
};



class Profiler : public Timer
{
public:
    Profiler &  Reset();                        // Reset accumulators
    Profiler &  Start();                        // Start one sample
    Profiler &  Stop();                         // Stop the sampe and add to accumulator
    double  GetAverageTime();               // Get the avreage time over all samples

    Profiler &  Print(std::string msg="");

    long    number_of_samples = 0;          // The number of samples
    double  accumulated_time = 0;           // Total time used (in seconds)
};



