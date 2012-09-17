//
//	FunctionGenerator.cc		This file is a part of the IKAROS project
//
//
//    Copyright (C) 2005 Christian Balkenius
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
//	Created: 2005-02-15
//



#include "FunctionGenerator.h"

using namespace ikaros;

void
FunctionGenerator::Init()
{
    Bind(type, "type");
    
    Bind(offset, "offset");
    Bind(amplitude, "amplitude");
    Bind(frequency, "frequency");
    Bind(shift, "shift");
    Bind(duty, "duty");
    
    Bind(basetime, "basetime");
    Bind(tickduty, "tickduty");
    
    output = GetOutputArray("OUTPUT");

    t = 0;
}



// Functions

static float
triangle(float t)
{
    float v =  2*(t/(2*pi)-trunc(t/(2*pi)));
    if (v<1.0)
        return v;
    else
        return 2-v;
}



static float
ramp(float t)
{
    float v =  t/(2*pi)-trunc(t/(2*pi));
    return v;
}



static float
square(float t, float d)
{
    float v =  t/(2*pi)-trunc(t/(2*pi));
    if (v<d)
        return 1;
    else
        return 0;
}



static float
ticksquare(int tick, int basetime, int duty)
{
    if ((tick % basetime) < duty)
        return 1;
    else
        return 0;
}



void
FunctionGenerator::Tick()
{
    switch (type)
    {
        case 0:
            output[0] = offset+amplitude*sin(frequency*(float(t)+shift));
            break;

        case 1:
            output[0] = offset+amplitude*triangle(frequency*(float(t)+shift));
            break;

        case 2:
            output[0] = offset+amplitude*ramp(frequency*(float(t)+shift));
            break;

        case 3:
            output[0] = offset+amplitude*square(frequency*(float(t)+shift), duty);
            break;

        case 4:
            output[0] = offset+amplitude*ticksquare(t, basetime, tickduty);
            break;

        default:
            output[0] = 0;
        }
    t++;
}


