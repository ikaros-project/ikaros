//
//	DelayOne.cc		This file is a part of the IKAROS project
//                  Pointless example module delaying its input one tick.
//
//    Copyright (C) 2002 Christian Balkenius
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
//	Created: 2002-01-27
//
//	This module demonstrates how the output size can depend on the input size
//	using the SetSizes function


#include "DelayOne.h"


// This is a function used to create and return an object
// of class DelayOne
// This function is identical for all modules

Module * DelayOne::Create(Parameter * p)
{
    return new DelayOne(p);
}


// This is the creator function
// It should include code to add inputs and outputs to a module

DelayOne::DelayOne(Parameter * p):
        Module(p)
{
    // Add and intput named "INPUT"

    AddInput("INPUT");

    // Add an output to the module names "OUTPUT"
    // The size of the output size not set here
    // The parameter 'unknown_size' can be left out since it is the default
    // It is included here for clarity

    AddOutput("OUTPUT", unknown_size);

    // Set pointers to NULL if something goes wrong during initilization
    // This indicates that no memory has been allocated for these
    // arrays yet

    input = NULL;
    output = NULL;
}



// SetSizes is called during initialization of a network to
// let modules set output sizes based on the sizes of their inputs

void
DelayOne::SetSizes()
{
    // Get the size of the input named "INTPUT"

    int s = GetInputSize("INPUT");

    // Check that the size of the input is set and set the
    // size of the output named "OUTPUT" accordingly

    if (s != unknown_size)
        SetOutputSize("OUTPUT", s);
}


// The function Init is called at start-up when the size of all intputs
// and outputs have been calculated.
// You should allocate any additional memory here.
// This is also a useful place to get the pointers to intput and
// output arrays as well as their sizes.

void
DelayOne::Init()
{
    // Get the sizes of inputs and outputs

    theNoOfInputs	 	= GetInputSize("INPUT");
    theNoOfOutputs	= GetOutputSize("OUTPUT");

    // Do some error checking

    if (theNoOfOutputs == unknown_size)		// This should never happen since the kernel should already have terminated the program
    {
        Notify(msg_fatal_error, "DelayOne: OutputSize could not be resolved\n");
        return;
    }

    if (theNoOfInputs != theNoOfOutputs)		// This should never happen since DelayOne::SetSizes() should make sure this condition is met
    {
        Notify(msg_fatal_error, "DelayOne: Both inputs and outputs need to be of the same size\n");
        return;
    }

    // Get pointers to the input and output arrays
    // so we do not have to do that at each iteration later

    input				= GetInputArray("INPUT");
    output			= GetOutputArray("OUTPUT");
}

// This is the destructor function for the module
// Any additional memory allocated in Init should
// be deallocated here

DelayOne::~DelayOne()
{
}


// Tick is the module function that is called repeatedly during
// execution. It transforms its input to the new input
// In this simple example module it copies the input to the
// output. This will delay the signal by one time step.

void DelayOne::Tick()
{
    for (int i=0; i<theNoOfOutputs; i++)
        output[i] = input[i];
}

