//
//	ArrayToMatrix.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2014 Birger Johansson
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
//    See http://www.ikaros-project.org/ for more information.
//


#include "ArrayToMatrix.h"

using namespace ikaros;

void
ArrayToMatrix::Init()
{
    input = GetInputArray("INPUT");
    output = GetOutputMatrix("OUTPUT");
}

void
ArrayToMatrix::SetSizes()
{
    // Getting parameters early as we will be using these to set output size
    arrayLength = GetIntValue("array_length");
    
    // Find length of input
    inputSizeX = GetInputSizeX("INPUT");
    
    if (inputSizeX != unknown_size)
    {
        nrArrays = inputSizeX/arrayLength;
        SetOutputSize("OUTPUT", arrayLength, nrArrays);
    }
}


void
ArrayToMatrix::Tick()
{
    copy_array(*output, input, inputSizeX);
}

static InitClass init("ArrayToMatrix", &ArrayToMatrix::Create, "Source/Modules/UtilityModules/ArrayToMatrix/");


