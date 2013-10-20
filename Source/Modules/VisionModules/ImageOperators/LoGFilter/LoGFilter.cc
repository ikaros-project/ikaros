//
//	LoGFilter.cc	This file is a part of the IKAROS project
//
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

#include "LoGFilter.h"
#include "ctype.h"

using namespace ikaros;



void
LoGFilter::ComputeKernel()
{
    float km = float(kernel_size)/2.0-0.5;
    float f  = -1/(pi*sqr(sqr(sigma)));
    float g  =  1/(2*sqr(sigma));
    
    for(int i=0; i<kernel_size; i++)
        for(int j=0; j<kernel_size; j++)
        {
            float d = hypot(float(j)-km, float(i)-km);
            kernel[j][i] = f * (1-g*sqr(d)) * exp(-g*sqr(d));
        }
    
    if(normalize)
    {
        float sum_p = 0;
        float sum_n = 0;

        for(int i=0; i<kernel_size; i++)
            for(int j=0; j<kernel_size; j++)
                if(kernel[j][i] > 0)
                    sum_p += kernel[j][i];
                else
                    sum_n -= kernel[j][i];
        
        float s_p = (sum_p != 0 ? 1/sum_p : 0);
        float s_n = (sum_n != 0 ? 1/sum_n : 0);
        
        for(int i=0; i<kernel_size; i++)
            for(int j=0; j<kernel_size; j++)
                if(kernel[j][i] > 0)
                    kernel[j][i] *= s_p;
                else
                    kernel[j][i] *= s_n;

    }

    copy_array(kernel_profile, kernel[kernel_size/2], kernel_size);
}



void
LoGFilter::SetSizes()
{
    kernel_size = GetIntValue("kernel_size");
    int sx = GetInputSizeX("INPUT");
    int sy = GetInputSizeY("INPUT");

    if (sx != unknown_size && sy != unknown_size)
        SetOutputSize("OUTPUT", sx-kernel_size+1, sy-kernel_size+1);
    
    Module::SetSizes();
}



void
LoGFilter::Init()
{
    Bind(sigma, "sigma");
    Bind(normalize, "normalize");

    size_x	 = GetOutputSizeX("OUTPUT");
    size_y	 = GetOutputSizeY("OUTPUT");

    input    = GetInputMatrix("INPUT");
    output   = GetOutputMatrix("OUTPUT");
    kernel   = GetOutputMatrix("KERNEL");
    kernel_profile = GetOutputArray("PROFILE");
}



void
LoGFilter::Tick()
{
    if(sigma != sigma_last)
        ComputeKernel();

    convolve(output, input, kernel, size_x, size_y, kernel_size, kernel_size);
    
    sigma_last = sigma;
}


static InitClass init("LoGFilter", &LoGFilter::Create, "Source/Modules/VisionModules/ImageOperators/LoGFilter/");

