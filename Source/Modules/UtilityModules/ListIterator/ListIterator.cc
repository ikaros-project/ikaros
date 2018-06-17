//
//	ListIterator.cc		This file is a part of the IKAROS project
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

#include "ListIterator.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;


void
ListIterator::Init()
{
    Bind(list_length, "list_length");
    list = GetArray("list", list_length);
    Bind(repeat, "repeat");
	Bind(debugmode, "debug");    

    // these should have size == 1
    sync_in = GetInputArray("SYNC IN");
    select = GetInputArray("SELECT");

    sync_out = GetOutputArray("SYNC OUT");
    output_array = GetOutputArray("OUTPUT");
    output_array_size = GetOutputSize("OUTPUT");

    index = 0;
    stop = false;
    sync_sent = false;
}



ListIterator::~ListIterator()
{
    // Destroy data structures that you allocated in Init.
}



void
ListIterator::Tick()
{
    // send a sync signal when starting, lasting 1 tick
    if(index==0 && !sync_sent)
    {
        sync_out[0] = 1.f;
        sync_sent = true;
    }
    else
        sync_out[0] = 0.f;

    if(sync_in[0] && !stop)
    {
        reset_array(output_array, output_array_size);
        if((int)select[0] < output_array_size)
        {
            output_array[(int)select[0]] = list[index];
            int nextix = (index < (list_length-1))? index+1 : 0;
            sync_sent = !(nextix==0 && index > 0);
            index = nextix;
            stop = index==0 && !repeat; 

        }
        else
            printf("ListIterator: error got select=%i with outputsize=%i\n", (int)select[0], output_array_size);
    }

    if(debugmode)
	{
		// print out debug info
        //printf("sync_sent=%i\n", sync_sent);
        // for (int i = 0; i < list_length; ++i)
        // {
        //     printf("list %i = %f\n", i, list[i]); 
        // }
	   printf("list = %1.f at index = %i, select = %.1f, output=[", list[index], index, select[0]);
       for (int i = 0; i < output_array_size; ++i)
           printf("%.1f ", output_array[i] );
       printf("]\n");
       
    }
}



// Install the module. This code is executed during start-up.

static InitClass init("ListIterator", &ListIterator::Create, "Source/Modules/UtilityModules/ListIterator/");


