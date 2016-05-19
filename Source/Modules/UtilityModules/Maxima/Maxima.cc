//
//	Maxima.cc		This file is a part of the IKAROS project
//					A module that sets the maximum element of its input to 1
//
//    Copyright (C) 2013  Christian Balkenius
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


#include "Maxima.h"

using namespace ikaros;


static float **
sort_rows(float ** a, int col, int size_x, int size_y) // Should ne moved to math
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



void
Maxima::Init()
{
    Bind(sort_points, "sort_points");
    Bind(max_points, "max_points");
    Bind(threshold, "threshold");
    
    size_x	= GetInputSizeX("INPUT");
    size_y	= GetInputSizeY("INPUT");

    input   = GetInputMatrix("INPUT");
    output	= GetOutputMatrix("OUTPUT");
    points  = GetOutputMatrix("POINTS");
    point_count  = GetOutputArray("POINT_COUNT");
}



void
Maxima::Tick()
{
    reset_matrix(output, size_x, size_y);
    reset_matrix(points, 3, max_points);
    int c = 0;
    
    for(int j=1; j<size_y-1; j++)
        for(int i=1; i<size_x-1; i++)
        {
            float v = input[j][i];
            
            if(v< threshold)
                continue;
            
            if(v < input[j-1][i-1])
                continue;

            if(v < input[j-1][i])
                continue;
            
            if(v < input[j-1][i+1])
                continue;
            
            if(v < input[j][i-1])
                continue;
            
            if(v < input[j][i+1])
                continue;
            
            if(v < input[j+1][i-1])
                continue;
            
            if(v < input[j+1][i])
                continue;
            
            if(v < input[j+1][i+1])
                continue;
            
            output[j][i] = 1;

            if(c<max_points)
            {
                points[c][0] = float(i+0.5)/float(size_x);
                points[c][1] = float(j+0.5)/float(size_y);
                points[c][2] = input[j][i];
            
                c++;
            }
        }
    
    *point_count = float(c);
    
    if(sort_points)
        sort_rows(points, 2, 3, c);
}


static InitClass init("Maxima", &Maxima::Create, "Source/Modules/UtilityModules/Maxima/");
