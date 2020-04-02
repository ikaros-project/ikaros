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
// const char* lstname = "list";

void
ListIterator::SetSizes()
{
    int sx = GetInputSizeX("INPUT");
    int sy = GetInputSizeY("INPUT");
    if(sx == 0 && sy==0)
    {
        float ** m = create_matrix(GetValue("listdata"), sx, sy); // get the sizes but ignore the data
        Notify(msg_warning, "(SetSizes) List is empty");
        destroy_matrix(m);
    }
    SetOutputSize("OUTPUT", sy);
    SetOutputSize("SYNC_OUT", 1);
}


void
ListIterator::Init()
{
    
    Bind(repeat, "repeat");
	Bind(debugmode, "debug");    

    io(input_matrix, input_x, input_y, "INPUT");
    
    if(!input_matrix)
    {        
        list = create_matrix(GetValue("listdata"), list_length_x, list_length_y);
        // TODO fix so bind works
        //Bind(list, list_length_x, list_length_y, "listdata", true);
        if(list_length_x==0 || list_length_y==0)
            Notify(msg_fatal_error, "(Init) List is empty");
        print_matrix("List: ", list, list_length_x, list_length_y);
    }
    else
    {
        list = create_matrix(input_x, input_y);
        list_length_x = input_x;
        list_length_y = input_y;
    }
    
    
    
    // these should have size == 1
    // TODO make this optional
    sync_in = GetInputArray("SYNC_IN");
    select = GetInputArray("SELECT");

    sync_out = GetOutputArray("SYNC_OUT");
    output_array = GetOutputArray("OUTPUT");
    output_array_size = GetOutputSize("OUTPUT");

    index = 0;
    stop = false;
    sync_sent = false;
}



ListIterator::~ListIterator()
{
    // Destroy data structures that you allocated in Init.
    if(input_matrix)
        destroy_matrix(list);
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
        if(input_matrix)
            copy_matrix(list, input_matrix, input_x, input_y);
        int countix = 0;
        if(list_length_y>1)
        {
            copy_array(output_array, list[index], list_length_x);
            countix = list_length_y;
        }
        else
        {
            output_array[0] = list[0][index];
            countix = list_length_x;
        }
        
        int nextix = (index < (countix-1))? index+1 : 0;
        sync_sent = !(nextix==0 && index > 0);
        index = nextix;
        stop = index==0 && !repeat; 
    }

    if(debugmode)
	{
		printf("Instance: %s\n", this->instance_name);
        // print out debug info
        print_matrix("list", list, list_length_x, list_length_y);
	   printf("index = %i\n", index);
       print_array("output: ", output_array, output_array_size);
    }
}



// Install the module. This code is executed during start-up.

static InitClass init("ListIterator", &ListIterator::Create, "Source/Modules/UtilityModules/ListIterator/");


