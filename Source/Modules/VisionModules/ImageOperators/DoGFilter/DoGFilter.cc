//
//	DoGFilter.cc	This file is a part of the IKAROS project
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

#include "DoGFilter.h"
#include "ctype.h"

using namespace ikaros;



void
DoGFilter::ComputeKernel()
{
    float km = float(kernel_size)/2.0-0.5;
    
    for(int i=0; i<kernel_size; i++)
        for(int j=0; j<kernel_size; j++)
        {
            float d = hypot(float(j)-km, float(i)-km);
            kernel[j][i] = (1/(sigma1*sqrt2pi))*exp(-sqr(d)/(2*sqr(sigma1)))  -  (1/(sigma2*sqrt2pi))*exp(-sqr(d)/(2*sqr(sigma2)));
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
DoGFilter::SetSizes()
{
    kernel_size = GetIntValue("kernel_size");
    int sx = GetInputSizeX("INPUT");
    int sy = GetInputSizeY("INPUT");

    if (sx != unknown_size && sy != unknown_size)
        SetOutputSize("OUTPUT", sx-kernel_size+1, sy-kernel_size+1);
    
    Module::SetSizes();
}



void
DoGFilter::Init()
{
    Bind(sigma1, "sigma1");
    Bind(sigma2, "sigma2");
    Bind(normalize, "normalize");

    size_x	 = GetOutputSizeX("OUTPUT");
    size_y	 = GetOutputSizeY("OUTPUT");

    input    = GetInputMatrix("INPUT");
    output   = GetOutputMatrix("OUTPUT");
    kernel   = GetOutputMatrix("KERNEL");
    kernel_profile = GetOutputArray("PROFILE");
}



void
DoGFilter::Tick()
{
    if(sigma1 != sigma1_last || sigma2 != sigma2_last)
        ComputeKernel();

    convolve(output, input, kernel, size_x, size_y, kernel_size, kernel_size);
    
    sigma1_last = sigma1;
    sigma2_last = sigma2;
}


static InitClass init("DoGFilter", &DoGFilter::Create, "Source/Modules/VisionModules/ImageOperators/DoGFilter/");

