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
    
    if(excitation)
        a += beta * sum(excitation, excitation_size);
    
     if(inhibition)
        a -= gamma * sum(inhibition, inhibition_size);

    *output += epsilon * (delta*a - *output); // use internal state later to model more interesting dynamics
}

static InitClass init("Nucleus", &Nucleus::Create, "Source/UserModules/Nucleus/");


