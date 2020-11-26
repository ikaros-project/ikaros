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
#include <iostream>
using namespace ikaros;

ArrayToMatrix::ArrayToMatrix(Parameter *p):
    Module(p)
{
    input = new float * [channels];
    output = new float ** [channels];
    Bind(arrayLength, "array_length");
    Bind(channels, "channels");

    for(int i=1; i <= channels; i++)
    {
        std::string ix_str = channels>1?"_"+std::to_string(i):"";
        std::string str = "INPUT" + ix_str;
        AddInput(str.c_str());

        str = "OUTPUT" + ix_str;
        AddOutput(str.c_str());
    }
}

ArrayToMatrix::~ArrayToMatrix()
{
    delete[] input;
    delete[] output;
}

void
ArrayToMatrix::SetSizes()
{
    // Getting parameters early as we will be using these to set output size
    arrayLength = GetIntValue("array_length");
    
    // Find length of input
    std::string ix_str = channels>1?"_"+std::to_string(1):"";
    std::string str = "INPUT" + ix_str;
    inputSizeX = GetInputSizeX(str.c_str());
    int sy = GetInputSizeY(str.c_str());
    if(inputSizeX == unknown_size || sy == unknown_size)
        return; // not ready

    nrArrays = inputSizeX/arrayLength;;
    
    if (inputSizeX != unknown_size)
    {
        for(int i=1; i <= channels; i++)
        {
            ix_str = channels>1?"_"+std::to_string(i):"";
            str = "OUTPUT" + ix_str;
            SetOutputSize(str.c_str(), arrayLength, nrArrays);
        }
        
        
    }
}


void
ArrayToMatrix::Init()
{
    Bind(debug_mode, "debug_mode");

    int vcnt = 0;
    for(int i=0; i < channels; i++)
    {
        std::string ix_str = channels>1?"_"+std::to_string(i+1):"";
        std::string str = "INPUT" + ix_str;
        float *tmp = GetInputArray(str.c_str(), false);

        input[i] = GetInputArray(str.c_str(), false);

        if(input[i]!=NULL)
            vcnt++;
        str = "OUTPUT" + ix_str;

        output[i] = GetOutputMatrix(str.c_str());;
    }
    if(vcnt != channels)
        Notify(msg_fatal_error, "All inputs must have connections");

    
}



void
ArrayToMatrix::Tick()
{
    if(debug_mode)
    {    
        for(int i=0; i < channels; i++)
        {
             print_array("input: ", input[i], inputSizeX);
        }
    }

    
    for(int c=0; c < channels; c++)
        copy_array(output[c][0], input[c], inputSizeX);
}

static InitClass init("ArrayToMatrix", &ArrayToMatrix::Create, "Source/Modules/UtilityModules/ArrayToMatrix/");


