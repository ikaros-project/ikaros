//
//		 NeuralArray.cc		This file is a part of the IKAROS project
//				
//
//    Copyright (C) 2019 Christian Balkenius
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


#include "NeuralArray.h"

using namespace ikaros;

void
NeuralArray::Init()
{
    Bind(alpha, "alpha");
    Bind(beta, "beta");

    Bind(phi, "phi");
    Bind(chi, "chi");
    Bind(psi, "psi");

    Bind(noise, "noise");
    
    excitation	=	GetInputArray("EXCITATION");
    inhibition	=	GetInputArray("INHIBITION");
    shunting_inhibition	=	GetInputArray("SHUNTING_INHIBITION");
    activity    =    GetOutputArray("ACTIVITY");
    output      =    GetOutputArray("OUTPUT");

    size =	GetInputSize("INHIBITION");
}



void
NeuralArray::Tick()
{
    for(int i=0; i<size; i++)
        activity[i] += phi*excitation[i] - chi*inhibition[i] + alpha * (1-activity[i]) + gaussian(0, noise);
    
    float I = norm(inhibition, size);
    if (I > 0)
        reset_array(output, size);

    // Set output if not set and activity high enough
    float O = norm(output, size);
    if(O < 1)
    {
        int m = arg_max(activity, size);
        if(activity[m] > 0.9)
            output[m] = 1;
    }
}



static InitClass init("NeuralArray", &NeuralArray::Create, "Source/Modules/BrainModels/NeuralArray/");


