//
//	Average.cc		This file is a part of the IKAROS project
//
//
//    Copyright (C) 2004  Christian Balkenius
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


#include "Average.h"

using namespace ikaros;

void
Average::Init()
{
    Bind(type, "type");
    Bind(operation, "operation");
    Bind(window_size, "window_size");
    Bind(alpha, "alpha");
    Bind(termination_criterion, "termination_criterion");
    Bind(select, "select");

    window_size_last = 0;
    window      = NULL;

    size		= GetInputSize("INPUT");
    input		= GetInputArray("INPUT");
    op          = create_array(size);
    sum			= create_array(size);
    output		= GetOutputArray("OUTPUT");
    tick_count	= 0;
}



Average::~Average()
{
    destroy_array(sum);
    destroy_matrix(window);
    destroy_array(op);
}



void
Average::Tick()
{
    // Initializatiom must be here if changed during execution

    if(window_size != window_size_last)
    {
        window_size_last = window_size;
        
        if(window != NULL)
            destroy_matrix(window);
        
        window = create_matrix(window_size, size);
        tick_count = 0;
    }


    switch(operation)
    {
        case 0: // none
            copy_array(op, input, size);
            break;

        case 1: // abs
            abs(op, input, size);
            break;

        case 2: // sqr
            sqr(op, input, size);
            break;

    }

    switch(type)
    {
        case 0: // CMA
            tick_count++;
            add(sum, op, size);
            multiply(output, sum, 1.0/float(tick_count), size);
            break;
        
        case 1: // SMA
            copy_array(window[tick_count], input, size);
        
            for(int j=0; j<size; j++)               // TODO: use set_col function
                window[j][tick_count] = op[j];
            tick_count = (tick_count+1) % window_size;
            mean(output, window, window_size, size);
            break;
        
        case 2: // EMA
            add(output, 1-alpha, output, alpha, op, size);
            break;
    }
    
    if(abs(output[select]) < termination_criterion)
        Notify(msg_terminate, "Average: Terminated because criterion was met.");
}


static InitClass init("Average", &Average::Create, "Source/Modules/UtilityModules/Average/");
