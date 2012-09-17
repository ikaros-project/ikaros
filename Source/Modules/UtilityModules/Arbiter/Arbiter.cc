//
//    Arbiter.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2006 Christian Balkenius
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

#include "Arbiter.h"

using namespace ikaros;


void
Arbiter::Init()
{
    input1		=	GetInputArray("INPUT1");
    input2		=	GetInputArray("INPUT2");
    value1		=	GetInputArray("VALUE1", false);
    value2		=	GetInputArray("VALUE2", false);
    output		=	GetOutputArray("OUTPUT");
    value		=	GetOutputArray("VALUE");
    size		=	GetOutputSize("OUTPUT");
}



void
Arbiter::Tick()
{
    float v1 = (value1 ? value1[0] : norm(input1, size));
    float v2 = (value2 ? value2[0] : norm(input2, size));
	
    if (v1 >= v2)
    {
        copy_array(output, input1, size);
        value[0] = v1;
    }
    else
    {
        copy_array(output, input2, size);
        value[0] = v2;
    }
}


