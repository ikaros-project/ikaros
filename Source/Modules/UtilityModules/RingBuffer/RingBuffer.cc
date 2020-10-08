//
//	RingBuffer.cc		This file is a part of the IKAROS project
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

#include "RingBuffer.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;

void RingBuffer::SetSizes()
{
    buffersize = string_to_int( GetValue("buffersize"));
    buffersize = buffersize <= 0 ? 1 : buffersize;
    //Bind(size, "size");
    
    int inpsz = GetInputSize("INPUT");
    
    SetOutputSize("OUTPUT", buffersize, inpsz);
}

void
RingBuffer::Init()
{
    // Bind(size, "size"); // TODO: minimum is 1, edge case
    // size = size<=0 ? 1 : size;
	Bind(debugmode, "debug");    

    io(input_array, input_array_size,"INPUT");
    
    io(output_matrix, output_x, output_y, "OUTPUT");

    // store time as rows internally
    internal_matrix = create_matrix(input_array_size, buffersize);
    
    head=0;
    tail=head+1;
}



RingBuffer::~RingBuffer()
{
    // Destroy data structures that you allocated in Init.
    destroy_matrix(internal_matrix);
}



void
RingBuffer::Tick()
{
    //
    // note 2020-05-06: want to output by column (time is col), not row
    // but may have to store as by row and transpose on output
    // since copy array method uses y or row indexing
    // float ** tmp = create_matrix(input_array_size, size); // for copying
    // edge case, size=1
    if(buffersize==1)
    {
        copy_array(output_matrix[0], input_array, input_array_size);
        return;
    }
    // copy input to head
    copy_array(internal_matrix[head], input_array, input_array_size);
    //print_matrix("internal matrix", internal_matrix, input_array_size, size);    
    //printf("head: %i, tail: %i\n", head, tail);
    //printf("first start at: %i, size: %i, put at end\n", tail, size - tail);
    //printf("last put at: %i, size: %i\n", (size - tail), head);

    //if (head == 0)
        transpose(output_matrix, internal_matrix, buffersize, input_array_size);
        // copy_matrix(output_matrix, internal_matrix, size, input_array_size);
    // else
    // {
    //     // copy to output; first tail part, from tail to end
    //     //copy_matrix(tmp, internal_matrix + tail, size - tail, input_array_size);
    //     for (int i = 0; i < head; i++)
    //     {
    //         for (int j = tail; j < size; j++)
    //         {
    //             printf("tail i=%i j=%i\n", i, j);
    //             copy_array(tmp[i], internal_matrix[j], input_array_size);
    //             
    //         }   
    //     }
    //     
    //     // copy head part, from 0 to head
    //     //copy_matrix(tmp + (size-tail), internal_matrix, head, input_array_size);
    //     for (int i = head; i < size; i++)
    //     {
    //         for (int j = 0; j < head; j++)
    //         {
    //             printf("head i=%i j=%i\n", i, j);
    //             copy_array(tmp[i], internal_matrix[j], input_array_size);
    //             
    //         }
    //     }
    //     transpose(output_matrix, internal_matrix, size, input_array_size);
    // }
    

	if(debugmode)
	{
		// print out debug info
        printf("Instance: %s\n", this->instance_name);
        printf("head: %i, tail: %i\n", head, tail);
        print_matrix("internal matrix", internal_matrix, input_array_size, buffersize);
        // print_matrix("tmp", tmp, input_array_size, size);
        print_matrix("output", output_matrix, buffersize, input_array_size);
        
	}
    head = (head+1)%buffersize;
    tail = (tail+1)%buffersize;
    // destroy_matrix(tmp);

}



// Install the module. This code is executed during start-up.

static InitClass init("RingBuffer", &RingBuffer::Create, "Source/Modules/UtilityModules/RingBuffer/");


