//
//	DepthHistogram	This file is a part of the IKAROS project
//                          Segment image into depth planes
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

#include "DepthHistogram.h"


using namespace ikaros;


void
DepthHistogram::Init()
{
    Bind(min, "min");
    Bind(max, "max");
    
    Bind(filter, "filter");
    Bind(threshold, "threshold");

    size_x	 = GetInputSizeX("INPUT");
    size_y	 = GetInputSizeY("INPUT");
    size     = GetOutputSize("OUTPUT");

    input       = GetInputMatrix("INPUT");
    output		= GetOutputArray("OUTPUT");
    object		= GetOutputArray("OBJECT");
}



void
DepthHistogram::Tick()
{
/*
    float aa;
    float bb;
    minmax(aa, bb, input, size_x, size_y);
    printf("min = %f, max = %f\n", aa, bb);
*/

    reset_array(output, size);
    reset_array(object, 3);

    for(int i=0; i<size_x; i++)
        for(int j=0; j<size_y; j++)
        {
            int index = int((float(size)*(input[j][i]-min))/(max-min));
//            printf("index = %d\n", index);
            if(0 <= index && index < size)
                output[index] += 1;
        }
    
    if(!filter)
        return;
    
    output[0] = 0;
    output[size-1] = 0;

    int a = 0;
    int b = 0;
    float s = 0;
    long int n = 0;

    for(int i=1; i<size; i++)
    {
        if(a == 0 && output[i] > 0)
            a = i;
        
        if(a > 0)
        {
            if(output[i] == 0 && s > threshold)
            {
                b = i;
                break;
            }
            s += float(i)*output[i];
            n += output[i];
        }
    }

    if(a == 0 || b == 0)
        return;
    
    for(int i=b; i<size; i++)
        output[i] = -output[i];


    object[0] = a*(max-min)/float(size);
    object[1] = b*(max-min)/float(size);
    object[2] = s*(max-min)/(float(n)*float(size));
    
//    printf("%.0f %.0f %.0f\n", object[0], object[1], object[2]);
}



static InitClass init("DepthHistogram", &DepthHistogram::Create, "Source/Modules/VisionModules/DepthProcessing/DepthHistogram/");

