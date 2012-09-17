//
//		Distance.cc		This file is a part of the IKAROS project
//						Implements a modules that calculates the distance between its two inputs
//
//    Copyright (C) 2005 Christian Balkenius
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
//	Created: 2005-04-13
//


#include "Distance.h"

using namespace ikaros;

void
Distance::Init()
{
    type        =   GetIntValueFromList("type");
    
    input1		=	GetInputArray("INPUT1");
    input2		=	GetInputArray("INPUT2");
    output		=	GetOutputArray("OUTPUT");

    int size1		=	GetInputSize("INPUT1");
    int size2		=	GetInputSize("INPUT2");

    if (size1 != size2)
        Notify(msg_warning, "Module \"%s\" (Distance): The input sizes should normally be equal: INPUT1 = %d, INPUT2 = %d\n", GetName(), size1, size2);

    size = (size1 < size2 ? size1 : size2);
}



void
Distance::Tick()
{
    if(type == 0)
        output[0]  = dist(input1, input2, size);
        
    else if(type==1)
        output[0]  = dist1(input1, input2, size);
}


