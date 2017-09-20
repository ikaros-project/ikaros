//
//	  DirectionDetector.cc		This file is a part of the IKAROS project
//
//
//    Copyright (C) 2006  Christian Balkenius
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

#include "DirectionDetector.h"

using namespace ikaros;

void
DirectionDetector::Init()
{
	Bind(no_of_directions, "no_of_directions");

	size_x	 	= GetInputSizeX("INPUT");
	size_y	 	= GetInputSizeY("INPUT");

	input		= GetInputMatrix("INPUT");
	no_of_rows	= GetInputArray("NO-OF-ROWS");
    
	motion_vector		= GetOutputArray("MOTION-VECTOR");
	motion_direction	= GetOutputArray("MOTION-DIRECTION");
	looming				= GetOutputArray("LOOMING");
	left_field_motion	= GetOutputArray("LEFT-FIELD-MOTION");
	right_field_motion	= GetOutputArray("RIGHT-FIELD-MOTION");

	motion_vector_draw	= GetOutputArray("MOTION-VECTOR-DRAW");

	filtered_dx = 0;
	filtered_dy = 0;
	filtered_central_dx = 0;
	filtered_central_dy = 0;
}

#define SGN(x) (x == 0 ? 0 : (x>0 ? 1 : -1))

void
DirectionDetector::Tick() // average
{
	// Calculate average motion (from all detectors)
    
    float dx = 0;
    float dy = 0;
    for(int i=0; i<8; i++)
    {
        dx += input[i][2] - input[i][0];
    	dy += input[i][3] - input[i][1];
	}
    float a = atan2(dy, dx);
    float L = hypot(dy, dx);

	float alpha = 0.7;
    filtered_dx = alpha*filtered_dx + dx;
    filtered_dy = alpha*filtered_dy + dy;
    float af = atan2(filtered_dy, filtered_dx);

	a = af;

	motion_vector[0] = filtered_dx;
	motion_vector[1] = filtered_dy;

	motion_vector_draw[0] = 0.5;
	motion_vector_draw[1] = 0.5;
	motion_vector_draw[2] = 0.5+filtered_dx;
	motion_vector_draw[3] = 0.5+filtered_dy;

	// Calculate average motion (from center detectors) and get center detector vectors
    
    float adx = 0;
    float ady = 0;
    float c_dx[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    float c_dy[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    for(int i=0; i<8; i++)
    {
		c_dx[i] = input[i][2] - input[i][0];
		c_dy[i] = input[i][3] - input[i][1];
        adx += (1/8)*c_dx[i];
    	ady += (1/8)*c_dy[i];
	}

	// Remove average motion

    for(int i=0; i<4; i++)
    {
		c_dx[i] -= adx;
		c_dy[i] -= ady;
	}

    // Detect looming by checking if resulting vectos are pointing outward from the center
    // Use dot product with detector position in the image

    int loom_count = 0;

    for(int i=0; i<8; i++)
    {
    	float vx = SGN(input[i][0]-0.5);
        float vy = SGN(input[i][1]-0.5);

		if(vx*c_dx[i] + vy*c_dy[i] > 0.0005)
			loom_count++;
    }
	
//    printf("\t\t\t\t%d\n", loom_count);

    // DECIDE ON MOTION CATEGORY

    reset_array(motion_direction, 4);

    if(L > 0)
    {
		if(a < -3*pi/4)
        {
//        	printf("MOTION LEFT\n");
            motion_direction[0] = 1;
        }
        else if(a < -pi/4)
        {
//        	printf("MOTION UP\n");
            motion_direction[2] = 1;
        }
        else if(a < pi/4)
        {
//        	printf("MOTION RIGHT\n");
            motion_direction[1] = 1;
        }
        else if(a < 3*pi/4)
        {
//        	printf("MOTION DOWN\n");
            motion_direction[3] = 1;
        }
        else
        {
//        	printf("MOTION LEFT\n");
            motion_direction[0] = 1;
        }
    }

	if(loom_count >= 4)
    {
//		printf(">>>>>>>>>>>>>>>>>>> LOOM\n");
    	*looming = 1;
    }
    else
    	*looming = 0;
    
    *left_field_motion = hypot(input[0][2] - input[0][0], input[0][3] - input[0][1]) +
    					hypot(input[1][2] - input[1][0], input[1][3] - input[1][1]);
    
	*right_field_motion = hypot(input[2][2] - input[2][0], input[2][3] - input[2][1]) +
    					hypot(input[3][2] - input[3][0], input[3][3] - input[3][1]);
    
//	printf("LEFT: %f, RIGHT: %f\n", *left_field_motion, *right_field_motion);
}


static InitClass init("DirectionDetector", &DirectionDetector::Create, "Source/Modules/VisionModules/ImageOperators/DirectionDetector/");
