//
//    Arbiter.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2006-2016 Christian Balkenius
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

#include "Arbiter.h"

using namespace ikaros;


Arbiter::Arbiter(Parameter * p):
    Module(p)
{
    Bind(switch_time, "switch_time");
    
    no_of_inputs = GetIntValue("no_of_inputs");

    input_name  = new char * [no_of_inputs];
    value_name  = new char * [no_of_inputs];

    input      = new float * [no_of_inputs];
    value_in   = new float * [no_of_inputs];

    for (int i=0; i<no_of_inputs; i++)
    {
        AddInput(input_name[i] = create_formatted_string("INPUT_%d", i+1));
        AddInput(value_name[i] = create_formatted_string("VALUE_%d", i+1));
    }

    AddOutput("OUTPUT");
    AddOutput("VALUE");
}



Arbiter::~Arbiter()
{
    for (int i=0; i<no_of_inputs; i++)
    {
        destroy_string(input_name[i]);
        destroy_string(value_name[i]);
    }

    delete [] input_name;
    delete [] value_name;

    delete [] input;
    delete [] value_in;
}



void
Arbiter::SetSizes()
{
    int sx = 0;
    int sy = 0;

    for(int i=0; i<no_of_inputs; i++)
    {
        int sxi = GetInputSizeX(input_name[i]);
        int syi = GetInputSizeY(input_name[i]);

        if(sxi == unknown_size)
            return; // Not ready yet

        if(syi == unknown_size)
            return; // Not ready yet

        if(sx != 0 && sxi != 0 && sx != sxi)
            Notify(msg_fatal_error, "Inputs have different sizes");

        if(sy != 0 && syi != 0 && sy != syi)
            Notify(msg_fatal_error, "Inputs have different sizes");
        
        sx = sxi;
        sy = syi;
    }

    SetOutputSize("OUTPUT", sx, sy);
    SetOutputSize("VALUE", 1);
}



void
Arbiter::Init()
{
   for(int i=0; i<no_of_inputs; i++)
    {
        input[i] = GetInputArray(input_name[i]);
        value_in[i] = GetInputArray(value_name[i], false);
    }

    output = GetOutputArray("OUTPUT");
    value_out = GetOutputArray("VALUE");

    size = GetOutputSize("OUTPUT");
    
    current_channel = -1;
}


// Slow switching should use normalized weight vector to produce convex combinations of inputs
// This is necessary when a switch occurs before the switch time has ended
// Special case of convex combination of processes - WITH selection in addition to weighting
// Could weigh by value without competition as a special case

void
Arbiter::Tick()
{
    int   ix = 0;
    float vix = 0;
    
    for(int i=0; i<no_of_inputs; i++)
    {
        float v = (value_in[i] ? *value_in[i] : norm(input[i], size)); // use norm if value input is not connected
        if(v > vix)
        {
            ix = i;
            vix = v;
        }
    }
    
    if(switch_time == 0)
        copy_array(output, input[ix], size);
    
    else // run slow switch
    {
        if(ix != current_channel)  // start switch
        {
			if (current_channel == -1)
				current_channel = ix;
				
            from_channel = current_channel;
            current_channel = ix;
            switch_counter = switch_time;
        }
        
        if(switch_counter == 0)
            copy_array(output, input[ix], size);
        else
        {
            switch_counter--;
            float a = float(switch_time-switch_counter)/float(switch_time);
            for(int i=0; i<no_of_inputs; i++)
                output[i] = a * input[current_channel][i] + (1-a) * input[from_channel][i];
        }
    }

    *value_out = *value_in[ix];
}



static InitClass init("Arbiter", &Arbiter::Create, "Source/Modules/UtilityModules/Arbiter/");
