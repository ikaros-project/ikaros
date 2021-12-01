//
//	MotorControl.cc		This file is a part of the IKAROS project
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

#include "MotorControl.h"

// use the ikaros namespace to access the math library
// this is preferred to using <cmath>

using namespace ikaros;


void
MotorControl::Init()
{
      io(left_eye_input, left_eye_input_size, "LEFT_EYE");
      io(right_eye_input, right_eye_input_size, "RIGHT_EYE");
   
      io(head_tilt_output, head_tilt_output_size, "HEAD_TILT");
      io(head_pan_output, head_pan_output_size, "HEAD_PAN");
      io(left_eye_pan_output, left_eye_pan_output_size, "LEFT_EYE_PAN");
      io(right_eye_pan_output, right_eye_pan_output_size, "RIGHT_EYE_PAN");

}


MotorControl::~MotorControl()
{
}



void
MotorControl::Tick()
{
    

    // Someting cool here!!
    head_tilt_output[0] = -left_eye_input[1];
    head_pan_output[0] = left_eye_input[0];
    left_eye_pan_output[0] = -left_eye_input[0];
    left_eye_pan_output[0] = -left_eye_input[0];
}



// Install the module. This code is executed during start-up.

static InitClass init("MotorControl", &MotorControl::Create, "Source/UserModules/MotorControl/");


