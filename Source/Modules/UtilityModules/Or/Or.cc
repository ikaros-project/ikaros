//
//	Or.cc		This file is a part of the IKAROS project
//				A module can be used to disconnect modules using a flag
//
//    Copyright (C) 2022  Christian Balkenius
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


#include "Or.h"
void
Or::Init()
{
    Bind(high, "high");
    Bind(low, "low");
    Bind(threshold, "threshold");
    input1	= GetInputArray("INPUT1");
    input2	= GetInputArray("INPUT2");
    output	= GetOutputArray("OUTPUT");
    inverse	= GetOutputArray("INVERSE");
}




void
Or::Tick()
{
    if(*input1 > threshold || *input2 > threshold)
    {
        *output = high;
        *inverse = low;   
    }
    else
    {
        *output = low;
        *inverse = high;     
    }   
}


static InitClass init("Or", &Or::Create, "Source/Modules/UtilityModules/Or/");

