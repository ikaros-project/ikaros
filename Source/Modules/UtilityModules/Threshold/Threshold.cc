//
//	Threshold.cc		This file is a part of the IKAROS project
//					A module applies a threshold to its input
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


#include "Threshold.h"

void
Threshold::Init()
{
    type		= GetIntValueFromList("type");
    threshold	= GetFloatValue("threshold");

    size_x	= GetInputSizeX("INPUT");
    size_y	= GetInputSizeY("INPUT");

    input   = GetInputMatrix("INPUT");
    output	= GetOutputMatrix("OUTPUT");
}



void
Threshold::Tick()
{
    switch (type)
    {
    case 0: // Binary
        for (int i=0; i<size_x; i++)
            for (int j=0; j<size_y; j++)
                output[j][i] = (input[j][i] > threshold ? 1.0 : 0.0);
        break;

    case 1: // Linear
        for (int i=0; i<size_x; i++)
            for (int j=0; j<size_y; j++)
                output[j][i] = (input[j][i] > threshold ? input[j][i] - threshold : 0.0);
        break;

    default:
        break;
    }
}


