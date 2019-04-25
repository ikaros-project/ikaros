//
//	DecomposeTransform.cc		This file is a part of the IKAROS project
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

#include "DecomposeTransform.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;

void
DecomposeTransform::SetSizes()
{
    // expect hom. matrices with dimension 1x16 as input
    int sx = GetInputSizeX("INPUT");
    //int sy = GetInputSizeY("INPUT");
    int sy = sx / 16;


    SetOutputSize("TRANSLATION", 3, sy);
    SetOutputSize("ROTATION", 3, sy);
    SetOutputSize("SCALE", 3, sy);
}

void
DecomposeTransform::Init()
{
    Bind(use_degrees, "use_degrees");
	Bind(debugmode, "debug");    

    io(input_matrix, input_size_x, input_size_y,"INPUT");
    
    io(translation, output_size_x, output_size_y, "TRANSLATION");
    io(rotation, output_size_x, output_size_y, "ROTATION");
    io(scale, output_size_x, output_size_y,"SCALE");

    internal_array = create_array(10);
}



DecomposeTransform::~DecomposeTransform()
{
    // Destroy data structures that you allocated in Init.
    destroy_array(internal_array);
}



void
DecomposeTransform::Tick()
{
	if(debugmode)
	{
		// print out debug info
        print_matrix("input: ", input_matrix, input_size_x, input_size_y, 1);
        printf("input x= %i y= %i\n", input_size_x, input_size_y);

        print_matrix("transl: ", translation, output_size_x, output_size_y, 1);
        print_matrix("rotation: ", rotation, output_size_x, output_size_y, 1);
        print_matrix("scale: ", scale, output_size_x, output_size_y, 1);
	}
    for(int i = 0; i < output_size_y; i++)
    {
        h_matrix mat;
        int offset = i*16;
        float x, y, z;
        memcpy(mat, input_matrix[0] + offset, 16*sizeof(float));
        // get translation
        h_get_translation(mat, x, y, z);
        translation[i][0] = x;
        translation[i][1] = y;
        translation[i][2] = z;

        // get rotation
        h_get_euler_angles(mat, x, y, z);
        rotation[i][0] = x;
        rotation[i][1] = y;
        rotation[i][2] = z;

        // TODO if(use_degrees)

        // get scale
        h_get_scale(mat, x, y, z);
        scale[i][0] = x;
        scale[i][1] = y;
        scale[i][2] = z;

    }
    
}



// Install the module. This code is executed during start-up.

static InitClass init("DecomposeTransform", &DecomposeTransform::Create, "Source/Modules/UtilityModules/DecomposeTransform/");


