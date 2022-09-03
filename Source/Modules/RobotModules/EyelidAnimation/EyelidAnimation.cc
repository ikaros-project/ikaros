//
//	EyelidAnimation.cc		This file is a part of the IKAROS project
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

#include "EyelidAnimation.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;


void
EyelidAnimation::Init()
{
    Bind(parameter, "size");
	Bind(debugmode, "debug");    

    input_array = GetInputArray("INPUT");
    input_array_size = GetInputSize("INPUT");

    output_array = GetOutputArray("OUTPUT");
    output_array_size = GetOutputSize("OUTPUT");


    internal_array = create_array(10);
}



EyelidAnimation::~EyelidAnimation()
{
    // Destroy data structures that you allocated in Init.
    destroy_array(internal_array);
}



void
EyelidAnimation::Tick()
{
	if(debugmode)
	{
		// print out debug info
	}
}

void shuteye(float* r, float a, int sz){
  // a: 0-1 degree of shut
  // diodes indexed clockwise from 0 degrees; 0-sz/2 bottom; sz/2-sz top
  float retval = new color[sz];
  
  // translate degree to colors for shuteye
  color dark = #666666;
  color light = #bbbbbb;
  for(int i=0; i < sz; i++) retval[i] = light;
  // 0 - all light
  if(a >= 0 && a < 0.2) return retval;
  else if(a >= 0.2 && a < 0.4){
    // 0.25 - 2,3 and 8,9 dark
    int[] ix = {1,2,7,8};
    for (int i=0; i<ix.length; i++) retval[ix[i]] = dark;
  }
  else if(a >= 0.4 && a < 0.6){
    // 0.5 - 1234 and 789a
    int[] ix = {0,1,2,3,6,7,8,9};
    for (int i=0; i<ix.length; i++) retval[ix[i]] = dark;
  }
  else if(a >= 0.6 && a < 0.8){
    // 0.75 - 012345 and 789a
    int[] ix = {11,0,1,2,3,4, 6,7,8,9};
    for (int i=0; i<ix.length; i++) retval[ix[i]] = dark;
  }
  else if (a >= 0.8){
    // 1 - all dark
    for (int i=0; i<sz; i++) retval[i] = dark;
  }
  return retval;
  
}

// Install the module. This code is executed during start-up.

static InitClass init("EyelidAnimation", &EyelidAnimation::Create, "Source/Modules/RobotModules/EyelidAnimation/");


