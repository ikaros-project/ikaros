//
//	MyModule.cc		This file is a part of the IKAROS project
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

#include "TrainHead.h"

// use the ikaros namespace to access the math library
// this is preferred to using <cmath>

using namespace ikaros;


void
TrainHead::Init()
{

  head1_angle_in = GetInputArray("HEAD1_ANGLE");
  head1_rotation_in = GetInputArray("HEAD1_ROTATION");
  head2_angle_in = GetInputArray("HEAD2_ANGLE");
  head2_rotation_in = GetInputArray("HEAD2_ROTATION");

  out_movements_angle = GetOutputArray("OUTPUT_ANGLES");
  out_movements_rotation = GetOutputArray("OUTPUT_ROTATIONS");

  mean_value_angle = GetOutputArray("MEAN_ANGLE");
  variance_value_angle = GetOutputArray("VARIANCE_ANGLE");

  mean_value_rotation = GetOutputArray("MEAN_ROTATION");
  variance_value_rotation = GetOutputArray("VARIANCE_ROTATION");

  write = GetOutputArray("WRITE");

  segment_size = GetOutputSize("OUTPUT_ANGLES");

  now_movement_angle = create_array(segment_size);
  now_movement_rotation = create_array(segment_size);

  past_movement_angle = create_array(segment_size);
  past_movement_rotation = create_array(segment_size);

  t=0;
  iteration=0;
  first = true;
}

TrainHead::~TrainHead()
{
    // Destroy data structures that you allocated in Init.
    // Do NOT destroy data structures that you got from the
    // kernel with GetInputArray, GetInputMatrix etc.
}

void
TrainHead::Tick()
{

  if(t < segment_size){
    write[0] = 0;
    out_movements_angle[t] = head1_angle_in[0];
    out_movements_rotation[t] = head1_rotation_in[0];

    now_movement_angle[t] = head2_angle_in[0];
    now_movement_rotation[t] = head2_rotation_in[0];
    t++;

  }else{
    if(first == true){
      first = false;
    }else{
      //Calculate average for past movement
      mean_value_angle[0] = mean(past_movement_angle,segment_size);
      mean_value_rotation[0] = mean(past_movement_rotation,segment_size);

      float sum1 = 0;
      float sum2 = 0;
      for(int y=0; y<segment_size; y++){
        sum1 = sum1 + sqr(past_movement_angle[y] - mean_value_angle[0]); //Calculate Variance
        sum2 = sum2 + sqr(past_movement_rotation[y] - mean_value_rotation[0]);
      }
      variance_value_angle[0] = sum1 / segment_size;
      variance_value_rotation[0] = sum2 / segment_size;

      write[0] = 1;
    }

    for(int i=0; i<segment_size; i++){
      past_movement_angle[i] = now_movement_angle[i];
      past_movement_rotation[i] = now_movement_rotation[i];
    }

    printf("Movement %i \n",iteration);
    iteration++;
    t=0;
  }
}

// Install the module. This code is executed during start-up.
static InitClass init("TrainHead", &TrainHead::Create, "Source/UserModules/Projects/StudentProjects/RobotMimicry/TrainHead/");
