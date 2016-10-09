//
//    EventTrigger.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2016  Christian Balkenius
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


#include "EventTrigger.h"

using namespace ikaros;

void
EventTrigger::Init()
{
    output      = GetOutputArray("OUTPUT");
    size        = GetOutputSize("OUTPUT");
    
    next        = GetInputArray("NEXT");
    next_size   = GetInputSize("NEXT");
    
    counter     = GetIntValue("initial_delay");
    current_state = -1;
    last_state = -1;
    
    duration    = GetIntArray("duration", size);
    timeout     = GetIntArray("timeout", size);
    
}



void
EventTrigger::SelectNewState()  // Choose new state - or same if there is only one
{
        int new_state = random(size);
        if(size > 1)
            while(new_state == last_state)
                new_state = random(size);
        
        reset_array(output, size);
        output[new_state] = 1;
        
        counter = duration[new_state];
        current_state = new_state;
        last_state = new_state;
}



void
EventTrigger::Tick()
{
    if(next && (norm(next, next_size) > 0))
    {
        SelectNewState();
    }
    
    else if(counter <= 0)
    {
        if(current_state != -1) // end of state
        {
            reset_array(output, size);
            counter = timeout[current_state];
            current_state = -1;
        }
        else
        {
            SelectNewState();
        }
    }
    
    else
    {
        counter--;
    }

}



static InitClass init("EventTrigger", &EventTrigger::Create, "Source/Modules/UtilityModules/EventTrigger/");

