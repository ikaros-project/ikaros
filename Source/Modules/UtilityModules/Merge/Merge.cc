//
//	Merge.cc		This file is a part of the IKAROS project
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


#include "Merge.h"

using namespace ikaros;


float **
create_concatenated_matrix(float ** m1, float ** m2, int size_x, int size_y1, int size_y2)
{
    float ** m = create_matrix(size_x, size_y1+size_y2);
    int j = 0;
    
    for(int i=0; i<size_y1; i++)
        copy_array(m[j++], m1[i], size_x);
    
    for(int i=0; i<size_y2; i++)
        copy_array(m[j++], m2[i], size_x);
    
    return m;
}



void
Merge::Init()
{
    Bind(id_column, "id_column");
    
    input1 = GetInputMatrix("INPUT1");
    input2 = GetInputMatrix("INPUT2");
    
    output = GetOutputMatrix("OUTPUT");
    
    size_x = GetInputSizeX("INPUT2");
    size_y1 = GetInputSizeY("INPUT1");
    size_y2 = GetInputSizeY("INPUT2");
    
    if(size_x != GetInputSizeX("INPUT2"))
        Notify(msg_warning, "Merge: input matrices have incompatible sizes");
    
    max_rows = GetOutputSizeY("OUTPUT");
}



void
Merge::Tick()
{
    float ** m = create_concatenated_matrix(input1, input2, size_x, size_y1, size_y2);
    
    reset_matrix(output, size_x, max_rows);
    int k = 0; // output row
    
    for(int i=0; i<size_y1+size_y2 && k <max_rows; i++)
    {
        if (h_matrix_is_valid(m[i]))
        {
        float n = 0;
        float s[16];
        
        h_reset(s);
        
        int id = m[i][id_column];
        copy_array(output[k], m[i], size_x);   // Copy first entry to output to propagade id (and possibly other values; or if n=1)
        
        // Sum matrices for selected id
        
        for(int j=0; j<size_y1+size_y2; j++)
            if(m[j][id_column] == id)
            {
                h_add(s, m[j]);
                m[j][15] = 0; // mark as used
                n ++;
            }
        
        if(n > 1)   // Calculate average and orthogonalize
        {
            h_multiply(s, 1/n);
            h_normalize_rotation(s);
            h_copy(output[k], s);
        }
        
        k++;
        }
    }
    
    destroy_matrix(m);
    
//    print_matrix("merge", output, 19, 3);
}


static InitClass init("Merge", &Merge::Create, "Source/UserModules/Merge/");

