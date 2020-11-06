//
//	BlackBox.cc		This file is a part of the IKAROS project
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

#include "BlackBox.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;
const std::string cNone = "None";
BlackBox::BlackBox(Parameter *P):
Module(P)
{
    std::vector<std::string>
            insvec = split( std::string(GetValue("ins"))==cNone?"":GetValue("ins"), std::string(";"));
    
    std::vector<std::string> 
        outsvec = split(std::string(GetValue("outs"))==cNone?"":GetValue("outs"), std::string(";"));
    ins = insvec.size()==1 && insvec[0]=="" ? 0 : insvec.size();
    outs = outsvec.size()==1 && outsvec[0]=="" ? 0 : outsvec.size();

    if(ins>0)
    {
        input_name = new char *[ins];
        input = new float **[ins];
    }
    if(outs>0)
    {
        output_name = new char *[outs];
        output = new float **[outs];
    }
    for (int i = 0; i < ins; i++)
        AddInput(input_name[i] = create_formatted_string(insvec.at(i).c_str()));
    for (int i = 0; i < outs; i++)
        AddOutput(output_name[i] = create_formatted_string(outsvec.at(i).c_str()));
    // add other IO channels here
    // AddOutput("OUTPUT");
}

BlackBox::~BlackBox()
{
    for (int i = 0; i < ins; i++)
        destroy_string(input_name[i]);
    for (int i = 0; i < outs; i++)
        destroy_string(output_name[i]);
    delete[] input_name;
    delete[] output_name;
    if(input)
        delete[] input;
    if(output)
        delete[] output;
    destroy_array(internal_array);
}

void 
BlackBox::SetSizes()
{
    int sx = 1;
    int sy = 1;

    // assumes all inputs same size, change if otherwise
    /*
    for (int i = 0; i < ins; i++)
    {
        int sxi = GetInputSizeX(input_name[i]);
        int syi = GetInputSizeY(input_name[i]);

        if (sxi == unknown_size)
            continue; // Not ready yet

        if (syi == unknown_size)
            continue; // Not ready yet

        if (sx != 0 && sxi != 0 && sx != sxi)
            Notify(msg_fatal_error, "Inputs have different sizes");

        if (sy != 0 && syi != 0 && sy != syi)
            Notify(msg_fatal_error, "Inputs have different sizes");

        sx = sxi;
        sy = syi;
    }
    */
    
    if (sx == unknown_size || sy == unknown_size)
        return; // Not ready yet
    for (int i = 0; i < outs; i++)
        SetOutputSize(output_name[i], sx, sy);
    // other sizes here
    size_x=sx;
    size_y=sy;
}

void
BlackBox::Init()
{
    // Bind(parameter, "parameter1");
    Bind(debugmode, "debug");

    for (int i = 0; i < ins; i++)
        input[i] = GetInputMatrix(input_name[i]);
    for (int i = 0; i < outs; i++)
        output[i] = GetOutputMatrix(output_name[i]);

    // other outputs etc
    // io(output_array, output_array_size, "OUTPUT");

    internal_array = create_array(10);
}



void
BlackBox::Tick()
{
    // iterate over inputs and outputs as required
    
    if (debugmode)
    {
        printf("Instance name: %s", this->instance_name);
        // print out debug info
        for (int i = 0; i < ins; i++)
            print_matrix(input_name[i], input[i], size_x, size_y);
    }
}

// Install the module. This code is executed during start-up.

static InitClass init("BlackBox", &BlackBox::Create, "Source/Modules/UtilityModules/BlackBox/");
