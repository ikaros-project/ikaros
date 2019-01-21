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

#include "MaximumToRobot.h"

// use the ikaros namespace to access the math library
// this is preferred to using <cmath>

using namespace ikaros;

static inline void depth_to_sensor_coords(float & x, float & y, float & z, double xRes, double yRes, double fXToZ, double fYToZ)
{
    // compensate for perspective

    float tx = (float)((x / xRes - 0.5) * z * fXToZ);
    float ty = (float)((0.5 - y / yRes) * z * fYToZ);
    float tz = z;

    // shift to sensor coordinate system
    // x is pointing forwards and y to the side; z is up

    x = tz;
    y = -tx;
    z = -ty;
}


void
MaximumToRobot::Init()
{
  Bind(mask_left, "mask_left");
  Bind(mask_right, "mask_right");

  size_x	 = GetInputSizeX("INPUT");
  size_y	 = GetInputSizeY("INPUT");

  object      = GetInputArray("OBJECT");
  input       = GetInputMatrix("INPUT");
  output		= GetOutputMatrix("OUTPUT");

  out_head_degrees = GetOutputArray("HEAD");
  out_body_degrees = GetOutputArray("BODY");

  target_degree = 0;
  current_degree = 0;

  t = 0;
  sum_head = 0;
  sum_body = 0;
}

MaximumToRobot::~MaximumToRobot()
{
    // Destroy data structures that you allocated in Init.
    // Do NOT destroy data structures that you got from the
    // kernel with GetInputArray, GetInputMatrix etc.
}



void
MaximumToRobot::Tick()
{

  int a = int(mask_left*size_x);
  int b = int(mask_right*size_x);

  float x_res = 640;
  float y_res = 480;

  float fov_h = 1.1;
  float fov_v = 1.1;

  float x_head_max = 0;
  float x_head_min = 100000;

  float x_body_max = 0;
  float x_body_min = 100000;

  float y_head_max = 0;
  float y_head_min = 100000;

  float y_body_max = 0;
  float y_body_min = 100000;

  // recalculate if parameter have changed
  double fXToZ = tan(fov_h/2)*2;
  double fYToZ = tan(fov_v/2)*2;

  reset_matrix(output, size_x, size_y);

  for(int i=a; i<b; i++)
  {
      for(int j=0; j<size_y; j++)
      {
          if(object[0] <= input[j][i] && input[j][i] <= object[1]){
                if(h_matrix_is_valid(input[j]))
                {
                /*  float x = h_get_x(input[j]);
                  float y = h_get_y(input[j]);
                  float z = h_get_z(input[j]);

                  depth_to_sensor_coords(x, y, z, x_res, y_res, fXToZ, fYToZ);  */

                    //Head
                    if(j < 225){

                      if(i > x_head_max){
                        x_head_max = i;
                      }
                      if(i < x_head_min){
                        x_head_min = i;
                      }

                      if(j > y_head_max){
                        y_head_max = j;
                      }
                      if(j < y_head_min){
                        y_head_min = j;
                      }

                      output[j][i] = 1;

                     //Body
                    }else{

                      if(i > x_body_max){
                        x_body_max = i;
                      }
                      if(i < x_body_min){
                        x_body_min = i;
                      }

                      if(j > y_body_max){
                        y_body_max = j;
                      }
                      if(j < y_body_min){
                        y_body_min = j;
                      }
                      output[j][i] = 0.2;
                    }

                }else{
                    printf("NOT VALID MATRIX");
                }
          }
      }
  }

  float avg_body_x = (x_body_max + x_body_min) / 2;
  float avg_body_y = (y_body_max + y_body_min) / 2;

  float avg_head_x = (x_head_max + x_head_min) / 2;
  float avg_head_y = (y_head_max + y_head_min) / 2;

  float x_diff = avg_body_x - avg_head_x;
  float y_diff = avg_body_y - avg_head_y;

  float head_degree = 0;

  //Caclulate upper and lower head
  float upper_head_average = (avg_head_y + y_head_min) / 2;
  float lower_head_average = (avg_head_y + y_head_max) / 2;

  float upper_head_x_max = 0;
  float lower_head_x_max = 0;

  int upper_head_average_int = int(upper_head_average);
  int lower_head_average_int = int(lower_head_average);

  if(avg_body_x == 50000 || avg_body_y == 50000 || avg_head_x == 50000 || avg_head_y == 50000){
    //  printf("Out of Bounds \n");
  }else{

      for(int i=a; i<b; i++)
      {
        if(h_matrix_is_valid(input[upper_head_average_int]))
        {
            if(mask_left == 0){
                if(object[0] <= input[upper_head_average_int][i] && input[upper_head_average_int][i] <= object[1]){
                        upper_head_x_max = i;
                }
            }else{
              if(object[0] <= input[upper_head_average_int][i] && input[upper_head_average_int][i] <= object[1] && upper_head_x_max == 0){
                      upper_head_x_max = i;
              }
            }
          }

          if(h_matrix_is_valid(input[lower_head_average_int]))
          {

            if(mask_left == 0){
              if(object[0] <= input[lower_head_average_int][i] && input[lower_head_average_int][i] <= object[1]){
                      //Set to last value in row where head average is
                      lower_head_x_max = i;
              }
            }else{
              if(object[0] <= input[lower_head_average_int][i] && input[lower_head_average_int][i] <= object[1] && lower_head_x_max == 0){
                      //Set to first value in row where head average is
                      lower_head_x_max = i;
              }
            }
          }
      }
      target_degree = (atan(x_diff / y_diff) * 180) / 3.14;

      float x_head_diff = lower_head_x_max - upper_head_x_max;
      float y_head_diff = lower_head_average - upper_head_average;

      head_degree = 1.25 * (atan(x_head_diff / y_head_diff) * 180) / 3.14;
    //printf("Head Degree %f  \n",head_degree);
  }


  if(mask_left == 0){
      out_body_degrees[0] = target_degree;
  }else{
      out_body_degrees[0] = -target_degree;
  }

  out_head_degrees[0] = -out_body_degrees[0] + head_degree;

  t++;

  if(out_head_degrees[0] != 0){
      sum_head = sum_head + out_head_degrees[0];
    //  printf("Average head %f \n", sum_head / t);
  }

  if(out_body_degrees[0] != 0){
      sum_body = sum_body + out_body_degrees[0];
    //  printf("Average body %f \n", sum_body / t);
  }
}



// Install the module. This code is executed during start-up.

static InitClass init("MaximumToRobot", &MaximumToRobot::Create, "Source/UserModules/MaximumToRobot/");
