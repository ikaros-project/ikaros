//
//		Multiply.cc		This file is a part of the IKAROS project
//						Implements a modules that multiplies its two inputs element by element
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


#include "Multiply.h"

using namespace ikaros;


void
Multiply::Init()
{
    // The test of equal input size must be done here and not in SetSize
    // to allow an ouput to be connected to an input of the same module

    int size1x = GetInputSizeX("INPUT1");
    int size1y = GetInputSizeY("INPUT1");

    int size2x = GetInputSizeX("INPUT2");
    int size2y = GetInputSizeY("INPUT2");

    if (size1x != size2x || size1y != size2y)
    {
        Notify(msg_fatal_error, "Module \"%s\" (Multiply): The input sizes must be equal: INPUT1 = [%d, %d] INPUT2 = [%d, %d]\n", GetName(), size1x, size1y, size2x, size2y);
        return;
    }

    // The input and outputs are treated as arrays internally

    input1		=	GetInputArray("INPUT1");
    input2		=	GetInputArray("INPUT2");
    output		=	GetOutputArray("OUTPUT");
    size        =	GetOutputSize("OUTPUT");
}



void
Multiply::Tick()
{
    multiply(output, input1, input2, size);
}


