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
    Bind(size, "size");
	Bind(debugmode, "debug");    
    Bind(light, "light");
    Bind(dark, "dark");

    input_array = GetInputArray("INPUT");
    io(rotate, "ROTATE");
    //input_array_size = GetInputSize("INPUT");

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
    shuteye(output_array, input_array[0], size);
	if(debugmode)
	{
		// print out debug info
	}
}

void 
EyelidAnimation::shuteye(float* r, float a, int sz){
    // a: 0-1 degree of shut
    // diodes indexed clockwise from 0 degrees; 0-sz/2 bottom; sz/2-sz top
    // float retval = new color[sz];
    // TODO: generalize to n number of LEDs
    float *retval = r;

    // translate degree to colors for shuteye
    //float dark = 0.2; //#666666;
    //float light = 0.9; //#bbbbbb;
    for(int i=0; i < sz; i++) retval[i] = light;
    // 0 - all light
    if(a >= 0 && a < 0.2) return; // retval;
    else if(a >= 0.2 && a < 0.4){
        // 0.25 - 2,3 and 8,9 dark
        int ix[] = {2,3,8,9};
        int ixlength = 4;
        addmod(ix, ix, rotate[0], sz, ixlength);
        for (int i=0; i<ixlength; i++) retval[ix[i]] = dark;
    }
    else if(a >= 0.4 && a < 0.6){
        // 0.5 - 1234 and 789a
        int ix[] = {1,2,3,4,7,8,9,10};
        int ixlength = 8;
        addmod(ix, ix, rotate[0], sz, ixlength);
        for (int i=0; i<ixlength; i++) retval[ix[i]] = dark;
    }
    else if(a >= 0.6 && a < 0.8){
        // 0.75 - 012345 and 789a
        int ix[] = {0,1,2,3,4,5, 7,8,9,10}; // 5, 10 light
        int ixlength = 10;
        addmod(ix, ix, rotate[0], sz, ixlength);
        for (int i=0; i<ixlength; i++) retval[ix[i]] = dark;
    }
    else if (a >= 0.8){
        // 1 - all dark
        for (int i=0; i<sz; i++) retval[i] = dark;
    }
    //return retval;

}

int* 
EyelidAnimation::addmod(int *r, int *a, int inc, int mod, int sz)
{
    for(int i=0; i< sz; i++){
        r[i] = (a[i] + inc) % mod;
        if (r[i] < 0) r[i] = mod + r[i];
    }
    return r;
}
// Install the module. This code is executed during start-up.

static InitClass init("EyelidAnimation", &EyelidAnimation::Create, "Source/Modules/RobotModules/EyelidAnimation/");


