//
//	Transform.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2014 Christian Balkenius
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


#include "Transform.h"

using namespace ikaros;


void
Transform::SetSizes()
{
    int sy1 = GetInputSizeY("MATRIX_1");
    int sy2 = GetInputSizeY("MATRIX_2");

    if(sy1 == unknown_size || sy2 == unknown_size)
        return;
        
    int sy = max(sy1, sy2);

    SetOutputSize("MATRIX", 16, sy);
    SetOutputSize("OBJECT_ID", sy);
    SetOutputSize("FRAME_ID", sy);
}



void
Transform::Init()
{
    Bind(invert_1, "invert_1");
    Bind(invert_2, "invert_2");

    matrix_1 = GetInputMatrix("MATRIX_1");
    object_id_1 = GetInputArray("OBJECT_ID_1");
    frame_id_1 = GetInputArray("FRAME_ID_1");
    
    matrix_2 = GetInputMatrix("MATRIX_2");
    object_id_2 = GetInputArray("OBJECT_ID_2");
    frame_id_2 = GetInputArray("FRAME_ID_2");
    
    matrix = GetOutputMatrix("MATRIX");
    object_id = GetOutputArray("OBJECT_ID");
    frame_id = GetOutputArray("FRAME_ID");
    
    size_x = GetInputSizeX("MATRIX_1");
    size_y = GetOutputSizeY("MATRIX");
    size_y_1 = GetInputSizeY("MATRIX_1");
    size_y_2 = GetInputSizeY("MATRIX_2");
}



void
Transform::Tick()
{
    reset_matrix(matrix, 16, size_y);
    reset_array(object_id, size_y);
    reset_array(frame_id, size_y);

    h_matrix a, b;

    int k = 0; // target location in output
    float o1, f1, o2, f2;

    for(int i=0; i<size_y_1; i++)
        if(h_matrix_is_valid(matrix_1[i]))
        {
            for(int j=0; j<size_y_2; j++)
            {
                o1 = (invert_1 ? frame_id_1[i] : object_id_1[i]);
                f1 = (invert_1 ? object_id_1[i] : frame_id_1[i]);

                o2 = (invert_2 ? frame_id_2[j] : object_id_2[j]);
                f2 = (invert_2 ? object_id_2[j] : frame_id_2[j]);

                if(h_matrix_is_valid(matrix_2[j]) && o1 == f2)  // matching rule
                {
                    if(invert_1)
                        h_inv(a, matrix_1[i]);
                     else
                        h_copy(a, matrix_1[i]);

                    if(invert_2)
                        h_inv(b, matrix_2[j]);
                    else
                        h_copy(b, matrix_2[j]);

                    h_multiply(matrix[k], a, b);
                    object_id[k] = o2;
                    frame_id[k] = f1;
                    
                    k++;
                }
            }
        }
}



static InitClass init("Transform", &Transform::Create, "Source/Modules/UtilityModules/Transform/");

