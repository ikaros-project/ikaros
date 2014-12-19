//
//	TappedDelayLine.cc	This file is a part of the IKAROS project
//
//
//    Copyright (C) 2014  Christian Balkenius
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


#include "TappedDelayLine.h"


void
TappedDelayLine::Init()
{
    input       =   GetInputArray("INPUT");
    output      =   GetOutputMatrix("OUTPUT");

    size_x      =   GetOutputSizeX("OUTPUT");
    size_y      =   GetOutputSizeY("OUTPUT");
}



void
TappedDelayLine::Tick()
{
    for(int j=size_y-1; j>0; j--)
        copy_array(output[j], output[j-1], size_x);
    
    copy_array(output[0], input, size_x);
}



static InitClass init("TappedDelayLine", &TappedDelayLine::Create, "Source/Modules/CodingModules/TappedDelayLine/");

