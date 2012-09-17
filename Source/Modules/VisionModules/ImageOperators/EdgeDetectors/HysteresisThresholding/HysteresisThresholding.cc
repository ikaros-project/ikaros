//
//	HysteresisThresholding.cc	This file is a part of the IKAROS project
//							A module to threshold edges in an image
//
//    Copyright (C) 2003  Christian Balkenius
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


#include "HysteresisThresholding.h"



Module *
HysteresisThresholding::Create(Parameter * p)
{
    return new HysteresisThresholding(p);
}



HysteresisThresholding::HysteresisThresholding(Parameter * p):
	Module(p)
{
    AddInput("INPUT");
    AddOutput("OUTPUT");

    input			= NULL;
    output			= NULL;

    T1				= GetFloatValue("T1", 0.3);
    T2				= GetFloatValue("T2", 0.6);

    iterations		= GetIntValue("iterations", 1);
    range			= GetIntValue("range", 1);
}



void
HysteresisThresholding::SetSizes()
{
    int sx = GetInputSizeX("INPUT");
    int sy = GetInputSizeY("INPUT");
    if (sx != unknown_size && sy != unknown_size)
        SetOutputSize("OUTPUT", sx, sy);
}



void
HysteresisThresholding::Init()
{
    inputsize_x	 	= GetInputSizeX("INPUT");
    inputsize_y	 	= GetInputSizeY("INPUT");

    outputsize_x	 = GetOutputSizeX("OUTPUT");
    outputsize_y	 = GetOutputSizeY("OUTPUT");

    input			= GetInputMatrix("INPUT");
    output			= GetOutputMatrix("OUTPUT");
}



HysteresisThresholding::~HysteresisThresholding()
{
}



void
HysteresisThresholding::Tick()
{
    reset_matrix(output, outputsize_x, outputsize_y);

    for (int n=0; n<iterations; n++)
        for (int j =range; j<inputsize_y-range; j++)
            for (int i=range; i<inputsize_x-range; i++)
                if (input[j][i] > T2 || output[j][i] > 0)	// edge in input or after hysteresis
                    for (int jj=-range; jj<=range; jj++)
                        for (int ii=-range; ii<=range; ii++)
                            if (input[j+jj][i+ii] > T1)
                                output[j+jj][i+ii] = 1;
}



