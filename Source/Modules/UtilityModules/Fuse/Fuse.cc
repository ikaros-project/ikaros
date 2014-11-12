//
//	Fuse.cc		This file is a part of the IKAROS project
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

#include "Fuse.h"

using namespace ikaros;

    // Sort a table and two index arrays according to ix1

    static void
    sort(float ** a, float * ix1, float * ix2, int sizey)
    {
        int i , j;

        h_matrix t;
        float t1;
        float t2;

        for(i = 1; i < sizey; i++)
        {
            h_copy(t, a[i]);
            t1 = ix1[i];
            t2 = ix2[i];

            for(j = i; j > 0 && t1 < ix1[j-1]; j--)
            {
                h_copy(a[j], a[j-1]);
                ix1[j] = ix1[j-1];
                ix2[j] = ix2[j-1];
            }

            h_copy(a[j], t);
            ix1[j] = t1;
            ix2[j] = t2;
        }
    }



Fuse::Fuse(Parameter * p):
    Module(p)
{
    no_of_inputs = GetIntValue("no_of_inputs");

    input_matrix_name  = new char * [no_of_inputs];
    input_object_id_name  = new char * [no_of_inputs];
    input_frame_id_name  = new char * [no_of_inputs];

    input_matrix      = new float ** [no_of_inputs];
    input_object_id   = new float *  [no_of_inputs];
    input_frame_id    = new float *  [no_of_inputs];

    input_size_y      = new int [no_of_inputs];

    for (int i=0; i<no_of_inputs; i++)
    {
        char nm[64];

        sprintf(nm, "MATRIX_%d", i+1);
        input_matrix_name[i] = create_string(nm);
        AddInput(input_matrix_name[i]);

        sprintf(nm, "OBJECT_ID_%d", i+1);
        input_object_id_name[i] = create_string(nm);
        AddInput(input_object_id_name[i]);

        sprintf(nm, "FRAME_ID_%d", i+1);
        input_frame_id_name[i] = create_string(nm);
        AddInput(input_frame_id_name[i]);
    }

    AddOutput("MATRIX");
    AddOutput("OBJECT_ID");
    AddOutput("FRAME_ID");
}



Fuse::~Fuse()
{
    delete [] input_matrix_name;
    delete [] input_object_id_name;
    delete [] input_frame_id_name;

    delete [] input_size_y;
}



// Try to calculate all size of the output
// which should be the sum of all inputs.
// Will fail until all inputs have known size

void
Fuse::SetSizes()
{
    int sy = 0;

    for(int i=0; i<no_of_inputs; i++)
    {
        input_size_y[i] = GetInputSizeY(input_matrix_name[i]);

        if(input_size_y[i] == unknown_size)
            return; // Not ready yet

        sy += input_size_y[i];
    }

    SetOutputSize("MATRIX", 16, sy);
    SetOutputSize("OBJECT_ID", sy);
    SetOutputSize("FRAME_ID", sy);

}



void
Fuse::Init()
{
   for(int i=0; i<no_of_inputs; i++)
    {
        input_matrix[i] = GetInputMatrix(input_matrix_name[i]);
        input_object_id[i] = GetInputArray(input_object_id_name[i]);
        input_frame_id[i] = GetInputArray(input_frame_id_name[i]);
    }

    matrix = GetOutputMatrix("MATRIX");
    object_id = GetOutputArray("OBJECT_ID");
    frame_id = GetOutputArray("FRAME_ID");

    output_size_y = GetOutputSizeY("MATRIX");
}



void
Fuse::Tick()
{
    // Fuse all inputs into one output table

    int r=0;
    for(int i=0; i<no_of_inputs; i++)
        for(int j=0; j<input_size_y[i]; j++)
            if(h_matrix_is_valid(input_matrix[i][j]))
            {
                h_copy(matrix[r], input_matrix[i][j]);
                object_id[r] = input_object_id[i][j];
                frame_id[r] = input_frame_id[i][j];

                r++;
            }

    for(int j=r; j<output_size_y; j++)
    {
        h_reset(matrix[j]);
        object_id[j] = 0;
        frame_id[j] = 0;
    }

    // Sort according to object_id (up to row r)

    sort(matrix, object_id, frame_id, r);

    // Fuse rows with identical ids

    int j=0; // source
    int k=0; // target

    while(j<output_size_y && h_matrix_is_valid(matrix[j]))
    {
        float cur_object_id = object_id[j];
        float cur_frame_id = frame_id[j];

        h_matrix a;
        h_reset(a);
        
        float n=0;

        while(j<output_size_y && object_id[j] == cur_object_id && frame_id[j] == cur_frame_id)
        {
            h_add(a, matrix[j]);
            n++;
            j++;
        }

        // Calculate average and orthogonalize

        if(n > 0)
        {
            h_multiply(a, 1/n);
            h_normalize_rotation(a);
            h_copy(matrix[k], a);
            object_id[k] = cur_object_id;
            frame_id[k] = cur_frame_id;
        }

        k++;
    }

    for(int z=k; z<output_size_y; z++)
    {
        h_reset(matrix[z]);
        object_id[z] = 0;
        frame_id[z] = 0;
    }
}



static InitClass init("Fuse", &Fuse::Create, "Source/Modules/UtilityModules/Fuse/");

