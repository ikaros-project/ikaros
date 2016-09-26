//
//	MotionGuard		This file is a part of the IKAROS project
//
//    Copyright (C) 2015-2016 Christian Balkenius
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

#include "MotionGuard.h"

using namespace ikaros;


void
MotionGuard::Init()
{
    Bind(max_speed, "max_speed");
    Bind(log, "log");
    
    start_up_time = GetIntValue("start_up_time");
    
    input = GetInputArray("INPUT");
    reference = GetInputArray("REFERENCE");
    size = GetInputSize("INPUT");
    start_up_position = NULL;
    input_cleaned = create_array(size);
    output = GetOutputArray("OUTPUT");

	inputLimitMin = GetArray("input_limit_min", size);
	inputLimitMax = GetArray("input_limit_max", size);
	Notify(msg_print,"MOTION GUARD ACTIVE\n");
}



MotionGuard::~MotionGuard()
{
    destroy_array(start_up_position);
}



void
MotionGuard::Tick()
{
    bool err = false;
    bool speed_limit = false;
	bool position_limit = false;
	
    long t = GetTick();

    // Set default output = current position (until neutral position parameter is added)

    copy_array(output, reference, size);
    
    if(t == 1)
        start_up_position = copy_array(create_array(size), reference, size);
    
    // Clean-up input

    for(int i=0; i<size; i++)
        if(input[i] != 0)
            input_cleaned[i] = input[i];
        else if(reference[i] != 0)
            input_cleaned[i] = reference[i];
        else if(start_up_position)
            input_cleaned[i] = start_up_position[i];
        else
        {
            input_cleaned[i] = 0;
            err = true;
        }
    
    // Handle start up smoothing
    
    if(t < start_up_time)
    {
        float a = float(t)/float(start_up_time);
        for(int i=0; i<size; i++)
            if(input_cleaned[i] != 0 && reference[i] != 0)
                output[i] = a * input_cleaned[i] + (1-a) * reference[i];
    }
    else
    {
        copy_array(output, input_cleaned, size);
    }
    
    // Check maximum speed
    
    float m = 0;
    for(int i=0; i<size; i++)
    {
        float speed = (output[i] - reference[i]);
        if(speed > m)
            m = speed;
    }
    
    if(m > max_speed)
    {
        speed_limit = true;
        float scale = max_speed/m;
        for(int i=0; i<size; i++)
        {
            float speed = (output[i] - reference[i]);
            output[i] = reference[i] + scale * speed;
        }
    }
	
	// Check position software limit
    if (inputLimitMin && inputLimitMax)
    {
        for(int i=0; i<size; i++)
        {
            if (output[i] < inputLimitMin[i] || output[i] > inputLimitMax[i])
                position_limit = true;
            if (output[i] < inputLimitMin[i])
                output[i] = inputLimitMin[i];
            if (output[i] > inputLimitMax[i])
                output[i] = inputLimitMax[i];
        }
    }
	
    // Check zeros again
    for(int i=0; i<size; i++)
        if(output[i] == 0)
            err = true;
	

	if(position_limit)
		Notify(msg_warning, "Position is out of range and has been limited.");
	
    if(speed_limit)
        Notify(msg_warning, "Speed is out of range and has been limited.");
    
    if(err)
        Notify(msg_warning, "Position array contains zeros and should not be used.");
}



static InitClass init("MotionGuard", &MotionGuard::Create, "Source/Modules/RobotModules/MotionGuard/");

