//
//	PlanarArm.cc		This file is a part of the IKAROS project
//					Implements a simple arm and a target object
//
//    Copyright (C) 2007 Christian Balkenius
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

#include "PlanarArm.h"

using namespace ikaros;

void
PlanarArm::UpdateArm()
{
    // Check bounds of angles

    angles[0] = clip(angles[0], -pi/1.5, pi/1.5);
    angles[1] = clip(angles[1], 0, pi);
    angles[2] = clip(angles[2], 0, pi);

    // Set joints

    joints[0][0] = 0.5;
    joints[0][1] = 0.0;

    joints[1][0] = joints[0][0] + segment_length[0] * sin(angles[0]);
    joints[1][1] = joints[0][1] + segment_length[0] * cos(angles[0]);

    joints[2][0] = joints[1][0] + segment_length[1] * sin(angles[1]+angles[0]);
    joints[2][1] = joints[1][1] + segment_length[1] * cos(angles[1]+angles[0]);

    joints[3][0] = joints[2][0] + segment_length[2] * sin(angles[2]+angles[1]+angles[0]);
    joints[3][1] = joints[2][1] + segment_length[2] * cos(angles[2]+angles[1]+angles[0]);

    hand_position[0] = joints[3][0];
    hand_position[1] = joints[3][1];
}



void
PlanarArm::MoveArm()
{
    subtract(movement, desired_angles, angles, 3);

    // amax

    float m = abs(movement[0]);
    if (abs(movement[1]) > m)
        m =abs(movement[1]);
    if (abs(movement[2]) > m)
        m =abs(movement[2]);

    // correct for maximum speed

    if (m > speed)
        multiply(movement, speed/m, 3);

//	printf("%f %f %f\n", movement[0], movement[1], movement[2]);

    float a0 = angles[0];
    float a1 = angles[1];
    float a2 = angles[2];

    add(angles, movement, 3);
    UpdateArm();

    // Reject positions outside the 0.01-0.99 frame

    if (hand_position[0] < 0.01f || hand_position[0]>0.99f || hand_position[1] < 0.01f || hand_position[1]>0.99f)
    {
        angles[0] = a0;
        angles[1] = a1;
        angles[2] = a2;
        UpdateArm();
    }
}



void
PlanarArm::Init()
{
    target_behavior	= GetIntValueFromList("target_behavior");
    target_speed = GetFloatValue("target_speed");
    target_size = GetFloatValue("target_size");
    target_range = GetFloatValue("target_range");
    target_noise = GetFloatValue("target_noise");
    
    speed = GetFloatValue("speed");
    
    grasp_limit = GetFloatValue("grasp_limit");

    target = GetOutputArray("TARGET");
    joints = GetOutputMatrix("JOINTS");
    hand_position = GetOutputArray("HAND_POSITION");
    angles = GetOutputArray("ANGLES");
    desired_angles = GetInputArray("DESIRED_ANGLES", false);	// check size = 3 if connected
    movement = create_array(3);

    distance = GetOutputArray("DISTANCE");
    contact = GetOutputArray("CONTACT");

    segment_length = create_array(3);

    segment_length[0] = 0.35;
    segment_length[1] = 0.25;
    segment_length[2] = 0.20;

    set_array(angles, 0.4, 3);

    UpdateArm();
}



void
PlanarArm::Tick()
{
    // Arm simulation

    if (desired_angles)
        MoveArm();

    // Detect grasp posibility

    *distance = dist(hand_position, target, 2);
    if (*distance < target_size)
        *contact = 1;
    else
        *contact = 0;

    // Target simulation

    switch (target_behavior)
    {
    case 0: // random
    {
        if (time == 0 || *distance < grasp_limit || time % 100 == 0)
        {
            target[0] = random(0.1, 0.9);
            target[1] = random(0.1, 0.9);
        }
        break;
    }
    case 1: // linear (sin speed)
    {
        target[0] = 0.5+target_range*sin(target_speed*float(time));
        target[1] = 0.5;
        break;
    }
    case 2:	// circle
    {
        target[0] = 0.5+target_range*sin(target_speed*float(time));
        target[1] = 0.5+target_range*cos(target_speed*float(time));
        break;
    }

    case 3:	// lisajous
    {
        target[0] = 0.5+target_range*sin(target_speed*float(time));
        target[1] = 0.5+target_range*cos(0.3*target_speed*float(time));
        break;
    }
    case 4:	// square
    {
        float x = sin(target_speed*float(time));
        float y = cos(target_speed*float(time));
        float L = abs(x)+abs(y);
        target[0] = 0.5+target_range*(L != 0 ? x/L : 0);
        target[1] = 0.5+target_range*(L != 0 ? y/L : 0);
        break;
    }
    default:

        break;
    }

//	printf("%f %f\t", target[0], target[1]);	// Actual

    target[0] += gaussian_noise(0, target_noise);
    target[1] += gaussian_noise(0, target_noise);

    if (*distance < grasp_limit)
        time = int(random(0, 100));

    time++;
}


