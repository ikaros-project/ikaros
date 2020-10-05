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

#include "MimicHead.h"
#include <iostream>
#include <cstdio>
#include <ctime>
// use the ikaros namespace to access the math library
// this is preferred to using <cmath>

using namespace ikaros;

void
MimicHead::Init()
{

  angles_out  = GetOutputArray("ANGLES");
  input_movement    = GetInputArray("MOVEMENT");
  mean_value       = GetInputArray("MEAN");
  variance_value		= GetInputArray("VARIANCE");

  head_angle_in		= GetInputArray("HEAD_ANGLE_IN");
  head_rotation_in = GetInputArray("HEAD_ROTATION_IN");

  out_head_angle = GetOutputArray("HEAD_ANGLE");
  out_head_angle[0] = 0;
  out_head_rotation = GetOutputArray("HEAD_ROTATION");
  out_head_rotation[0] = 0;

  input_length = GetInputSize("MOVEMENT");

  movement_matrix = create_matrix(input_length,100);
  mean_array = create_array(input_length);
  variance_array = create_array(input_length);

  buffer_angle = create_array(input_length);
  buffer_rotation = create_array(input_length);

  output_movements_angle = create_array(input_length);
  output_movements_rotation = create_array(input_length);

  set_array(output_movements_angle,0,input_length);

  load_data = true;
  movement_in_progress = false;

  Bind(baysian_threshold, "baysian_threshold");
  Bind(max_movements, "max_movements");
  Bind(outlier_limit_rotation, "outlier_limit_rotation");
  Bind(outlier_limit_angle, "outlier_limit_angle");

  Bind(limit_rotation, "limit_rotation");
  Bind(limit_angle, "limit_angle");

  t=0;
  movement_timer = 0;
  std::clock_t start_time = std::clock();
  movement_type = rand()%(4-1 + 1) + 1;
  row = 0;
}

MimicHead::~MimicHead()
{
    // Destroy data structures that you allocated in Init.
    // Do NOT destroy data structures that you got from the
    // kernel with GetInputArray, GetInputMatrix etc.
}

static inline int naive_bayesian(float * buffer, float * means, float * variances, int length,int max_movements,float threshold)
{
  float buffer_mean = mean(buffer,length);
  float out_prop = 0;
  float cat = -1;

  for(int i=0; i < max_movements; i++){
    float m = means[i];
    float v = variances[i];
    float prob = 1 / sqrt(2 * pi * v) * exp(-sqr(buffer_mean-m) / (2 * v));

    if(prob > out_prop){
      out_prop = prob;
      cat = i;
    }
  }
  printf("out_prop: %f \n",out_prop);
  if(out_prop < threshold){
    cat = -1;
  }
  return cat;
}

void
MimicHead::Tick()
{
  if(load_data){
    if(row >= max_movements){
      load_data = false;
    }
    for(int i = 0; i < input_length; i++){
        movement_matrix[row][i] = input_movement[i];
    }
    mean_array[row] = mean_value[0];
    variance_array[row] = variance_value[0];

    printf("\n ROW: %i \n",row);
    printf("MEAN: %f \n",mean_array[row]);
    print_array("Movement", movement_matrix[row], input_length,2);

    row++;

  }else{
      //This will fill the buffer
      if(t < input_length){
        buffer_angle[t] = head_angle_in[0];
        buffer_rotation[t] = head_rotation_in[0];
        t++;

      }else{

          for(int p = 0; p < input_length; p++){
            output_movements_rotation[p] = buffer_rotation[p];
          }

          /*Get the most probable category using the Naive Baysian algorithm.
          Will return -1 if the probability falls below threshold */
          int c = naive_bayesian(buffer_angle,mean_array,variance_array,input_length,max_movements,baysian_threshold);
          printf("\n Category: %i \n",c);

          if(c < 0){
              for(int k = 0; k < input_length; k++){
                  output_movements_angle[k] = buffer_angle[k];
              }

          }else{
            for(int l = 0; l < input_length; l++){
              output_movements_angle[l] = movement_matrix[c][l];
            }
          }
          t=0;
      }

      //Filter away outliers
      if(output_movements_angle[t] < out_head_angle[0] + outlier_limit_angle && 
      output_movements_angle[t] > out_head_angle[0]-outlier_limit_angle &&
      (out_head_angle[0] - output_movements_angle[t] < -limit_angle ||
      out_head_angle[0] - output_movements_angle[t] > limit_angle))
      {
            out_head_angle[0] = output_movements_angle[t];
      }

      if(output_movements_rotation[t] < out_head_rotation[0] + outlier_limit_rotation && 
      output_movements_rotation[t] > out_head_rotation[0]-outlier_limit_rotation &&
      (out_head_rotation[0] - output_movements_rotation[t] < -limit_rotation ||
      out_head_rotation[0] - output_movements_rotation[t] > limit_rotation))
      {
            out_head_rotation[0] = output_movements_rotation[t];
      } 

//      printf("\n HEAD ANGLE OUT: %f \n",out_head_angle[0]);
//      printf("\n HEAD ROTATION OUT: %f \n",out_head_rotation[0]);
   }

    reset_array(angles_out, 19);
    angles_out[1] = out_head_angle[0]-10; 
    angles_out[2] = out_head_rotation[0];

    movement_timer = ( std::clock() - start_time ) / (double) CLOCKS_PER_SEC;
    
    if(movement_type == 1){
      
     //hand out
    angles_out[5] = 74;
    angles_out[6] = -83;
    angles_out[7] = 11;
    angles_out[8] = -53;
    angles_out[9] = -160;
    } else if (movement_type == 2){
    if (movement_timer < 0.5 || 
    (movement_timer < 1.5 && movement_timer > 1.0) ||
    (movement_timer < 2.5 && movement_timer > 2.0) ||
    (movement_timer < 3.5 && movement_timer > 3.0) ||
    (movement_timer < 4.5 && movement_timer > 4.0)){
        //default
      angles_out[5] = 89;
      angles_out[6] = -86;
      angles_out[7] = 34;
      angles_out[8] = -9;
      angles_out[9] = -119;
      } else{
        //fippla
      angles_out[5] = 89;
      angles_out[6] = -76;
      angles_out[7] = 34;
      angles_out[8] = -9;
      angles_out[9] = -119;
      }
    } else if (movement_type == 3){
      if (movement_timer < 1){
        //intermediate
        angles_out[5] = 70;
        angles_out[6] = -45;
        angles_out[7] = -4;
        angles_out[8] = -80;
        angles_out[9] = -80;
      } else{
        //Hand ansikte
         angles_out[5] = 51;
         angles_out[6] = -66;
         angles_out[7] = 46;
         angles_out[8] = -122;
         angles_out[9] = -89;
      }
    } else if (movement_type == 4){
      //default
      angles_out[5] = 89;
      angles_out[6] = -86;
      angles_out[7] = 34;
      angles_out[8] = -9;
      angles_out[9] = -119;
    }

    if (movement_timer > 5){
      movement_type = rand()%(4-1 + 1) + 1;
      start_time = std::clock();
    }    
  
  //print_array("Angles", angles_out, 19);
}



// Install the module. This code is executed during start-up.
static InitClass init("MimicHead", &MimicHead::Create, "Source/UserModules/Projects/StudentProjects/RobotMimicry/MimicHead/");
