//
//	ImageConvolution.cc	This file is a part of the IKAROS project
//						A module to filter image with a kernel
//
//    Copyright (C) 2002-2007  Christian Balkenius
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

#include "ImageConvolution.h"
#include "ctype.h"

using namespace ikaros;


Module *
ImageConvolution::Create(Parameter * p)
{
    return new ImageConvolution(p);
}



ImageConvolution::ImageConvolution(Parameter * p):
        Module(p)
{
    AddInput("INPUT");
    AddOutput("OUTPUT");

    input	= NULL;
    output	= NULL;
    filter	= NULL;

    scale	= 1.0;
    bias	= 0.0;

    init_ok	= false;

    // Init filter kernel from xml file

    rectify		= GetBoolValue("rectify");
    scale           = GetFloatValue("scale");
    bias			= GetFloatValue("bias");
    filtersize_x	= GetIntValue("size_x");
    filtersize_y	= GetIntValue("size_y");
    filter			= GetMatrix("kernel", filtersize_x, filtersize_y);

    for (int j=0; j<filtersize_y; j++)
    {
        for (int i=0; i<filtersize_x; i++)
            Notify(msg_verbose, "%f\t", filter[j][i]);
        Notify(msg_verbose, "\n");
    }

    // prescale filter coefficients

    for (int j=0; j<filtersize_y; j++)
        for (int i=0; i<filtersize_x; i++)
            filter[j][i] *= scale;

    // Init filter kernel from file

    const char * filename = GetValue("file");
    if (filename != NULL)
    {
        // *****
    }
}



void
ImageConvolution::SetSizes()
{
    int sx = GetInputSizeX("INPUT");
    int sy = GetInputSizeY("INPUT");
    if (sx != unknown_size && sy != unknown_size)
        SetOutputSize("OUTPUT", sx-filtersize_x+1, sy-filtersize_y+1);
}



void
ImageConvolution::Init()
{
    inputsize_x	 = GetInputSizeX("INPUT");
    inputsize_y	 = GetInputSizeY("INPUT");

    outputsize_x    = GetOutputSizeX("OUTPUT");
    outputsize_y    = GetOutputSizeY("OUTPUT");

    input       = GetInputMatrix("INPUT");
    output		= GetOutputMatrix("OUTPUT");
}



ImageConvolution::~ImageConvolution()
{
}



void
ImageConvolution::Tick()
{
    convolve(output, input, filter, outputsize_x, outputsize_y, filtersize_x, filtersize_y, bias);

    if (rectify)
        abs(output, output, outputsize_x, outputsize_y);
}



