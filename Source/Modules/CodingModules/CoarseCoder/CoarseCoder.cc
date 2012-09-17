//
//	CoarseCoder.cc	This file is a part of the IKAROS project
//
//
//    Copyright (C) 2007  Christian Balkenius
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


#include "CoarseCoder.h"

using namespace ikaros;

void
CoarseCoder::Init()
{
    type			=   GetIntValueFromList("type");
    output_size		=   GetIntValue("output_size");
    radius			=   GetIntValue("radius");
    min             =   GetFloatValue("min");
    max             =   GetFloatValue("max");
    
    if(GetBoolValue("normalize", false))
        value = 1.0/sqr(2*radius+1);
    else
        value = 1.0;

    input = GetInputArray("INPUT");
	if(GetInputSize("INPUT") != 2)
	{
		Notify(msg_fatal_error, "%s (CoarseCoder) input must have size 2 (not %d)\n", GetName(), GetInputSize("INPUT"));
		return;
	}
	
    output = GetOutputMatrix("OUTPUT");
}



void
CoarseCoder::MakeTile()
{
    float r = float(radius);
    float w = 1+2*r;
    float s = float(output_size);
    float scale = s-w;
    float x = r+scale*(input[0] - min)/(max-min);
    float y = r+scale*(input[1] - min)/(max-min);
    
    reset_matrix(output, output_size, output_size);

	for (int j=int(y-r+0.5); j<=int(y+r+0.5); j++)
		for (int i=int(x-r+0.5); i<=int(x+r+0.5); i++)
            output[j][i] = value;
}



void
CoarseCoder::MakeGaussian()
{
    float r = float(radius);
    float w = 1+2*r;
    float s = float(output_size);
    float scale = s-w;
    float x = r+scale*(input[0] - min)/(max-min);
    float y = r+scale*(input[1] - min)/(max-min);

	for (int j=0; j<output_size; j++)
		for (int i=0; i<output_size; i++)
				output[j][i] = exp(-hypot(y-j,x-i));
}



void
CoarseCoder::Tick()
{
    switch(type)
    {
        case 0: MakeTile(); break;
        case 1: MakeGaussian(); break;
    }    
}


