//
//	SumMatrix.cc		This file is a part of the IKAROS project
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

#include "SumMatrix.h"
#include <string.h>
// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;

const char *c_all = "all";
const char *c_col = "column";
const char *c_row = "row";

void
SumMatrix::SetSizes()
{
    int sx = GetInputSizeX("INPUT");
    int sy = GetInputSizeY("INPUT");
    const char* dir = GetValue("direction");
    int outputsize = 1;
    if(strcmp(dir, c_all)==0)
    {
        direction = eAll;
    }
    else if(strcmp(dir, c_col)==0)
    {
        direction = eCol;
        outputsize = sx;
    }
    else if(strcmp(dir, c_row)==0)
    {
        direction = eRow;
        outputsize = sy;
    }
    else
    {
        // TODO notify error
    } 
    SetOutputSize("OUTPUT", outputsize);
}

void
SumMatrix::Init()
{
	Bind(debugmode, "debug");    

    input_matrix = GetInputMatrix("INPUT");
    input_matrix_size_x = GetInputSizeX("INPUT");
    input_matrix_size_y = GetInputSizeY("INPUT");

    output_array = GetOutputArray("OUTPUT");
    output_array_size = GetOutputSize("OUTPUT");


    internal_array = create_array(10);
}



SumMatrix::~SumMatrix()
{
    // Destroy data structures that you allocated in Init.
    destroy_array(internal_array);
}



void
SumMatrix::Tick()
{
    reset_array(output_array, output_array_size);
    // sum across the 
    switch(direction)
    {
        case eCol:
            for (int i = 0; i < input_matrix_size_x; ++i)
                for (int j = 0; j < input_matrix_size_y; ++j)
                    output_array[i] += input_matrix[j][i];
            break;
        case eRow:
            for (int j = 0; j < input_matrix_size_y; ++j)
                for (int i = 0; i < input_matrix_size_x; ++i)
                    output_array[j] += input_matrix[j][i]; 
            break;
        default:
            // sum across all
            for (int i = 0; i < input_matrix_size_x; ++i)
                for (int j = 0; j < input_matrix_size_y; ++j)
                    output_array[0] += input_matrix[i][j];

    }
	if(debugmode)
	{
		// print out debug info
	}
}



// Install the module. This code is executed during start-up.

static InitClass init("SumMatrix", &SumMatrix::Create, "Source/Modules/UtilityModules/SumMatrix/");


