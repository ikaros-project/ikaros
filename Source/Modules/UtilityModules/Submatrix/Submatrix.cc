//
//		Submatrix.cc	This file is a part of the IKAROS project
//
//
//    Copyright (C) 2013  Christian Balkenius
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


#include "Submatrix.h"

using namespace ikaros;


void
Submatrix::SetSizes()
{
    int sx	= GetInputSizeX("INPUT");
    int sy	= GetInputSizeY("INPUT");

    if(GetValue("kernel_size"))
    {
        kernel_size = GetIntValue("kernel_size");

        x0 = (kernel_size-1)/2;
        y0 = (kernel_size-1)/2;
        x1 = sx - (kernel_size-1)/2;
        y1 = sy - (kernel_size-1)/2;
    }
    else
    {
        x0		=   GetIntValue("x0");
        x1		=   GetIntValue("x1");
        y0		=   GetIntValue("y0");
        y1		=   GetIntValue("y1");
    }
    
    if(x1<x0)
        Notify(msg_fatal_error, "Submatrix: Illegal range for x.");

    if(y1<y0)
        Notify(msg_fatal_error, "Submatrix: Illegal range for y.");
    
    SetOutputSize("OUTPUT", x1-x0, y1-y0);
}



void
Submatrix::Init()
{
    offset_x		=   GetFloatValue("offset_x", 0);
    offset_y		=   GetFloatValue("offset_y", 0);
    direction		=   GetFloatValue("direction", 1);

    input		= GetInputMatrix("INPUT");
    shift		= GetInputArray("SHIFT");

    output      = GetOutputMatrix("OUTPUT");

    size_x      = GetOutputSizeX("OUTPUT");
    size_y      = GetOutputSizeY("OUTPUT");
}



void
Submatrix::Tick()
{
    if(shift)
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
    else
    {
        for(int i=0; i<size_x; i++)
            for(int j=0; j<size_y; j++) // TODO: This loop can be optimized  using copy array for larger matrices
                output[j][i] = input[j+x0][i+y0];
    }
}


static InitClass init("Submatrix", &Submatrix::Create, "Source/Modules/UtilityModules/Submatrix/");
