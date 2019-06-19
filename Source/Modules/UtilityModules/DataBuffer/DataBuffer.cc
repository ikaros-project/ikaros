//
//	DataBuffer.cc		This file is a part of the IKAROS project
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

#include "DataBuffer.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;

DataBuffer::DataBuffer(Parameter * p):
    Module(p)
{
    Bind(size, "size");
    AddOutput("OUTPUT");
    AddInput("INPUT");
}

void
DataBuffer::SetSizes()
{
    int sx = GetInputSize("INPUT");
    SetOutputSize("OUTPUT", sx, size);
}

void
DataBuffer::Init()
{
    // Bind(size, "size");
    Bind(policy, "update_policy");
	Bind(debugmode, "debug");    

    io(input_array, input_array_size,"INPUT");
    
    io(output_array, output_array_size_x, output_array_size_y, "OUTPUT");

    internal_array = create_matrix(input_array_size, size);
}



DataBuffer::~DataBuffer()
{
    // Destroy data structures that you allocated in Init.
    destroy_matrix(internal_array);
}



void
DataBuffer::Tick()
{
	if(debugmode)
	{
		// print out debug info
	}
    switch (policy)
    {
    case eCircular:
        // TODO
        //break;
    case eQueue:
        // TODO
        //break;
    case eRandom:
    default:
        // copy input into random pos
        int pos = random_int(0, size);
        copy_array(internal_array[pos], input_array, input_array_size);
        break;
    }
    copy_matrix(output_array, internal_array, input_array_size, size);
}



// Install the module. This code is executed during start-up.

static InitClass init("DataBuffer", &DataBuffer::Create, "Source/Modules/UtilityModules/DataBuffer/");


