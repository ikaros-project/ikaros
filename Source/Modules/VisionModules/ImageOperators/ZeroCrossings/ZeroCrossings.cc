//
//	ZeroCrossings.cc	This file is a part of the IKAROS project
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

#include "ZeroCrossings.h"
#include "ctype.h"


void
ZeroCrossings::Init()
{
    size_x	 = GetOutputSizeX("OUTPUT");
    size_y	 = GetOutputSizeY("OUTPUT");

    input    = GetInputMatrix("INPUT");
    output   = GetOutputMatrix("OUTPUT");
}



void
ZeroCrossings::Tick()
{
    reset_matrix(output, size_x, size_y);
    for(int i=0; i<size_x-1; i++)
        for(int j=0; j<size_y-1; j++)
        {
            float p = input[j][i];
            if(p*input[j+1][i] <= 0)
                output[j][i] = 1;
            else if(p*input[j][i+1] <= 0)
                output[j][i] = 1;
            else if(p*input[j+1][i+1] <= 0)
                output[j][i] = 1;
        };
}


static InitClass init("ZeroCrossings", &ZeroCrossings::Create, "Source/Modules/VisionModules/ImageOperators/ZeroCrossings/");

