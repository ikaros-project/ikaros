//
//	Sum.cc		This file is a part of the IKAROS project
//              Old example for version 1.1 and earlier.
//
//    Copyright (C) 2001-2002 Jan Moren
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
//	Created: 2001-12-01
//
//	2003-01-14 Updated for the new simulator

#include "Sum.h"

Module * Sum::Create(Parameter * p)
{
    return new Sum(p);
}


Sum::Sum(Parameter * p): Module(p)
{

    // AddInput here declares two inputs for the module. These are the same
    // names used in the experiment configuration file.

    AddInput("INPUT1");
    AddInput("INPUT2");

    // Here we declare an output and tell the simulator that its size will be
    // figured out later. The parameter is not strictly necessary as it is the
    // default value, but it is a good idea to include it for clarity.

    AddOutput("OUTPUT", unknown_size);

}

// SetSizes is called during initialization of a network to
// let modules set output sizes based on the sizes of their inputs
//
// It is called repeatedly until all sizes for all modules are fixed or until
// Ikaros discovers it is not possible to set a consistent set of data sizes.

void Sum::SetSizes()
{
    // find out the sizes of the inputs.

    int in1 = GetInputSize("INPUT1");
    int in2 = GetInputSize("INPUT2");

    // now, we require that the input vectors are the same size. So we first
    // check that the sizes are both set, and the compare them to see if
    // they're equal. If not, we abort with an error message.

    if (in1 != unknown_size && in2 != unknown_size){
        if (in1!=in2)
        {
            Notify(msg_fatal_error, "Sum: the input sizes must be equal: INPUT1 = %d\t INPUT2 = %d\n", in1, in2);
            return;
        }

        // Set the size of the output vector to be equal to the size of the
        // input vectors.

        SetOutputSize("OUTPUT", in1);
    }
}


void Sum::Init()
{
    input1 = NULL;
    input2 = NULL;
    output = NULL;

    theNoOfInputs1	= GetInputSize("INPUT1");
    theNoOfInputs2	= GetInputSize("INPUT2");
    theNoOfOutputs	= GetOutputSize("OUTPUT");

    // Here we could add any number of consistency checks and do any memory
    // allocations and other initializations that wer need to do.



    // Set the variables that will be associated with the inputs and outputs
    // during the simulation. Each time the Tick method is entered, the input1
    // and input2 will point to the input vectors, and output will point to
    // the output vector.

    input1		= GetInputArray("INPUT1");
    input2		= GetInputArray("INPUT2");
    output		= GetOutputArray("OUTPUT");
}


Sum::~Sum()
{
}


void Sum::Tick()
{
    int i;

    // Sum in action. input1 and input2 are pointers to the current input
    // vectors and output points to the output vector that will be sent
    // along to whatever modules it is connected to.

    for (i=0; i<theNoOfOutputs; i++) {
        output[i] = input1[i] + input2[i];
    }
}

