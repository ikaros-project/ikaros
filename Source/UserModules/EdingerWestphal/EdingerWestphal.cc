//
//	EdingerWestphal.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2016 Christian Balkenius & Birger Johansson
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
//    See http://www.ikaros-project.org/ for more information.
//


#include "EdingerWestphal.h"

using namespace ikaros;

void
EdingerWestphal::Init()
{
    Bind(alpha, "alpha");
    Bind(beta, "beta");
    Bind(gamma, "gamma");
    Bind(delta, "delta");
    Bind(epsilon1, "epsilon1");
    Bind(epsilon2, "epsilon2");
    
    input_ipsi = GetInputArray("INPUT_IPSI");
    input_contra = GetInputArray("INPUT_CONTRA");
    input_cb = GetInputArray("INPUT_CB");
    inhibition = GetInputArray("INHIBITION");
    shunting_inhibition = GetInputArray("SHUNTING_INHIBITION");
    
    size_ipsi = GetInputSize("INPUT_IPSI");
    size_contra = GetInputSize("INPUT_CONTRA");
    size_cb = GetInputSize("INPUT_CB");
    size_inhibition = GetInputSize("INHIBITION");
    size_shunting_inhibition = GetInputSize("SHUNTING_INHIBITION");

    output = GetOutputArray("OUTPUT");
}



void
EdingerWestphal::Tick()
{
    float a = alpha + beta * (0.5*sum(input_ipsi, size_ipsi) + 0.5*sum(input_contra, size_contra));
    float d = (delta*a - *output);

    if(d > 0)
        *output += epsilon1 * d;
    else
        *output += epsilon2 * d;
}



static InitClass init("EdingerWestphal", &EdingerWestphal::Create, "Source/UserModules/EdingerWestphal/");


