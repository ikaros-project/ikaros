//
//		Normalize.cc		This file is a part of the IKAROS project
//						Module that normalizes its inputs to the range 0-1
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
//	Created: 2004-03-22
//


#include "Normalize.h"

using namespace ikaros;

void
Normalize::Init()
{
    type    =   GetIntValueFromList("type");

    input	=	GetInputArray("INPUT");
    output  =	GetOutputArray("OUTPUT");
    size    =	GetOutputSize("OUTPUT");
}



void
Normalize::Tick()
{
    if(type == 0)
    {
        float minimum;
        float maximum;

        minmax(minimum, maximum, input, size);

        if (minimum != maximum)
            for (int i=0; i< size; i++)
                output[i] = (input[i] - minimum) / (maximum - minimum);
    }
    
    else if(type == 1)
    {
        copy_array(output, input, size);
        normalize(output, size);
    }
    
    else if(type == 2)
    {
        copy_array(output, input, size);
        normalize1(output, size);
    }

    else if(type == 3)
    {
        copy_array(output, input, size);
        normalize_max(output, size);
    }
}


