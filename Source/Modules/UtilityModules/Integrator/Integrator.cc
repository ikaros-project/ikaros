//
//	Integrator.cc		This file is a part of the IKAROS project
//					A module that integrates its input over time
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


#include "Integrator.h"

void
Integrator::Init()
{
    alpha		= GetFloatValue("alpha");
    beta		= GetFloatValue("beta");
    
    minimum		= GetFloatValue("min");
    maximum		= GetFloatValue("max");
    
    usemin		= GetValue("min") != NULL;
    usemax		= GetValue("max") != NULL;

    size_x      = GetInputSizeX("INPUT");
    size_y      = GetInputSizeY("INPUT");

    input		= GetInputMatrix("INPUT");
    output      = GetOutputMatrix("OUTPUT");
}



void
Integrator::Tick()
{
    for (int i=0; i<size_x; i++)
        for (int j=0; j<size_y; j++)
        {
            output[j][i] = alpha * output[j][i] + beta * input[j][i];
            if (usemin && output[j][i] < minimum)
                output[j][i] = minimum;
            if (usemax && output[j][i] > maximum)
                output[j][i] = maximum;
        }
}


