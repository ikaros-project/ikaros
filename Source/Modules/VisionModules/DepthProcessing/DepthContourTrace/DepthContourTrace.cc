//
//	DepthContourTrace	This file is a part of the IKAROS project
//                       
//
//    Copyright (C) 2012  Christian Balkenius
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

#include "DepthContourTrace.h"


using namespace ikaros;


void
DepthContourTrace::Init()
{
    Bind(segment_length, "segment_length");
    Bind(segment_smoothing, "segment_smoothing");

    size_x	 = GetInputSizeX("INPUT");
    size_y	 = GetInputSizeY("INPUT");

    size     = GetOutputSizeY("OUTPUT");

    input       = GetInputMatrix("INPUT");
    output		= GetOutputMatrix("OUTPUT");
    box         = GetOutputMatrix("BOX");
    length		= GetOutputArray("LENGTH");
    debug		= GetOutputMatrix("DEBUG");
        
    *length = 0;
}



void
DepthContourTrace::Tick()
{
    reset_matrix(box, 2, 4);
    copy_matrix(debug, input, size_x, size_y);
    multiply(debug, 0.25, size_x, size_y);

    const float inc = 0.001;
    const int bottom = 5;
    
    *length = 0;
    
    // position first and second point

    int i=0;
    for(; i<size_x; i++)
        if(input[size_y-bottom][i] > 0)
        {
            output[0][2] = float(i);
            output[0][3] = float(size_y-bottom);
            break;
        }
    
    if(i == size_x-1) // no object found
    {
//            printf("no object found (s)\n");
            return;
    }
    
    for(float a = 0; a<2*pi; a+=inc) // rotate line, starting to the left along x axis
    {
        float x = -segment_length*cos(a)+output[0][2];
        float y = -segment_length*sin(a)+output[0][3];
        
        x = clip(x, 0, size_x-1);
        y = clip(y, 0, size_y-1);
        
        if(input[int(y)][int(x)] == 1)
        {
            output[1][2] = x;
            output[1][3] = y;
            *length = 2;
            break;
        }
        else
            debug[int(y)][int(x)] = 1;
    }
    
     if(*length < 2)
    {
//            printf("no object found (l)\n");
            return;
    }

    // fold the rest of the snake
    
    for(int k=1; k<size-1; k++)
    {
        // Starting angle
        
        float b = atan2(output[k-1][3]-output[k][3], output[k-1][2]-output[k][2])-pi/2;
        
        // Rotate until contact
    
        for(float a = b; a<b+2*pi; a+=inc) // rotate line
        {
            float x = -segment_length*cos(a)+output[k][2];
            float y = -segment_length*sin(a)+output[k][3];
                
            x = clip(x, 0, size_x-1);
            y = clip(y, 0, size_y-1);
            
            if(input[int(y)][int(x)] == 1)
            {
                output[k+1][2] = x;
                output[k+1][3] = y;
                *length += 1;
                break;
            }
            else
                debug[int(y)][int(x)] = 1;
        }
        
        if(k>3 && output[k+1][3] > size_y-bottom)
            break;
    }
    
    output[0][3] = size_y-1;
    output[int(*length)-1][3] = size_y-1;
    
//    printf("object found: %f\n", *length);
   

    // Smoothing
    
    // Replace each point withe the average of its neighbors 
    
    for(int k=0; k<segment_smoothing; k++) // number of smoothing iterations
    {
        for(int i=k+1; i<size-k-1; i++)
        {
            output[i][0] = 0.5*(output[i-1][2] + output[i+1][2]);
            output[i][1] = 0.5*(output[i-1][3] + output[i+1][3]);
        }

        for(int i=1; i<size-1; i++)
        {
            output[i][2] = output[i][0];
            output[i][3] = output[i][1];
        }    
    }
    
    // Map to 0-1
    
    for(int i=0; i<size; i++)
    {
        output[i][0] = output[i][2] / size_x;
        output[i][1] = output[i][3] / size_y;
    }
    
    // Find rectangle around contour
    
    float x0=1;
    float x1=0;
    float y0=1;
    float y1=0;
    for(int i=0; i<*length; i++)
    {
        if(output[i][0] < x0)
            x0 = output[i][0];
        if(output[i][0] > x1)
            x1 = output[i][0];

        if(output[i][1] < y0)
            y0 = output[i][1];
        if(output[i][1] > y1)
            y1 = output[i][1];
    }
    
    box[0][0] = x0;
    box[0][1] = y1;
    box[1][0] = x0;
    box[1][1] = y0;
    
    box[2][0] = x1;
    box[2][1] = y0;
    box[3][0] = x1;
    box[3][1] = y1;
    
//    printf("%f %f - %f %f\n", x0, x1, y0, y1);
}



static InitClass init("DepthContourTrace", &DepthContourTrace::Create, "Source/Modules/VisionModules/DepthProcessing/DepthContourTrace/");

