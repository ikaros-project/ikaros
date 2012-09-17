//
//		Softmax.cc		This file is a part of the IKAROS project
//						A module applies a softmax function to its input
//
//    Copyright (C) 2004  Christian Balkenius
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


#include "Softmax.h"


using namespace ikaros;

void
Softmax::Init()
{
    type        =   GetIntValueFromList("type");
    exponent	=   GetFloatValue("exponent", 2.0);

    size_x	= GetInputSizeX("INPUT");
    size_y	= GetInputSizeY("INPUT");

    input	= GetInputMatrix("INPUT");
    output	= GetOutputMatrix("OUTPUT");
}



void
Softmax::Tick()
{
    if(type == 0) // p0lynomial
    {
        for (int i=0; i<size_x; i++)
            for (int j=0; j<size_y; j++)
                output[j][i] = pow(input[j][i], exponent);
    }
    else // exponential
    {
        for (int i=0; i<size_x; i++)
            for (int j=0; j<size_y; j++)
                output[j][i] = exp(input[j][i]);
    }

    float s = 0;
    for (int i=0; i<size_x; i++)
        for (int j=0; j<size_y; j++)
            s += output[j][i];

    if (s != 0)
    {
        for (int i=0; i<size_x; i++)
            for (int j=0; j<size_y; j++)
                output[j][i] = output[j][i]/s;
    }
    else
    {
        for (int i=0; i<size_x; i++)
            for (int j=0; j<size_y; j++)
                output[j][i] = 0.0;
    }
}


