//
//	SSC32.cc    This file is a part of the IKAROS project
//              Driver for the SSC-32 servo controller from Lynxmotion
//
//    Copyright (C) 20072010 Christian Balkenius, Magnus Johnsson
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
//	Created: 2007-08-27
//


#include "SSC32.h"

using namespace ikaros;

Module * SSC32::Create(Parameter * p)
{
    return new SSC32(p);
}



SSC32::SSC32(Parameter * p):
        Module(p)
{
    no_of_servos = GetIntValue("no_of_servos", unknown_size);    // Default to automatic size setting

    AddInput("INPUT");
    AddOutput("OUTPUT", no_of_servos);
    
    max_speed = GetIntValue("maxspeed", 100);
    timebase = GetIntValue("timebase", 1000);
    min_position = GetArray("min", 32);
    max_position = GetArray("max", 32);
    home = GetArray("home", 32);
    device = GetValue("device");
    
    if(!device)
    {
        Notify(msg_fatal_error, "Device not given - will not be able to communicate with SSC-32.\n");
        return;
    }
}



void
SSC32::SetSizes()
{
    if(GetOutputSize("OUTPUT") != unknown_size)
        return;

    SetOutputSize("OUTPUT", GetInputSize("INPUT"));
}



void
SSC32::SendPositions(float * pos, int t)
{
    bool debug = false;
    if(debug) printf("Sending [%d]: ", size);
    for(int i=0; i<size; i++)
    {
        float v = pos[i];
        if(v > max_position[i])
            v = max_position[i];
        else if(v < min_position[i])
            v = min_position[i];
         if(debug) printf("%5d", int(v));
        fprintf(ssc, "#%d P%d S%d", i, int(v), max_speed);
    }
    
    if(t > 0)
        fprintf(ssc, "T%d\r", t);
    else
        fprintf(ssc, "\r");
        
    if(debug) printf(" T%d\r", t);
    fflush(ssc);
    
    if(debug) printf("\n");
    if(debug) fflush(NULL);
}



void
SSC32::ReceivePositions()
{
    unsigned char positions[32];
    
    // Send request
    
    for(int i=0; i<size; i++)
        fprintf(ssc, "QP%d ", i);
    fprintf(ssc, "\r");
    fflush(ssc);
   
   // Receive result
   
    for(int i=0; i<size; i++)
    {
        fscanf(ssc, "%c", &positions[i]);
        if(max_position[i] != min_position[i])
            output[i] = (10*positions[i] - min_position[i])/(max_position[i] - min_position[i]);
        else
            output[i] = 0;
    }
}



void
SSC32::Init()
{
    input = GetInputArray("INPUT", false);
    size = GetInputSize("INPUT");
    output = GetOutputArray("OUTPUT");

    ssc = fopen(device, "r+");  // must be opened like this for bidiectional comomunication to work
    
    if(!ssc)
    {
        Notify(msg_fatal_error, "Device \"%s\"could not be opened - will not be able to communicate with SSC-32.\n", device);
        return;
    }
    
    SendPositions(home);
    
    generateOutput = GetBoolValue("generate_output") || OutputConnected("OUPUT");
}



SSC32::~SSC32()
{
    if(ssc)
    {
        SendPositions(home, 2500);
        fclose(ssc);
    }
    
    destroy_array(min_position);
    destroy_array(max_position);
    destroy_array(home);
}



void
SSC32::Tick()
{
    float pos[32];
    for(int i=0; i<size; i++)
        pos[i] = min_position[i] + (max_position[i] - min_position[i]) * input[i];
    SendPositions(pos, timebase);
    
    // Read current servo positions from the SSC32

    if(generateOutput)
        ReceivePositions();
}



