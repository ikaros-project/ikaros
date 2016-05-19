//
//	EdgeSegmentation.cc	This file is a part of the IKAROS project
//
//
//    Copyright (C) 2016  Christian Balkenius
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

#include "EdgeSegmentation.h"

using namespace ikaros;


/*
static float **
sort_rows(float ** a, int col, int size_x, int size_y)
{
    int i , j;
    float * t = create_array(size_x);
    
    for(i = 1; i < size_y; i++)
    {
        copy_array(t, a[i], size_x);
        
        for(j = i; j > 0 && t[col] < a[j-1][col]; j--)
            copy_array(a[j], a[j-1], size_x);
        
        copy_array(a[j], t, size_x);
    }
    
    destroy_array(t);
    return a;
}
*/



void
EdgeSegmentation::Init()
{
    Bind(threshold, "threshold");
    Bind(grid, "grid");
    Bind(normalize, "normalize");

    max_edges   = GetIntValue("max_edges");
    size_x	 	= GetInputSizeX("INPUT");
    size_y	 	= GetInputSizeY("INPUT");

    input			= GetInputMatrix("INPUT");
    dx              = GetInputMatrix("DX");
    dy              = GetInputMatrix("DY");
    output			= GetOutputMatrix("OUTPUT");

    edge_list       = GetOutputMatrix("EDGE_LIST");
    edge_elements   = GetOutputMatrix("EDGE_ELEMENTS");
    edge_list_size  = GetOutputArray("EDGE_LIST_SIZE");
}



void
EdgeSegmentation::Tick()
{
    float isx = 1/float(size_x);
    float isy = 1/float(size_y);
    float l = 2;
    float m = max(input, size_x, size_y);
    float t = threshold*m;
    
    int   edge_count = 0;

    reset_matrix(output, size_x, size_y);
    reset_matrix(edge_list, 4, max_edges);
    reset_matrix(edge_elements, 4, max_edges);
    
    if(t == 0)
    {
        *edge_list_size = 0;
        return;
    }

    for(int j=0; j<size_y; j+=(1+grid))
        for(int i=0; i<size_x; i+=(1+grid))
            if(input[j][i] > t && edge_count < max_edges)
            {
                output[j][i] = 1;
                edge_list[edge_count][0] = float(i);
                edge_list[edge_count][1] = float(j);
                
                float h = hypot(dx[j][i], dy[j][i]);
                if(!normalize)
                    h = 1;
                
                float dX = dx[j][i]/h;
                float dY = dy[j][i]/h;
                
                edge_list[edge_count][2] = dX;
                edge_list[edge_count][3] = dY;
                
                edge_elements[edge_count][0] = isx*(float(i) - l*dY);
                edge_elements[edge_count][1] = isy*(float(j) + l*dX);
                edge_elements[edge_count][2] = isx*(float(i) + l*dY);
                edge_elements[edge_count][3] = isy*(float(j) - l*dX);
                
                edge_count++;
            }
    
    *edge_list_size = float(edge_count);
}



static InitClass init("EdgeSegmentation", &EdgeSegmentation::Create, "Source/Modules/VisionModules/ImageOperators/EdgeDetectors/EdgeSegmentation/");


