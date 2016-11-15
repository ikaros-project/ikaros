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
    
    input           = GetInputArray("INPUT");
    reference       = GetInputArray("REFERENCE");
    size            = GetInputSize("INPUT");
    
    output          = GetOutputArray("OUTPUT");

	inputLimitMin   = GetArray("input_limit_min", size);
	inputLimitMax   = GetArray("input_limit_max", size);
	
    Notify(msg_print,"MOTION GUARD ACTIVE\n");
}


void
MotionGuard::Tick()
{
    bool speed_limit = false;
	bool position_limit = false;
	
    copy_array(output, input, size);
    
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

	if(position_limit)
		Notify(msg_warning, "Position is out of range and has been limited.");
	
    if(speed_limit)
        Notify(msg_warning, "Speed is out of range and has been limited.");
}

static InitClass init("MotionGuard", &MotionGuard::Create, "Source/Modules/RobotModules/MotionGuard/");