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

#include "HeadTracking.h"

// use the ikaros namespace to access the math library
// this is preferred to using <cmath>

using namespace ikaros;

void
HeadTracking::Init()
{
  Bind(mask_left, "mask_left");
  Bind(mask_right, "mask_right");

  size_x	 = GetInputSizeX("INPUT");
  size_y	 = GetInputSizeY("INPUT");

  object      = GetInputArray("OBJECT");
  input       = GetInputMatrix("INPUT");
  output		= GetOutputMatrix("OUTPUT");

  out_head_degrees = GetOutputArray("HEAD");

  target_degree = 0;
  current_degree = 0;

  t = 0;
  sum_head = 0;
}

HeadTracking::~HeadTracking()
{
    // Destroy data structures that you allocated in Init.
    // Do NOT destroy data structures that you got from the
    // kernel with GetInputArray, GetInputMatrix etc.
}

void
HeadTracking::Tick()
{

  int a = int(mask_left*size_x);
  int b = int(mask_right*size_x);

  float x_head_max = 0;
  float x_head_min = 100000;

  float y_head_max = 0;
  float y_head_min = 100000;
  int head_cut = 100;

  reset_matrix(output, size_x, size_y);

   for(int j=0; j<size_y; j++){
      for(int i=a; i<b; i++){
        if(object[0] <= input[j][i] && input[j][i] <= object[1]){
                head_cut = j + 125;
                goto foundHead;
          }
      }
  }
  foundHead:

  for(int i=a; i<b; i++)
  {
      for(int j=0; j<size_y; j++)
      {
          if(object[0] <= input[j][i] && input[j][i] <= object[1]){
                if(h_matrix_is_valid(input[j]))
                {
                    //Head
                    if(j < head_cut){

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

                      output[j][i] = 0.5;
                    }

                }else{
                  //  printf("NOT VALID MATRIX");
                }
          }
      }
  }

  float avg_head_x = (x_head_max + x_head_min) / 2;
  float avg_head_y = (y_head_max + y_head_min) / 2;

  float head_degree = 0;

  //Caclulate upper and lower head
  float upper_head_average = (avg_head_y + y_head_min) / 2;
  float lower_head_average = (avg_head_y + y_head_max) / 2;

  int upper_head_average_int = int(upper_head_average);
  int lower_head_average_int = int(lower_head_average);
  int avg_head_x_int = int(avg_head_x);

  if(upper_head_average_int < size_y && lower_head_average_int < size_y && avg_head_x_int < size_x){

    for(int k=0; k<14; k++)
    {
      output[upper_head_average_int + k][avg_head_x_int + k] = 1;
      output[lower_head_average_int + k][avg_head_x_int + k] = 1;
    }

    float z_upper = input[upper_head_average_int][avg_head_x_int];
    float z_lower = input[lower_head_average_int][avg_head_x_int];

    float y_diff = upper_head_average - lower_head_average;

  //  float target_degree = (atan(x_diff / y_diff) * 180) / 3.14;
  //  printf("Degrees: %f \n", input[upper_head_average_int][avg_head_x_int]);
  }

/*

  if(avg_head_x == 50000 || avg_head_y == 50000){
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
  }


  if(mask_left == 0){
      out_body_degrees[0] = target_degree;
  }else{
      out_body_degrees[0] = -target_degree;
  }
  */
//  out_head_degrees[0] = -out_body_degrees[0] + head_degree;
  out_head_degrees[0] = 0;

  /*
  if(out_head_degrees[0] != 0){
      sum_head = sum_head + out_head_degrees[0];
    //  printf("Average head %f \n", sum_head / t);
  }

  if(out_body_degrees[0] != 0){
      sum_body = sum_body + out_body_degrees[0];
    //  printf("Average body %f \n", sum_body / t);
  }
  */
}


// Install the module. This code is executed during start-up.
static InitClass init("HeadTracking", &HeadTracking::Create, "Source/UserModules/HeadTracking/");
