//
//		Shift.cc	This file is a part of the IKAROS project
//				A module that shifts a matrix
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


#include "Shift.h"

using namespace ikaros;


void
Shift::Init()
{
    offset_x		=   GetFloatValue("offset_x", 0);
    offset_y		=   GetFloatValue("offset_y", 0);
    direction		=   GetFloatValue("direction", 1);

    size_x	= GetInputSizeX("INPUT");
    size_y	= GetInputSizeY("INPUT");

    input		= GetInputMatrix("INPUT");
    output	= GetOutputMatrix("OUTPUT");
    shift		= GetInputArray("SHIFT");

    if (shift == NULL)
    {
        Notify(msg_fatal_error, "Shift: SHIFT input not connected\n");
    }
}



void
Shift::Tick()
{
    int dx = int(offset_x + direction * shift[0]);
    int dy = int(offset_y + direction * shift[1]);

    for (int i=0; i<size_x; i++)
        for (int j=0; j<size_y; j++)
            if (0 <= j+dy && j+dy < size_y && 0 <= i+dx && i+dx < size_x)
                output[j][i] = input[j+dy][i+dx];
            else
                output[j][i] = 0;
}


