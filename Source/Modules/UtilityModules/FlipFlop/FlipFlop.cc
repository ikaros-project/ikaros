//
//	FlipFlop.cc		This file is a part of the IKAROS project
//				
//
//    Copyright (C) 2022  Christian Balkenius
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


#include "FlipFlop.h"


void
FlipFlop::Init()
{
    Bind(high, "high");
    Bind(low, "low");
    Bind(threshold, "threshold");
    Bind(invert_inputs, "invert_inputs");
    Bind(enable_on, "enable_on");
    Bind(type, "type");

    set	= GetInputArray("SET");
    reset	= GetInputArray("RESET");
    enable	= GetInputArray("ENABLE");
    output	= GetOutputArray("OUTPUT");
    inverse	= GetOutputArray("INVERSE");
}



void
FlipFlop::Tick()
{
    bool e = !enable || *enable > threshold;
    switch(enable_on)
    {
        case 0:  if(!e) {last_enable = e; return;} break;
        case 1:  if(e) {last_enable = e;return;} break;
        case 2:  if(!e||last_enable) {last_enable = e;return;} break;
        case 3:  if(e||!last_enable) {last_enable = e;return;} break;
        default: Notify(msg_warning, "Invalid value for parameter 'enable_on'.");
    }
    last_enable = e;

    bool s = *set > threshold;
    bool r = *reset > threshold;

    if(invert_inputs)
    {
        s = !s;
        r = !r;
    }

    switch(type)
    {
        case 0: // SR
            if(s)
            {
                *output = high;
                *inverse = low; 
            }
            else if(r)
            {
                *output = low;
                *inverse = high; 
            }
            break;

        case 1: // JK
                if(s && r)
            {
                *output  = *output  > threshold ? low : high;
                *inverse = *inverse > threshold ? low : high; 
            }
            else if(s)
            {
                *output = high;
                *inverse = low; 
            }
            else if(r)
            {
                *output = low;
                *inverse = high; 
            }
            break;

        case 2: // D
                if(s)
            {
                *output  = high;
                *inverse = low; 
            }
            else
            {
                *output = low;
                *inverse = high; 
            }
            break;
        default:
            Notify(msg_warning,"Invalid vaue for parameer 'type.");
    }
}



static InitClass init("FlipFlop", &FlipFlop::Create, "Source/Modules/UtilityModules/FlipFlop/");

