//
//		Nucleus.cc		This file is a part of the IKAROS project
//				
//
//    Copyright (C) 2016 Christian Balkenius
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


#include "Nucleus.h"

using namespace ikaros;

void
Nucleus::Init()
{
    Bind(alpha, "alpha");
    Bind(beta, "beta");
    Bind(gamma, "gamma");
    Bind(delta, "delta");
    Bind(epsilon, "epsilon");

    x = 0;
    
    excitation	=	GetInputArray("EXCITATION");
    inhibition	=	GetInputArray("INHIBITION");
    shunting_inhibition	=	GetInputArray("SHUNTING_INHIBITION");

    excitation_size =	GetInputSize("EXCITATION");
    inhibition_size =	GetInputSize("INHIBITION");
    shunting_inhibition_size =	GetInputSize("SHUNTING_INHIBITION");

    output		=	GetOutputArray("OUTPUT");
}



void
Nucleus::Tick()
{
    float a = alpha;
    float s = 1;
    
    if(shunting_inhibition)
    {
        s = 1/(1+sum(shunting_inhibition, shunting_inhibition_size));
//       printf("%f %f\n", s, sum(shunting_inhibition, shunting_inhibition_size));
    }
    
    if(excitation)
        a += beta * s * sum(excitation, excitation_size);
    
     if(inhibition)
        a -= gamma * sum(inhibition, inhibition_size);

     x += epsilon * (a - x);
    
     *output = atan(x)/atan(1);
}

static InitClass init("Nucleus", &Nucleus::Create, "Source/UserModules/Nucleus/");


