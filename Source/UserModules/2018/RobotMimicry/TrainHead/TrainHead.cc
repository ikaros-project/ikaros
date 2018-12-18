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
  head1     = GetInputArray("HEAD1");
  head2     = GetInputArray("HEAD2");

  out_movement = GetOutputArray("OUTPUT");
  write = GetOutputArray("WRITE");
  mean_value = GetOutputArray("MEAN");
  variance_value = GetOutputArray("VARIANCE");

  segment_size = GetOutputSize("OUTPUT");

  now_movement = create_array(segment_size);
  past_movement = create_array(segment_size);

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
    out_movement[t] = head1[0];
    now_movement[t] = head2[0];
    t++;

  }else{
    if(first == true){
      first = false;
    }else{
      //Calculate average for past movement
      mean_value[0] = mean(past_movement,segment_size);
      printf("MEAN VALUE: %f \n",mean_value[0]);

      float sum = 0;
      for(int y=0; y<segment_size; y++){
        sum = sum + sqr(past_movement[y] - mean_value[0]); //Calculate Variance
      }
      variance_value[0] = sum / segment_size;
      printf("Variance: %f \n",variance_value[0]);

      write[0] = 1;
      print_array("Movement old: ", past_movement, segment_size,2);
      print_array("Movement out: ", out_movement, segment_size,2);
    }

    for(int i=0; i<segment_size; i++){
      past_movement[i] = now_movement[i];
    }

    printf("Movement %i \n",iteration);
    iteration++;
    t=0;
  }
}

// Install the module. This code is executed during start-up.
static InitClass init("TrainHead", &TrainHead::Create, "Source/UserModules/TrainHead/");
