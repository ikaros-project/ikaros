//
//	MidiFilter.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2020 <Author Name>
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

#include "MidiFilter.h"
#include <iostream>
// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;

const int cMidiArraySize = 3;

MidiFilter::MidiFilter(Parameter *P):
Module(P)
{
    
    
    int sx = 0;
    float ** m = create_matrix(GetValue("filter"), sx, outs); 
    // input_name = new char *[ins];
    output_name = new char *[outs];

    output = new float *[outs];
    AddInput("INPUT");
    for (int i = 0; i < outs; i++)
        AddOutput(output_name[i] = create_formatted_string("OUTPUT_%d", i + 1));
    // add other IO channels here
    // AddOutput("OUTPUT");
    destroy_matrix(m);
}

MidiFilter::~MidiFilter()
{
    
    for (int i = 0; i < outs; i++)
        destroy_string(output_name[i]);
    delete[] output_name;
    delete[] output;
    delete[] output_sizes;

}

void 
MidiFilter::SetSizes()
{
    
    int sx = 0;
    int sy = 0;
    float ** m = create_matrix(GetValue("filter"), sx, sy); 
    // check
    if(sx != cMidiArraySize)
        Notify(msg_fatal_error, "MidiFilter error: size of filter columns must be 3");
    output_sizes = new int[sy];

    for (int i = 0; i < sy; i++)
        output_sizes[i] = m[i][2] - m[i][1];
    
    if (sx == unknown_size || sy == unknown_size)
        return; // Not ready yet
    for (int i = 0; i < outs; i++)
        SetOutputSize(output_name[i], output_sizes[i]);
    // other sizes here
    //size_x=sx;
    //size_y=sy;
    destroy_matrix(m);
}

void
MidiFilter::Init()
{
    // Bind(parameter, "parameter1");
    int sx=cMidiArraySize;
    Bind(filter_data, sx, outs, "filter", true);
    Bind(debugmode, "debug");

    
    io(input, input_size, "INPUT" );
    if(input_size!=cMidiArraySize)
        Notify(msg_fatal_error, "MidiFilter error: input size must be 3");
    for (int i = 0; i < outs; i++)
        output[i] = GetOutputArray(output_name[i]);

    // other outputs etc
    // io(output_array, output_array_size, "OUTPUT");
}



void
MidiFilter::Tick()
{
    if (debugmode)
    {
        printf("Instance name: %s\n", this->instance_name);
        // print out debug info
        print_array("INPUT", input, input_size);
        print_matrix("filter", filter_data, 3, outs);
    }
    // check which filters are activated
    for (int i = 0; i < outs; i++)
    {
        if(filter_data[i][0] == input[0])
        {
            int ix = input[1] - filter_data[i][1];
            if(ix < output_sizes[i])
                output[i][ix] = input[2];
            else 
                Notify(msg_warning, "input index exceeds output array size");
        }
    }
    

    
    
}

// Install the module. This code is executed during start-up.

static InitClass init("MidiFilter", &MidiFilter::Create, "Source/UserModules/MidiFilter/");
