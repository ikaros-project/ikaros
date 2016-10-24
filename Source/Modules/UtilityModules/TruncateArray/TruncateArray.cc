//
//	TruncateArray.cc		This file is a part of the IKAROS project
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


#include "TruncateArray.h"

using namespace ikaros;

void
TruncateArray::Init()
{
    input = GetInputArray("INPUT");
    output = GetOutputArray("OUTPUT");
}

void
TruncateArray::SetSizes()
{

    selction = GetIntArray("selection", selectionSize);
    arrayLength = GetIntValue("array_length");
    
    // Find length of input
    inputSizeX = GetInputSize("INPUT");
    
    // Calculate output size
    if (inputSizeX != unknown_size)
    {
        nrArrays = inputSizeX/arrayLength;
        
        // Override for matrix. Fix this
        //nrArrays = GetInputSizeY("INPUT");
        
        SetOutputSize("OUTPUT", selectionSize);
		
    }

}


void
TruncateArray::Tick()
{
    for (int j = 0; j < selectionSize; j++)
        output[j] = input[selction[j]-1];
}

static InitClass init("TruncateArray", &TruncateArray::Create, "Source/Modules/UtilityModules/TruncateArray/");


