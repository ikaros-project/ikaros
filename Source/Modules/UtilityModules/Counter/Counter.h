//
//	Counter.h			This file is a part of the IKAROS project
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

#ifndef COUNTER
#define COUNTER

#include "IKAROS.h"


class Counter: public Module
{
public:
    static Module * Create(Parameter * p) { return new Counter(p); }

    Counter(Parameter * p) : Module(p) {}
    virtual ~Counter() {}

    void		Init();
    void		Tick();

    int         mode;   // count each (0), count any (1)
    float       threshold;
    int         reset_interval;
    int         print_interval;
    int         count_interval;
    int         interval_counter;

    int         size;

    long int	counter;
    long int	sample_counter;
    long int	tick_counter;

    float		integrator;

    float *     input;
    float *     count;
    float *     percent;
};


#endif
