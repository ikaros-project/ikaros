//
//	FanOut.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2012 <Author Name>
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
//  This example is intended as a starting point for writing new Ikaros modules
//  The example includes most of the calls that you may want to use in a module.
//  If you prefer to start with a clean example, use he module MinimalModule instead.
//

#include "FanOut.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;

FanOut::FanOut(Parameter *p):
    Module(p)
{
    Bind(outputs, "outputs");
    size_x = GetIntValue("cell_size_x");
    size_y = GetIntValue("cell_size_y");

    for(int i=1; i<=outputs; ++i)
    {
        std::string ix_str = std::to_string(i);
        std::string str = "OUTPUT" + ix_str;
        AddOutput(str.c_str());
    }
    AddInput("INPUT");
}

void
FanOut::SetSizes()
{
    // Assume all inputs same as #1
    int sx = GetInputSizeX("INPUT");
    int sy = GetInputSizeY("INPUT");
    if(sx == unknown_size || sy == unknown_size)
        return;
    int opsize_x = sx/size_x;
    int opsize_y = sy/size_y;
    for(int i=0; i<outputs; ++i)
    {
        std::string ix_str = std::to_string(i+1);
        std::string str = "OUTPUT" + ix_str;
        SetOutputSize(str.c_str(), opsize_x, opsize_y);
    }
}

void
FanOut::Init()
{
    input_matrix_size_x = GetInputSizeX("INPUT");
    input_matrix_size_y = GetInputSizeY("INPUT");

	Bind(debugmode, "debug");    

    input_matrix = GetInputMatrix("INPUT");
    
    output_matrix = new float**[outputs];
    output_matrix_size_x = GetOutputSizeX("OUTPUT1");
    output_matrix_size_y = GetOutputSizeY("OUTPUT1");

    for(int i=0; i<outputs; ++i)
    {
        std::string ix_str = std::to_string(i+1);
        std::string str = "OUTPUT" + ix_str;
        output_matrix[i] = GetOutputMatrix(str.c_str());
    }
    
}



FanOut::~FanOut()
{
    // Destroy data structures that you allocated in Init.
    
}



void
FanOut::Tick()
{
	if(debugmode)
	{
		// print out debug info
	}
    int op=0;
    for(int jj=0; jj<size_y; ++jj){
        for(int ii=0; ii<size_x; ++ii){
            for(int j= 0; j<output_matrix_size_y; ++j){
                for(int i=0; i<output_matrix_size_x; ++i)
                {
                    int y = j*size_y+jj;
                    int x = i*size_x+ii;
                    output_matrix[op][j][i] = input_matrix[y][x];
                }
            }
            op++;
        }    
    }
}



// Install the module. This code is executed during start-up.

static InitClass init("FanOut", &FanOut::Create, "Source/Modules/UtilityModules/FanOut/");


