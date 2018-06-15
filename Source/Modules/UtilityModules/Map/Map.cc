//
//	Map.cc		This file is a part of the IKAROS project
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

#include "Map.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;


void
Map::Init()
{
    // To get the parameters from the IKC file, use the Bind
    // function for each parameter. The parameters are initialized
    // from the IKC and can optionally be changed from the
    // user interface while Ikaros is running. If the parameter is not
    // set, the default value will be used instead.
    Bind(low_source, "low_source");  
    Bind(high_source, "high_source");
    Bind(low_target, "low_target");  
    Bind(high_target, "high_target");
    
    // This is were we get pointers to the inputs and outputs

    // Get a pointer to the input INPUT1 and its size which we set
    // to 10 above
    // It does not matter whether a matrix of array is connected
    // to the inputs. We will treat it an array in this module
    // anyway.

    input_array = GetInputArray("INPUT");
    input_array_size = GetInputSize("INPUT");

    // Get pointer to a matrix and treat it as a matrix. If an array is
    // connected to this input, size_y will be 1.

    
    // Do the same for the outputs

    output_array = GetOutputArray("OUTPUT");

    // Allocate some data structures to use internaly
    // in the module

    // Create an array with ten elements
    // To access the array use internal_array[i].


    // Create a matrix with the same size as INPUT2
    // IMPORTANT: For the matrix the sizes are given as X, Y
    // which is the OPPOSITE of ROW, COLUMN.


    // To acces the matrix use internal_matrix[y][x].
    //
    // IMPORTANT: y is the first index and x the second,
    //
    // It is also possible to use the new operator to
    // create arrays, but create_array and create_matix
    // should be used to make sure that memeory is
    // allocated in a way that is suitable for the math
    // library and fast copying operations.
}



Map::~Map()
{
    // Destroy data structures that you allocated in Init.


    // Do NOT destroy data structures that you got from the
    // kernel with GetInputArray, GetInputMatrix etc.
}



void
Map::Tick()
{

    for (int i = 0; i < input_array_size; ++i)
    {
        output_array[i] = low_target + 
            (high_target - low_target) * (input_array[i] - low_source) 
            / (high_source - low_source);
    }

}



// Install the module. This code is executed during start-up.

static InitClass init("Map", &Map::Create, "Source/Modules/UtilityModules/Map/");


