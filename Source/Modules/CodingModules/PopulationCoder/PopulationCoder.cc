//
//	PopulationCoder.cc	This file is a part of the IKAROS project
//
//
//    Copyright (C) 2016  Christian Balkenius
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


#include "PopulationCoder.h"

using namespace ikaros;


void
PopulationCoder::SetSizes()
{
    size_x  =   GetIntValue("size");
    int s   =   GetInputSize("INPUT");

    if (s != unknown_size)
        SetOutputSize("OUTPUT", size_x, s);
}


void
PopulationCoder::Init()
{
    size_x          =   GetIntValue("size");
    size_y          =   GetInputSize("INPUT");
    sigma			=   GetIntValue("sigma");
    min             =   GetFloatValue("min");
    max             =   GetFloatValue("max");
    
    input = GetInputArray("INPUT");
    output = GetOutputMatrix("OUTPUT");
}



void
PopulationCoder::Tick()
{
    for(int j=0; j<size_y; j++)
        gaussian1(output[j], (size_x-1)*(input[j]-min)/(max-min), sigma, size_x);
}

static InitClass init("PopulationCoder", &PopulationCoder::Create, "Source/Modules/CodingModules/PopulationCoder/");


