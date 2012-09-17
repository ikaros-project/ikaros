//
//	Upsample.cc		This file is a part of the IKAROS project
//					Module that extracts a focus of attention from an image
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

#include "Upsample.h"



Module *
Upsample::Create(Parameter * p)
{
    return new Upsample(p);
}



Upsample::Upsample(Parameter * p):
        Module(p)
{
    AddInput("INPUT");
    AddOutput("OUTPUT");

    input			= NULL;
    output		= NULL;
}



void
Upsample::SetSizes()
{
    int sizex = GetInputSizeX("INPUT");
    int sizey = GetInputSizeY("INPUT");

    if (sizex == unknown_size || sizex == unknown_size)
        return;

    SetOutputSize("OUTPUT", sizex*2, sizey*2);
}



void
Upsample::Init()
{
    inputsize_x	 = GetInputSizeX("INPUT");
    inputsize_y	 = GetInputSizeY("INPUT");

    outputsize_x	 = GetOutputSizeX("OUTPUT");
    outputsize_y	 = GetOutputSizeY("OUTPUT");

    input 		= GetInputMatrix("INPUT");
    output		= GetOutputMatrix("OUTPUT");
}



Upsample::~Upsample()
{
//	delete [] input;
}



void
Upsample::Tick()
{
    int sj = 0;
    for (int j=0; j<inputsize_y; j++)
    {
        int si = 0;
        for (int i=0; i<inputsize_x; i++)
        {
            float t = input[j][i];
            output[sj][si] = t;
            output[sj+1][si] =t;
            output[sj][si+1] = t;
            output[sj+1][si+1] = t;
            si += 2;
        }
        sj += 2;
    }
}


