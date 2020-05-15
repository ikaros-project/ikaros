//
//	Ones.cc		This file is a part of the IKAROS project
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

#include "Ones.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;

void
Ones::Init()
{
    output_matrix = GetOutputMatrix("OUTPUT");
    output_matrix_size_x = GetOutputSizeX("OUTPUT");
    output_matrix_size_y = GetOutputSizeY("OUTPUT");

    Bind(value, "value");

    set_matrix(output_matrix, value, output_matrix_size_x, output_matrix_size_y);
}


Ones::~Ones()
{
    // Destroy data structures that you allocated in Init.
    // destroy_array(internal_array);
}


void
Ones::Tick()
{
	
}


// Install the module. This code is executed during start-up.

static InitClass init("Ones", &Ones::Create, "Source/Modules/UtilityModules/Ones/");


