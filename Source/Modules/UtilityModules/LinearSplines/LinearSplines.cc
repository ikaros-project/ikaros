//
//	LinearSplines.cc		This file is a part of the IKAROS project
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

#include "LinearSplines.h"

using namespace ikaros;


void
LinearSplines::Init()
{
    points = create_matrix(GetValue("points"), size_x, size_y);

    input = GetInputArray("INPUT");
    output = GetOutputArray("OUTPUT");
    
    input_size = GetInputSize("INPUT");
    
    if(size_x < 2 || size_y < 2)
        Notify(msg_fatal_error, "Not enough point data");
}



void
LinearSplines::Tick()
{
    for(int i=0; i<input_size; i++)
    {
        float x = input[i];
        int col = 2*(i%(size_x/2));
        
        if(x < points[0][col])
        {
            output[i] = points[0][col+1];
            continue;
        }
        
        float y = points[0][col+1];
        for(int j=0; j<size_y-1; j++)
        {
            float x0 = points[j][col];
            float x1 = points[j+1][col];
            
            if(x0 != x1)
            {
                if(x0 <= x && x < x1)
                {
                    float y0 = points[j][col+1];
                    float y1 = points[j+1][col+1];
                    float k = (y1-y0)/(x1-x0);
                    y = y0 + k*(x-x0);
                    break;
                }
                else
                    y = points[j+1][col+1];
            }
        }
        
        output[i] = y;
    }
}



static InitClass init("LinearSplines", &LinearSplines::Create, "Source/Modules/UtilityModules/LinearSplines/");


