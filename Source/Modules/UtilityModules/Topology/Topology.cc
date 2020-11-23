//
//	Topology.cc		This file is a part of the IKAROS project
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

#include "Topology.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;

const int cEMPTY = 0;
const int cONE_TO_ONE = 1;
const int cNEAREST_NEIGHBOR_1D = 2;
const int cNEAREST_NEIGHBOR_2D = 3;
const int cNEAREST_NEIGHBOR_3D = 4;
const int cNEAREST_NEIGHBOR_4D = 5;
const int cCIRCLE = 6;
const int cDONUT = 7;

Topology::Topology(Parameter * p) : Module(p)
{
    Bind(input_array_size_x, "size_x");
    Bind(input_array_size_y, "size_y");

    io(input_array_x, "SIZE_X");
    io(input_array_y, "SIZE_Y");

    if(input_array_x)
        input_array_size_x = GetInputSize("SIZE_X");
    if(input_array_y)
        input_array_size_y = GetInputSize("SIZE_Y");

    AddOutput("OUTPUT");
}

void 
Topology::SetSizes()
{
    SetOutputSize("OUTPUT", input_array_size_x, input_array_size_y);
}

void
Topology::Init()
{
    Bind(type, "type");
	Bind(debugmode, "debug");    

    switch (type)
    {
    case cEMPTY:
        // do nothing, filled with  zeros
        break;
    case cONE_TO_ONE:
        SetOneToOneTop();
        break;
    case cNEAREST_NEIGHBOR_1D:
    break;
    case cNEAREST_NEIGHBOR_2D:
    break;
    case cNEAREST_NEIGHBOR_3D:
    break;
    case cNEAREST_NEIGHBOR_4D:
    break;
    case cCIRCLE:
    break;
    case cDONUT:
    default:
    break;
    }

    // internal_array = create_array(10);
}



Topology::~Topology()
{
    // Destroy data structures that you allocated in Init.
    // destroy_array(internal_array);
}



void
Topology::Tick()
{
	if(debugmode)
	{
		// print out debug info
	}
}

void
Topology::SetOneToOne()
{
    // check that sizes x=y
    if(input_array_size_y==input_array_size_y)
    {
        for(int j=0; j<input_array_size_y; j++)
            for(int j=0; j<input_array_x; i++)
                output_matrix[j][i]=1.f;
    }
}

void
Topology::SetNearestNeighbor(int adim)
{
    // 1 dim
    
    // 2 dim

    // 3 dim

    // 4 dim    
}

void
Topology::SetCircle()
{

}

void
Topology::SetDonut()
{

}


// Install the module. This code is executed during start-up.

static InitClass init("Topology", &Topology::Create, "Source/Modules/UtilityModules/Topology/");


