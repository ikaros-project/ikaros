//
//    Arbiter.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2006-2014 Christian Balkenius
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

#include "Arbiter.h"

using namespace ikaros;


Arbiter::Arbiter(Parameter * p):
    Module(p)
{
    no_of_inputs = GetIntValue("no_of_inputs");

    input_name  = new char * [no_of_inputs];
    value_name  = new char * [no_of_inputs];

    input      = new float * [no_of_inputs];
    value_in   = new float * [no_of_inputs];

    for (int i=0; i<no_of_inputs; i++)
    {
        AddInput(input_name[i] = create_formatted_string("INPUT_%d", i+1));
        AddInput(value_name[i] = create_formatted_string("VALUE_%d", i+1));
    }

    AddOutput("OUTPUT");
    AddOutput("VALUE");
}



Arbiter::~Arbiter()
{
    for (int i=0; i<no_of_inputs; i++)
    {
        destroy_string(input_name[i]);
        destroy_string(value_name[i]);
    }

    delete [] input_name;
    delete [] value_name;

    delete [] input;
    delete [] value_in;
}



void
Arbiter::SetSizes()
{
    int s = 0;

    for(int i=0; i<no_of_inputs; i++)
    {
        int si = GetInputSize(input_name[i]);

        if(si == unknown_size)
            return; // Not ready yet

        if(s != 0 && si != 0 && s != si)
            Notify(msg_fatal_error, "Inputs have different sizes");
        
        s = si;
    }

    SetOutputSize("OUTPUT", s);
    SetOutputSize("VALUE", 1);
}



void
Arbiter::Init()
{
   for(int i=0; i<no_of_inputs; i++)
    {
        input[i] = GetInputArray(input_name[i]);
        value_in[i] = GetInputArray(value_name[i], false);
    }

    output = GetOutputArray("OUTPUT");
    value_out = GetOutputArray("VALUE");

    size = GetOutputSize("OUTPUT");
}



void
Arbiter::Tick()
{
    int   ix = 0;
    float vix = 0;
    
    for(int i=0; i<no_of_inputs; i++)
    {
        float v = (value_in[i] ? *value_in[i] : norm(input[i], size)); // use norm if value input is not connected
        if(v > vix)
        {
            ix = i;
            vix = v;
        }
    }
    
    copy_array(output, input[ix], size);

    *value_out = *value_in[ix];
}



static InitClass init("Arbiter", &Arbiter::Create, "Source/Modules/UtilityModules/Arbiter/");
