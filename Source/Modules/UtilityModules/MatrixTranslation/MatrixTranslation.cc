//
//	MatrixTranslation.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2015 Trond Arild Tjöstheim
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

#include "MatrixTranslation.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;


void
MatrixTranslation::Init()
{
    
	Bind(debugmode, "debug");    

    input_matrix = GetInputMatrix("INPUT");
    input_matrix_size_x = GetInputSizeX("INPUT");
    input_matrix_size_y = GetInputSizeY("INPUT");
    x_trans = GetInputArray("X");
    y_trans = GetInputArray("Y");   

    output_matrix = GetOutputMatrix("OUTPUT");
    output_matrix_size_x = GetOutputSizeX("OUTPUT");
    output_matrix_size_y = GetOutputSizeY("OUTPUT");
    h_eye(trans_mat);


    
}



MatrixTranslation::~MatrixTranslation()
{
    // Destroy data structures that you allocated in Init.
    
}



void
MatrixTranslation::Tick()
{
	// get x and y translations
    int src_i = 0;
    int src_j = 0;
    h_translation_matrix(trans_mat, -x_trans[0], -y_trans[0], 0);
    reset_matrix(output_matrix, input_matrix_size_x, input_matrix_size_y);
    for (int i=0; i<input_matrix_size_y; i++) {
        for (int j=0; j<input_matrix_size_x; j++) {
            element[0] = (float)j;
            element[1] = (float)i;
            element[3] = 1.f;
            h_multiply_v(tmp, trans_mat, element);
            src_i = lround(tmp[1]);
            src_j = lround(tmp[0]);
            if(src_i>=0 && src_i<output_matrix_size_y &&
               src_j>=0 && src_j<output_matrix_size_x){
                output_matrix[i][j] = input_matrix[src_i][src_j];
                // printf("input assigned: %f", input_matrix[src_i][src_j]);
            }
            if(debugmode)
            {
                printf("dst_i=%i, dst_j=%i, src_i=%i, src_j=%i - ",i, j, src_i, src_j);
                printf("output at %i, %i = %f\n", i, j, output_matrix[i][j]);
            }
        }
    }

    if(debugmode)
	{
		// print out debug info
        printf("translation: %f, %f\n", x_trans[0], y_trans[0]);
	}
}



// Install the module. This code is executed during start-up.

static InitClass init("MatrixTranslation", &MatrixTranslation::Create, "Source/Modules/UtilityModules/MatrixTranslation/");


