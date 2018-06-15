//
//	MatrixRotation.cc		This file is a part of the IKAROS project
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

#include "MatrixRotation.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;

const char *cDegFormat = "deg";
const char *cRadFormat = "rad";
const float torad = 0.0174532925;

void
MatrixRotation::Init()
{
    conv_factor = 1.0f;
    const char* format = GetValue("angle_format");
    if (strcmp(format, cDegFormat)==0) {
        conv_factor = torad;
    }
    
	Bind(debugmode, "debug");    

    input_matrix = GetInputMatrix("INPUT");
    input_matrix_size_x = GetInputSizeX("INPUT");
    input_matrix_size_y = GetInputSizeY("INPUT");
    angle = GetInputArray("ANGLE");
    output_matrix = GetOutputMatrix("OUTPUT");

    h_eye(rot_mat);
    h_translation_matrix(
                            trans_mat,
                           -input_matrix_size_x/2.0,
                           -input_matrix_size_y/2.0,
                            0);
    h_translation_matrix(
                           detrans_mat,
                           input_matrix_size_x/2.0,
                           input_matrix_size_y/2.0,
                           0);
}



MatrixRotation::~MatrixRotation()
{
    // Destroy data structures that you allocated in Init.
    // destroy_array(internal_array);
}



void
MatrixRotation::Tick()
{
    int src_i=0;
    int src_j=0;
    // update rotation matrix
    h_rotation_matrix(rot_mat, ikaros::Z, conv_factor*angle[0]);
    reset_matrix(output_matrix, input_matrix_size_x, input_matrix_size_y);
    // iterate over target matrix
    for (int i=0; i<input_matrix_size_y; i++) {
        for (int j=0; j<input_matrix_size_x; j++) {
            //vec[0][0] = (float)(j-sj/2); vec[1][0] = (float)(i-si/2);
            //print_matrix(vec, 4, 1);
            //ret = multiply(ret, rot, vec, 4, 1, 4);
            //print_matrix(ret, 4, 1);
            //int src_j = (int)roundf(ret[0][0]+si/2);
            //int src_i = (int)roundf(ret[1][0]+sj/2);
            //multiply(float ** r, float ** a, fl
            element[0] = (float)j;
            element[1] = (float)i;
            element[3] = 1.f;
            // detransl * rot * transl * element
            h_multiply_v(tmp1, trans_mat, element);
            h_multiply_v(tmp2, rot_mat, tmp1);
            h_multiply_v(tmp1, detrans_mat, tmp2);
            src_i = lround(tmp1[1]);
            src_j = lround(tmp1[0]);
            if(src_i>=0 && src_i<input_matrix_size_y &&
               src_j>=0 && src_j<input_matrix_size_x)
                output_matrix[i][j] = input_matrix[src_i][src_j];
            if(debugmode)
            {
                printf("src_i=%i, src_j=%i - ", src_i, src_j);
                printf("output at %i, %i = %f\n", i, j, output_matrix[i][j]);
            }
        }
    }
    
	if(debugmode)
	{
		// print out debug info
        //print_matrix("inputmatrix", input_matrix, input_matrix_size_x, input_matrix_size_y);
        printf("angle: %f\n", angle[0]);
        //printf("src_i=%i, src_j=%i", src_i, src_j);
	}
}



// Install the module. This code is executed during start-up.

static InitClass init("MatrixRotation", &MatrixRotation::Create, "Source/UserModules/MatrixRotation/");


