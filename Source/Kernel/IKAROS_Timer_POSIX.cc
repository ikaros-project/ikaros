//
//	IKAROS_Timer.h		Timer utilities for the IKAROS project
//
//    Copyright (C) 2006-2021  Christian Balkenius
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
//	This file implements a timer class using the POSIX timing functions
//

#include "IKAROS_System.h"

#ifdef POSIX

#include "IKAROS_Timer.h"

#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>

#include <chrono>

using namespace std::chrono;

class TimerData
{
public:
    struct timeval start_time;
};



void
Timer::Reset()
{

    timer_time = 0;
    gettimeofday(&(data->start_time), NULL);
}



void
Timer::Restart()
{
    timer_time = 0;
    gettimeofday(&(data->start_time), NULL);
    running = true;
}



void
Timer::Start()
{
    timer_time = GetTime();
    gettimeofday(&(data->start_time), NULL);
    running = true;
}




void
Timer::Stop()
{
    timer_time = GetTime();
    running = false;
}



float
Timer::GetTime()
{
    if(!running)
        return timer_time;

    struct timeval stop_time;
    gettimeofday(&stop_time, NULL);
    long sec_diff = stop_time.tv_sec - data->start_time.tv_sec;
    long usec_diff = stop_time.tv_usec - data->start_time.tv_usec;

    return  timer_time + 1E3*float(sec_diff) + float(usec_diff)/1E3; // difference in milliseconds
}



void
Timer::SetTime(float time)
{
    timer_time = time;
    gettimeofday(&(data->start_time), NULL);
}



float
Timer::WaitUntil(float time)
{
    float dt;
	while ((dt = GetTime()-time) < - 128){usleep(127000);};
	while ((dt = GetTime()-time) < - 4){usleep(3000);};
	while ((dt = GetTime()-time) < - 1){usleep(100);};
	return dt;
}



Timer::Timer(bool run)
{
    running = run;
    timer_time = 0;
    data = new TimerData();
    gettimeofday(&(data->start_time), NULL);
}



Timer::~Timer()
{
    delete data;
}



void Timer::Sleep(float time)
{
    usleep(useconds_t(1000*time));
}



long Timer::GetRealTime()
{
    milliseconds ms = duration_cast< milliseconds >(
        system_clock::now().time_since_epoch()
    );
    
    return ms.count();
}

#endif
