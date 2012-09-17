//
//    Sweep.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2004 Christian Balkenius
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
//	Created: 2004-03-24
//


#include "Sweep.h"


void
Sweep::Init()
{
    minimum =   GetFloatValue("min", 0);
    maximum =   GetFloatValue("max", 1);
    step	=   GetFloatValue("step", 0.1);

    output	=   GetOutputArray("OUTPUT");
    size	=	GetOutputSize("OUTPUT");

    for (int i=0; i<size;i++)
        output[i] = minimum;

    value = minimum;
}



void
Sweep::Tick()
{
    value += step;

    if (value < minimum)
        value = maximum;
    else if (value > maximum)
        value = minimum;

    for (int i=0; i<size;i++)
        output[i] = value;
}


