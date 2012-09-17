//
//	ColorMatch.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2004  Christian Balkenius, Anders J Johansson
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


#include "ColorMatch.h"

using namespace ikaros;

void
ColorMatch::Init()
{
    alpha		=	GetFloatValue("alpha", 0.01);
    sigma		=	GetFloatValue("sigma", 25.0);
    gain			=	GetFloatValue("gain", 1.0);
    
    target0		=	GetFloatValue("target0", 0.0);
    target1		=	GetFloatValue("target1", 0.0);
    target2		=	GetFloatValue("target2", 0.0);
    
    threshold	=	GetFloatValue("threshold", 0.0);

    size_x	 	= GetInputSizeX("INPUT0");
    size_y	 	= GetInputSizeY("INPUT0");

    input0		= GetInputMatrix("INPUT0");
    input1		= GetInputMatrix("INPUT1");
    input2		= GetInputMatrix("INPUT2");

    if(InputConnected("TARGETINPUT0"))
    {
        target_input0	= GetInputMatrix("TARGETINPUT0");
        target_input1	= GetInputMatrix("TARGETINPUT1");
        target_input2	= GetInputMatrix("TARGETINPUT2");
    }
    
    if(InputConnected("REINFORCEMENT"))
        reinforcement = GetInputArray("REINFORCEMENT");

    if(InputConnected("FOCUS"))
        focus = GetInputArray("FOCUS");

    output		= GetOutputMatrix("OUTPUT");
}



#define sqr(x) (x)*(x)

void
ColorMatch::Tick()
{
    // Normalize Target

    float s = target0+target1+target2;

    if (s != 0)
    {
        target0 /= s;
        target1 /= s;
        target2 /= s;
    }

    // Calculate Color Map

    for (int i=0; i<size_x; i++)
        for (int j=0; j<size_y; j++)
        {
            float cs = input0[j][i]  + input1[j][i]  + input2[j][i] ;
            if (cs > threshold)
            {
                float d = sqrt(sqr(input0[j][i]/cs - target0) + sqr(input1[j][i]/cs -  target1) + sqr(input2[j][i]/cs -  target2));
                output[j][i] = gain*exp(-sigma*d);
            }
            else
            {
                output[j][i] = 0.0;
            }
        }


    // Retune Target

    if (reinforcement != NULL && focus != NULL && alpha != 0)
    {
        float d = alpha * reinforcement[0];
        target0 = (1.0-d)*target0 + d*target_input0[int(focus[1])][int(focus[0])];
        target1 = (1.0-d)*target1 + d*target_input1[int(focus[1])][int(focus[0])];
        target2 = (1.0-d)*target2 + d*target_input2[int(focus[1])][int(focus[0])];
    }
}



