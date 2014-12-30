//
//    AttentionWindow.cc   This file is a part of the IKAROS project
//
//
//    Copyright (C) 2014  Christian Balkenius
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

#include "AttentionWindow.h"

using namespace ikaros;


static float **
unwrap(float ** output, int output_size_x, int output_size_y, float ** input, int input_size_x, int input_size_y, float bounds[8], float supersampling=10)
{
    float inc = 1/supersampling;
    reset_matrix(output, output_size_x, output_size_y);
    for(float j=0; j<output_size_y; j+=inc)
    {
        float dx0 = bounds[0] + (j/output_size_x) * (bounds[6]-bounds[0]);    // position along left boundary
        float dx1 = bounds[2] + (j/output_size_x) * (bounds[4]-bounds[2]);    // position along right boundary

        float dy0 = bounds[1] + (j/output_size_x) * (bounds[7]-bounds[1]);    // position along left boundary
        float dy1 = bounds[3] + (j/output_size_x) * (bounds[5]-bounds[3]);    // position along right boundary

        for(float i=0; i<output_size_x; i+=inc)
        {
            float x = dx0 + (i/output_size_y) * (dx1 - dx0);
            float y = dy0 + (i/output_size_y) * (dy1 - dy0);
            
            output[int(j)][int(i)]+= input[int(y)][int(x)];
        }
    }
    
    return output;
}



Module *
AttentionWindow::Create(Parameter * p)
{
    return new AttentionWindow(p);
}



void
AttentionWindow::Init()
{
    Bind(window_radius, "window_radius");
    Bind(window_size_x, "window_size_x");
    Bind(window_size_y, "window_size_y");

    input            = GetInputMatrix("INPUT");
    input_size_x	 = GetInputSizeX("INPUT");
    input_size_y	 = GetInputSizeY("INPUT");

    output          = GetOutputMatrix("OUTPUT");
    output_size_x	= GetOutputSizeX("OUTPUT");
    output_size_y	= GetOutputSizeY("OUTPUT");

    top_down_position = GetInputArray("TOP_DOWN_POSITION", false);
    top_down_bounds = GetInputArray("TOP_DOWN_BOUNDS", false);
    
    bottom_up_position = GetInputMatrix("BOTTOM_UP_POSITION", false);
    bottom_up_bounds = GetInputMatrix("BOTTOM_UP_BOUNDS", false);
    bottom_up_count = GetInputArray("BOTTOM_UP_COUNT", bottom_up_position); // required if bottom_up_position is connected
}



void
AttentionWindow::Tick()
{
    // Calculate focus point
    
    float focus_x = 0.5;
    float focus_y = 0.5;
    
    if(top_down_position)
    {
        focus_x = top_down_position[0];
        focus_y = top_down_position[1];
    }
    
    int k=0;
    if(bottom_up_position && bottom_up_count > 0)
    {
        float d=maxfloat;
        
        for(int i=1; i<int(*bottom_up_count); i++)
        {
            float t = hypot(focus_x-bottom_up_position[i][0], focus_y-bottom_up_position[i][1]);
            if(t < d)
            {
                d = t;
                k = i;
            }
        }
        
        focus_x = bottom_up_position[k][0];
        focus_y = bottom_up_position[k][1];
    }

    // Set source bounds
    
    if(window_size_x != 0 && window_size_y != 0)
    {
        window_size_x = 2*window_radius;
        window_size_y = 2*window_radius;
    }
    
    float bounds[8] = {
        focus_x + window_size_x/2,
        focus_x + window_size_x/2,
        focus_x + window_size_x/2,
        focus_x - window_size_x/2,
        focus_y - window_size_y/2,
        focus_y + window_size_y/2,
        focus_y + window_size_y/2,
        focus_y - window_size_y/2
    };
    
    if(bottom_up_position && bottom_up_count > 0)
        for(int i=0; i<8; i++)
            bounds[i] = bottom_up_bounds[k][i];
    
    else if(top_down_bounds)
        for(int i=0; i<8; i++)
            bounds[i] = top_down_bounds[i];
    
    // Unwrap attention window into output
    
    unwrap(output, output_size_x, output_size_y, input, input_size_x, input_size_y, bounds);
}


static InitClass init("AttentionWindow", &AttentionWindow::Create, "Source/Modules/VisionModules/AttentionWindow/");

