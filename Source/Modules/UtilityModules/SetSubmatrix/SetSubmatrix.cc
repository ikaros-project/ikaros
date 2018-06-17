//
//	SetSubmatrix.cc		This file is a part of the IKAROS project
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

#include "SetSubmatrix.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;


void
SetSubmatrix::Init()
{
    Bind(offset_x, "offset_x");
    Bind(offset_y, "offset_y");
	Bind(debugmode, "debug");    

    source_matrix = GetInputMatrix("SOURCE");
    destination_matrix = GetInputMatrix("DESTINATION");
    output_matrix = GetOutputMatrix("OUTPUT");
    
    source_size_x = GetInputSizeX("SOURCE");
    source_size_y = GetInputSizeY("SOURCE");
    destination_size_x = GetInputSizeX("DESTINATION");
    destination_size_y = GetInputSizeY("DESTINATION");

    // TODO assert correct sizes
    if (source_size_x > destination_size_x || source_size_y > destination_size_y)
    {
        Notify(msg_fatal_error, "Source is larger than destination.\n");
        return;
    }
    if ((source_size_x + offset_x) > destination_size_x || (source_size_y + offset_y) > destination_size_y)
    {
        Notify(msg_fatal_error, "Source + offset is larger than destination.\n");
        return;   
    }
    // internal_array = create_array(10);
}



SetSubmatrix::~SetSubmatrix()
{
    // Destroy data structures that you allocated in Init.
    // destroy_array(internal_array);
}



void
SetSubmatrix::Tick()
{
    // copy destination to output
    copy_matrix(output_matrix, 
        destination_matrix, destination_size_x, destination_size_y);
     set_submatrix(output_matrix[0], destination_size_x, 
         source_matrix[0], source_size_y, source_size_x,
         offset_y, offset_x);
    // set_submatrix(source_matrix[0], source_size_x, 
    //     output_matrix[0], destination_size_y, destination_size_x,
    //     offset_y, offset_x);
	if(debugmode)
	{
		// print out debug info
	}
}



// Install the module. This code is executed during start-up.

static InitClass init("SetSubmatrix", &SetSubmatrix::Create, "Source/Modules/UtilityModules/SetSubmatrix/");


