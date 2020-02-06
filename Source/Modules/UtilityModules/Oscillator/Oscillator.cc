//
//	Oscillator.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2019 Christian Balkenius
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

#include "Oscillator.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;


void
Oscillator::Init()
{
    input = GetInputArray("INPUT");
    output = GetOutputArray("OUTPUT");

    Bind(input_gain, "input_gain");
    Bind(alpha, "alpha");
}



void
Oscillator::Tick()
{
	
}



// Install the module. This code is executed during start-up.

static InitClass init("Oscillator", &Oscillator::Create, "Source/Modules/UtilityModules/Oscillator/");


