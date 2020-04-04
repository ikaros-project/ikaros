//
//	InitialValue.cc		This file is a part of the IKAROS project
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

#include "InitialValue.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;
const int cAccumulate = 0;
const int cCopy = 1;

void 
InitialValue::SetSizes() // Infer output size from data if none is given
{
    if (GetValue("outputsize"))
    {
        Module::SetSizes();
        return;
    }
    int sz;
    float *a = create_array(GetValue("data"), sz);
    //SetInputSize("INPUT", sz);
    SetOutputSize("OUTPUT", sz);
    destroy_array(a);

}
void
InitialValue::Init()
{
    Bind(outputsize, "outputsize");
    Bind(delay, "wait");
    Bind(debugmode, "debug");
    Bind(mode, "mode");
    Bind(internal_array, outputsize, "data");
    

    //input_array = GetInputArray("INPUT");
    //input_array_size = GetInputSize("INPUT");
    io(input_array, input_array_size, "INPUT");
    io(update, "UPDATE");

    output_array = GetOutputArray("OUTPUT");
    output_array_size = GetOutputSize("OUTPUT");
    outputsize = output_array_size;
    if (input_array_size != outputsize)
        Notify(msg_fatal_error, "InitialValue : Input has different size than output.");

    // internal_array = create_array(GetValue("data"), outputsize);
}

InitialValue::~InitialValue()
{
    // Destroy data structures that you allocated in Init.
    destroy_array(internal_array);
}

void
InitialValue::Tick()
{
    // TODO allow on-the-fly updating of values - 
    // if change in data use updated value instead of input

    if(delay<=0)
    {
        // default is to update, if not connected
        if(!update || (update && update[0]==1.f))
        {
            // add input to output
            //copy_array(output_array, input_array, outputsize);
            if(mode==cAccumulate)
                add(output_array, input_array, outputsize);
            else if (mode==cCopy)
                copy_array(output_array, input_array, outputsize);    
        }
    }
    else
    {
        copy_array(output_array, internal_array, outputsize);
        delay--;
    }
    if (debugmode)
    {
        // print out debug info
        print_array("Initialvalue:", output_array, outputsize);
    }
}

// Install the module. This code is executed during start-up.

static InitClass init("InitialValue", &InitialValue::Create, "Source/Modules/UtilityModules/InitialValue/");
