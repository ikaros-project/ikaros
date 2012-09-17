//	Delay.cc			This file is a part of the IKAROS project
// 					A generic Delay module, used to properly sync inputs from different sources
//					This module is not necessary now that connections can have delays
//
//    Copyright (C) 2002 Jan Moren
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
//	Created: 2002-11-14
//
//	2003-01-30 	Minor changes	Christian Balkenius
//	2007-02-01 	Minor changes	Christian Balkenius

#include "Delay.h"

#include <cstdlib>

void Delay::CopyArray(float *from, float *to)
{
    for (int i=0; i<INSize; i++) {
        to[i] = from[i];
    }
}



void Delay::Init()
{
    delay = GetIntValue("delay", 1);

    int i;

    fore = 0;
    aft = 0;

    input = NULL;
    output = NULL;

    INSize			 	= GetInputSize("INPUT");
    OUTSize				= GetOutputSize("OUTPUT");

    if (INSize!=OUTSize)
        Notify(msg_fatal_error, "Module \"%s\" (Delay): In (=%d) and output vectors (=%d) must be of the same size\n", GetName(), INSize, OUTSize);

    input				= GetInputArray("INPUT");
    output			= GetOutputArray("OUTPUT");

    if ((Cache = (float **)calloc(delay,sizeof(float *)))==NULL)
        Notify(msg_fatal_error, "Delay.cc: unable to allocate cache memory\n");

    for (i=0; i<delay; i++) {
        if ((Cache[i] = (float *)calloc(INSize,sizeof(float)))==NULL)
            Notify(msg_fatal_error, "Delay.cc: unable to allocate cache sub memory\n");
    }
}


void Delay::Tick()
{
//	int i;

    CopyArray(input, Cache[fore]);
    fore = ((fore+1) % delay);

    if (fore == aft){
        CopyArray(Cache[fore], output);

        aft = ((aft+1) % delay);
    }
}


