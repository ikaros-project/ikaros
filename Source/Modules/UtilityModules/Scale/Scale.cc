//
//		Scale.cc		This file is a part of the IKAROS project
//					Module that multiplies its inputs (and keeps its topology if possible)
//
//    Copyright (C) 2004 Christian Balkenius
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
//	Created: 2004-03-22
//


#include "Scale.h"

using namespace ikaros;

void
Scale::Init()
{
    Bind(factor, "factor");
    
    input		=	GetInputArray("INPUT");
    scale		=	GetInputArray("SCALE");
    output		=	GetOutputArray("OUTPUT");
    size		=	GetOutputSize("OUTPUT");
}



void
Scale::Tick()
{
    float s = factor;
    if(scale)
        s = s * scale[0];

    multiply(output, input, s, size);
}


static InitClass init("Scale", &Scale::Create, "Source/Modules/UtilityModules/Scale/");
