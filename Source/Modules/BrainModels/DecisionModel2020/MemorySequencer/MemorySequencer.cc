//
//		MemorySequencer.cc		This file is a part of the IKAROS project
//				
//
//    Copyright (C) 2020 Christian Balkenius
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


#include "MemorySequencer.h"

using namespace ikaros;

void
MemorySequencer::Init()
{
    io(input, size, "INPUT");
    io(output, "OUTPUT");
    last_input = create_array(8);
}


void
MemorySequencer::Tick()
{
    reset_array(output, 80);
        
    if(dist1(input, last_input, size) > 0 || counter > 10) // new input
    {
        counter = 1;
        index = 10*arg_max(input, size);
    }

    if(counter)
    {
        output[index] = 1;
        counter++;
        index++;
    }
    
    copy_array(last_input, input, size);
}

static InitClass init("MemorySequencer", &MemorySequencer::Create, "Source/Modules/BrainModels/DecisionModel2020/MemorySequencer/");


