//
//	IKAROS_Timer.h		Timer utilities for the IKAROS project
//
//    Copyright (C) 2006  Birger Johansson
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
//    This file implements a timer class using windows multimedia timing
//    This file uses the libwinmm.a libary in the lib directory
//

#include "IKAROS_System.h"

#ifdef WINDOWS

#include "IKAROS_Timer.h"
#include "windows.h"

#define usleep(x) ::Sleep(x/1000)

class TimerData
{
public:
    DWORD startTime;
    UINT period;
};

void
Timer::Restart()
{
    data->startTime = timeGetTime();
}

float
Timer::GetTime()
{
    return (timeGetTime() - data->startTime);
}

float
Timer::WaitUntil(float time)
{
    float dt;
//	if(time - t > 1.2)	sleep(long(t-0.2));

    while ((dt = GetTime()-time) < 0) ;
    return dt;
}

Timer::Timer()
{
    data = new TimerData();
    // Set the highest resolution for the multimedia timer
    TIMECAPS tc;
    // Retrieve timer caps, which contain resolution range
    timeGetDevCaps(&tc, sizeof tc);
    // Store period in a member variable so that we can restore it when program terminates
    data->period = tc.wPeriodMin;
    // Set resolution with this call
    timeBeginPeriod(tc.wPeriodMin);

    Restart();
}

Timer::~Timer()
{
    timeEndPeriod(data->period);
    delete data;
}

void Timer::Sleep(float time)
{
    usleep(long(1000*time));
}

#endif

