//
//	MyModule.cc		This file is a part of the IKAROS project
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

#include "MyModule.h"

// use the ikaros namespace to access the math library
// this is preferred to using <cmath>

using namespace ikaros;


void
MyModule::Init()
{
    // To get the parameters from the IKC file, use the Bind
    // function for each parameter. The parameters are initialized
    // from the IKC and can optionally be changed from the
    // user interface while Ikaros is running. If the parameter is not
    // set, the default value will be used instead.
    
    Bind(float_parameter, "parameter1");
    Bind(int_parameter, "parameter2");
    
    // This is were we get pointers to the inputs and outputs

    // Get a pointer to the input INPUT1 and its size which we set
    // to 10 above
    // It does not matter whether a matrix of array is connected
    // to the inputs. We will treat it an array in this module
    // anyway.

    input_array = GetInputArray("INPUT1");
    input_array_size = GetInputSize("INPUT1");

    // Get pointer to a matrix and treat it as a matrix. If an array is
    // connected to this input, size_y will be 1.

    input_matrix = GetInputMatrix("INPUT2");
    input_matrix_size_x = GetInputSizeX("INPUT2");
    input_matrix_size_y = GetInputSizeY("INPUT2");

    // Do the same for the outputs

    output_array = GetOutputArray("OUTPUT1");
    output_array_size = GetOutputSize("OUTPUT1");

    output_matrix = GetOutputMatrix("OUTPUT2");
    output_matrix_size_x = GetOutputSizeX("OUTPUT2");
    output_matrix_size_y = GetOutputSizeY("OUTPUT2");

    // Allocate some data structures to use internaly
    // in the module

    // Create an array with ten elements
    // To access the array use internal_array[i].

    internal_array = create_array(10);

    // Create a matrix with the same size as INPUT2
    // IMPORTANT: For the matrix the sizes are given in order X, Y
    // in all functions including when the matrix is created
    // See: http://www.ikaros-project.org/articles/2007/datastructures/

    internal_matrix = create_matrix(input_matrix_size_x, input_matrix_size_y);

    // To acces the matrix use internal_matrix[y][x].
    //
    // IMPORTANT: y is the first index and x the second,
    //
    // It is also possible to use the new operator to
    // create arrays, but create_array and create_matix
    // should be used to make sure that memory is
    // allocated in a way that is suitable for the math
    // library and fast copying operations.
}



MyModule::~MyModule()
{
    // Destroy data structures that you allocated in Init.

    destroy_array(internal_array);
    destroy_matrix(internal_matrix);

    // Do NOT destroy data structures that you got from the
    // kernel with GetInputArray, GetInputMatrix etc.
}



void
MyModule::Tick()
{
    // This is where you implement your algorithm
    // to calculate the outputs from the inputs

    // This example makes a copy of the data on INPUT2 which is now
    // in input_matrix to internal_matrix
    // Arrays can be copied with copy_array
    // To clear an array or matrix use reset_array and reset_matrix

    copy_matrix(internal_matrix, input_matrix, input_matrix_size_x, input_matrix_size_y);

    // Calculate the output by iterating over the elements of the
    // output matrix. Note the order of the indices into the matrix.
    //
    // In most cases it is much faster to put the for-loops with
    // the row index (j) in the outer loop, because it will lead
    // to more efficient cache use.

    for (int j=0; j<output_matrix_size_y; j++)
        for (int i=0; i<output_matrix_size_x; i++)
            output_matrix[j][i] = 0.5;

    // Fill the output array with random values

    random(output_array, 0.0, 1.0, output_array_size);
}



// Install the module. This code is executed during start-up.

static InitClass init("MyModule", &MyModule::Create, "Source/UserModules/MyModule/");


