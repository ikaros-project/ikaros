//
//	GoSignal.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2012 <Author Name>
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
//  This example is intended as a starting point for writing new Ikaros modules
//  The example includes most of the calls that you may want to use in a module.
//  If you prefer to start with a clean example, use he module MinimalModule instead.
//

#include "GoSignal.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;


void
GoSignal::Init()
{
    Bind(wait_low, "wait_low");
    Bind(wait_high, "wait_high");
    Bind(trigger_threshold, "trigger_threshold");
    Bind(reset_threshold, "reset_threshold");
	Bind(debugmode, "debug");    

    io(input_array, input_array_size,"INPUT");
    
    io(output_array, output_array_size, "OUTPUT");
    
    wait_time = new int[input_array_size];
    random_int(wait_time, wait_low, wait_high, input_array_size);
    reset = true;
}



GoSignal::~GoSignal()
{
    // Destroy data structures that you allocated in Init.
    delete[] wait_time;
}



void
GoSignal::Tick()
{
    for(int i = 0; i < input_array_size; i++)
    {
        // make sure cant hold the button down, have to let it up before
        // can trigger again

        // reset stop button when input goes below 
        // threshold, and go signal turned off
        //if (output_array[i] == 0.f && input_array[i] < reset_threshold)
        if (input_array[i] < reset_threshold)
            reset = true;
        // if go signal turned on, button has been reset, and input goes above
        // trigger threshold, turn signal off
        if(output_array[i] == 1.f && reset && input_array[i] > trigger_threshold)
        {
            output_array[i] = 0.f;
            wait_time[i] = random_int(wait_low, wait_high);
            reset = false;
        }
        
        if(wait_time[i] == 0)
            output_array[i] = 1.f;
        else if (output_array[i]==0)
            wait_time[i]--;
        
        

    }
    
	if(debugmode)
	{
		// print out debug info
        print_array("waiting times: ", (float*)wait_time, input_array_size, 1);
        print_array("input: ", input_array, input_array_size, 4);
        printf("trigger threshold = %f\n", trigger_threshold);
        printf("reset threshold =  %f\n\n", reset_threshold);
	}
}



// Install the module. This code is executed during start-up.

static InitClass init("GoSignal", &GoSignal::Create, "Source/UserModules/GoSignal/");


