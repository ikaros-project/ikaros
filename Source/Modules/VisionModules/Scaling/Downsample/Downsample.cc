//
//	Downsample.cc		This file is a part of the IKAROS project
//						Module that downsamples an image
//
//    Copyright (C) 2006  Christian Balkenius
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

#include "Downsample.h"



Module *
Downsample::Create(Parameter * p)
{
    return new Downsample(p);
}



Downsample::Downsample(Parameter * p):
        Module(p)
{
    AddInput("INPUT");
    AddOutput("OUTPUT");

    input			= NULL;
    output		= NULL;
}



void
Downsample::SetSizes()
{
    int sizex = GetInputSizeX("INPUT");
    int sizey = GetInputSizeY("INPUT");

    if (sizex == unknown_size || sizex == unknown_size)
        return;

    SetOutputSize("OUTPUT", sizex/2, sizey/2);
}



void
Downsample::Init()
{
    inputsize_x	 = GetInputSizeX("INPUT");
    inputsize_y	 = GetInputSizeY("INPUT");

    outputsize_x	 = GetOutputSizeX("OUTPUT");
    outputsize_y	 = GetOutputSizeY("OUTPUT");

    input 		= GetInputMatrix("INPUT");
    output		= GetOutputMatrix("OUTPUT");
}



Downsample::~Downsample()
{
//	delete [] input;
}



void
Downsample::Tick()
{
    int sj = 0;
    for (int j=0; j<outputsize_y; j++)
    {
        int si = 0;
        for (int i=0; i<outputsize_x; i++)
        {
            output[j][i] = 0.25*(input[sj][si] + input[sj+1][si] + input[sj][si+1] + input[sj+1][si+1]);
            si += 2;
        }
        sj += 2;
    }
}


