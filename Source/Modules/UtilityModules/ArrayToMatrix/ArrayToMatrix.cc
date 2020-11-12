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

ArrayToMatrix::ArrayToMatrix(Parameter *p):
    Module(p)
{
    Bind(arrayLength, "array_length");
    Bind(channels, "channels");

    for(int i=1; i <= channels; i++)
    {
        std::string ix_str = channels>1?std::to_string(i):"";
        std::string str = "INPUT" + ix_str;
        AddInput(str.c_str());

        str = "OUTPUT" + ix_str;
        AddOutput(str.c_str());
    }
}

void
ArrayToMatrix::Init()
{
    input = new float* [channels];
    output = new float** [channels];
    for(int i=1; i <= channels; i++)
    {
        std::string ix_str = channels>1?std::to_string(i):"";
        std::string str = "INPUT" + ix_str;
        input[i] = GetInputArray(str.c_str());
        str = "OUTPUT" + ix_str;
        output[i] = GetOutputMatrix(str.c_str());    
    }
    
}

void
ArrayToMatrix::SetSizes()
{
    // Getting parameters early as we will be using these to set output size
    arrayLength = GetIntValue("array_length");
    
    // Find length of input
    std::string ix_str = channels>1?std::to_string(1):"";
    std::string str = "INPUT" + ix_str;
    inputSizeX = GetInputSizeX(str.c_str());
    nrArrays = inputSizeX/arrayLength;;
    
    if (inputSizeX != unknown_size)
    {
        for(int i=1; i <= channels; i++)
        {
            ix_str = channels>1?std::to_string(i):"";
            str = "OUTPUT" + ix_str;
            SetOutputSize(str.c_str(), arrayLength, nrArrays);
        }
        
        
    }
}


void
ArrayToMatrix::Tick()
{
    for(int i=0; i < channels; i++)
        copy_array(output[i][0], input[i], inputSizeX);
}

static InitClass init("ArrayToMatrix", &ArrayToMatrix::Create, "Source/Modules/UtilityModules/ArrayToMatrix/");


