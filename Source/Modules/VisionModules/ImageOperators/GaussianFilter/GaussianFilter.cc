//
//	GaussianFilter.cc	This file is a part of the IKAROS project
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

#include "GaussianFilter.h"
#include "ctype.h"

using namespace ikaros;



void
GaussianFilter::ComputeKernel()
{
    gaussian(kernel, float(kernel_size)/2.0-0.5, float(kernel_size)/2.0-0.5, sigma, kernel_size, kernel_size);
    normalize1(kernel, kernel_size, kernel_size);
    copy_array(kernel_profile, kernel[kernel_size/2], kernel_size);
}



void
GaussianFilter::SetSizes()
{
    kernel_size = GetIntValue("kernel_size");
    int sx = GetInputSizeX("INPUT");
    int sy = GetInputSizeY("INPUT");

    if (sx != unknown_size && sy != unknown_size)
        SetOutputSize("OUTPUT", sx-kernel_size+1, sy-kernel_size+1);
    
    Module::SetSizes();
}



void
GaussianFilter::Init()
{
    Bind(sigma, "sigma");

    size_x	 = GetOutputSizeX("OUTPUT");
    size_y	 = GetOutputSizeY("OUTPUT");

    input    = GetInputMatrix("INPUT");
    output   = GetOutputMatrix("OUTPUT");
    kernel   = GetOutputMatrix("KERNEL");
    kernel_profile = GetOutputArray("PROFILE");
}



void
GaussianFilter::Tick()
{
    if(sigma != sigma_last)
        ComputeKernel();

    convolve(output, input, kernel, size_x, size_y, kernel_size, kernel_size);
    
    sigma_last = sigma;
}


static InitClass init("GaussianFilter", &GaussianFilter::Create, "Source/Modules/VisionModules/ImageOperators/GaussianFilter/");

