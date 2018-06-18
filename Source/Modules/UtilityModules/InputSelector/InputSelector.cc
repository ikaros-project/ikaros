//
//	InputSelector.cc		This file is a part of the IKAROS project
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

#include "InputSelector.h"
#include <sstream>
#include <string>
#include <iostream>
// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;

// TODO add check that the two inputs are the same size
InputSelector::InputSelector(Parameter *p):
    Module(p)
{
    Bind(inputs, "inputs");

    for(int i=0; i<inputs; ++i)
    {
        std::string ix_str = inputs > 1 ? std::to_string(i+1) : "";
        std::string str = "INPUT" + ix_str;
        //std::cout << "added " << str << "\n";
        AddInput(str.c_str());
    }
    AddInput("SELECT");
    AddOutput("OUTPUT");
}

void
InputSelector::SetSizes()
{
    std::string ix_str = inputs > 1 ? std::to_string(1) : "";
    std::string str = "INPUT" + ix_str;
    int sx = GetInputSizeX(str.c_str());
    int sy = GetInputSizeY(str.c_str());
    if (sx == unknown_size || sy == unknown_size)
        return;
    
    SetOutputSize("OUTPUT", sx, sy);

    // check that all inputs are same size
    for (int i = 2; i <= inputs; ++i)
    {
        std::stringstream ss(std::stringstream::in | std::stringstream::out);
        ss << "INPUT" << i;

        int x = GetInputSizeX(ss.str().c_str());
        int y = GetInputSizeY(ss.str().c_str());
        if (x == unknown_size || y == unknown_size)
            return;
        if (x != sx || y != sy)
            Notify(msg_fatal_error, "Input streams have different sizes: %i, %i vs %i, %i ; must have same size.",
                sx, sy, x, y);
        sx = x;
        sy = y;
    }
}

void
InputSelector::Init()
{
    
	Bind(debugmode, "debug");    

    select = GetInputArray("SELECT");
    
    std::string ix_str = inputs > 1 ? std::to_string(1) : "";
    std::string str = "INPUT" + ix_str;
    input_size_x = GetInputSizeX(str.c_str());
    input_size_y = GetInputSizeY(str.c_str());

    
    
    for(int i = 0; i < inputs; i++)
    {
        ix_str = inputs > 1 ? std::to_string(i + 1) : "";
        str = "INPUT" + ix_str;
        //input_matrix[i] = GetInputMatrix(str.c_str());
        input_matrix.push_back(GetInputMatrix(str.c_str()));
    }
    
    output_matrix = GetOutputMatrix("OUTPUT");
}



InputSelector::~InputSelector()
{
    // Destroy data structures that you allocated in Init.
    
}



void
InputSelector::Tick()
{
    int sel_ix = (int)select[0];
    if (sel_ix >= 0 && sel_ix <= inputs)
        copy_matrix(output_matrix, input_matrix.at(sel_ix), input_size_x, input_size_y);
    else
        Notify(msg_fatal_error, "InputSelector: got invalid selection index: %i.", sel_ix);
    if(debugmode)
	{
		// print out debug info
	}
}



// Install the module. This code is executed during start-up.

static InitClass init("InputSelector", &InputSelector::Create, "Source/Modules/UtilityModules/InputSelector/");


