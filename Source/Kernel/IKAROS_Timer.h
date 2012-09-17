//
//	IKAROS_Timer.h		Timer utilities for the IKAROS project
//
//    Copyright (C) 2006  Christian Balkenius
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

#ifndef IKAROS_TIMER
#define IKAROS_TIMER

class TimerData;

class Timer
{
private:
    TimerData *	data;
public:
    void		Restart();					// Start the timer
    float		GetTime();					// Get the time (in milliseconds) since the timer was created or restarted
    float		WaitUntil(float time);		// Suspend execution until time; return timing lag (in milliseconds)

    Timer();
    ~Timer();

    static void Sleep(float time);			// Sleep this many milliseconds
};

#endif

