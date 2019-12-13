//
//	DelayCounter.cc		This file is a part of the IKAROS project
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

#include "DelayCounter.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;


void
DelayCounter::Init()
{
    Bind(count_threshold, "count_threshold");
    Bind(reset_threshold, "reset_threshold");
	Bind(debugmode, "debug");    

    io(input_array, input_array_size,"INPUT");
    
    io(output_array, output_array_size, "OUTPUT");
    io(write_out, "WRITE");

    counter = new int[input_array_size];
}



DelayCounter::~DelayCounter()
{
    // Destroy data structures that you allocated in Init.
    delete[] counter;
}



void
DelayCounter::Tick()
{
    write_out[0] = 0.f;
    for(int i = 0; i < input_array_size; i++)
    {
        
        if(input_array[i] >= count_threshold)
            counter[i]++;
        else if(input_array[i] < reset_threshold && counter[i] > 0)
        {
            write_out[0] = 1.f;
            output_array[i] = (float)counter[i];
            counter[i] = 0;
        }
    }
    
	if(debugmode)
	{
		// print out debug info
        print_array("counter: ", counter, input_array_size);
	}
}



// Install the module. This code is executed during start-up.

static InitClass init("DelayCounter", &DelayCounter::Create, "Source/UserModules/DelayCounter/");


