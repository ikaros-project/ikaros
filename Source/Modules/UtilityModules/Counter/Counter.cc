//
//	Counter.cc		This file is a part of the IKAROS project
//					A module that counts how many time its input is above a threshold
//
//    Copyright (C) 2004  Christian Balkenius
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


#include "Counter.h"

void
Counter::Init()
{
    mode            = GetIntValueFromList("mode");
    threshold		= GetFloatValue("threshold");
    
    reset_interval  = GetIntValue("reset_interval");
    print_interval  = GetIntValue("print_interval");
    count_interval  = GetIntValue("count_interval");
    
    interval_counter = 0;
    integrator = 0;

    size	= GetInputSize("INPUT");
    input   = GetInputArray("INPUT");
    count	= GetOutputArray("COUNT");
    percent	= GetOutputArray("PERCENT");

    tick_counter = 0;
    counter = 0;
}



void
Counter::Tick()
{
    for (int i=0; i<size; i++)
        integrator += input[i];

    if (interval_counter < count_interval-1)
    {
        interval_counter++;
        return;
    }

    tick_counter++;
    sample_counter++;

    if (integrator > threshold)
        counter++;

    count[0] = float(counter);
    percent[0] = float(counter)/float(sample_counter);

    if (tick_counter % print_interval == 0)
        Notify(msg_print, "COUNTER: %7ld %7ld %7.2f%%\n", tick_counter, counter, 100*percent[0]);

    if (sample_counter >= reset_interval)
    {
        sample_counter = 0;
        counter = 0;
    }

    interval_counter = 0;
    integrator = 0;
}


