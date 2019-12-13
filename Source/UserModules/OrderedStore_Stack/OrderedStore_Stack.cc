//
//	OrderedStore_Stack.cc		This file is a part of the IKAROS project
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

#include "OrderedStore_Stack.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;


void
OrderedStore_Stack::Init()
{
    Bind(debugmode, "debug");    

    input_matrix = GetInputMatrix("INPUT");
    size_x = GetInputSizeX("INPUT");
    size_y = GetInputSizeX("INPUT");
    push  = GetInputArray("PUSH");

    output_matrix_1 = GetOutputMatrix("OUTPUT1");
    output_matrix_2 = GetOutputMatrix("OUTPUT2");


}



OrderedStore_Stack::~OrderedStore_Stack()
{
    // Destroy data structures that you allocated in Init.
}



void
OrderedStore_Stack::Tick()
{
    // only save on positive change
    if(push[0] - prevpush > 0)
    {
        // update store
        copy_matrix(output_matrix_2, output_matrix_1, size_x, size_y);
        copy_matrix(output_matrix_1, input_matrix, size_x, size_y);
    }
    prevpush = push[0];
	if(debugmode)
	{
		// print out debug info
	}
}



// Install the module. This code is executed during start-up.

static InitClass init("OrderedStore_Stack", &OrderedStore_Stack::Create, "Source/UserModules/OrderedStore_Stack/");


