//
//	IntervalCoder.cc	This file is a part of the IKAROS project
//
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


#include "IntervalCoder.h"


void
IntervalCoder::Init()
{
    input       =   GetInputArray("INPUT");
    output      =   GetOutputArray("OUTPUT");

    output_size	=   GetOutputSize("OUTPUT");

    radius		=   GetIntValue("radius", 1);
    min			=   GetFloatValue("min", 0.0);
    max			=   GetFloatValue("max", 1.0);
	
}



void
IntervalCoder::Tick()
{
    reset_array(output, output_size);

    float N = float(output_size) - 2 * float(radius);
    float I = (max-min) / float(N-1);
//	float c0 = min - float(radius) * I;

    for (int i=0; i<output_size; i++)
    {
        float x = float(i) - (input[0] -  min)/I - radius;

        if (x < 0)
            x = -x;

        if (x < radius + 0.5)
            output[i] = 1.0/float(1+2*radius);
    }
}


