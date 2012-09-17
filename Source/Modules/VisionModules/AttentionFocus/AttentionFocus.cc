//
//	AttentionFocus.cc   This file is a part of the IKAROS project
//						Module that extracts a focus of attention from an image
//
//    Copyright (C) 2003  Christian Balkenius
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

#include "AttentionFocus.h"

using namespace ikaros;

Module *
AttentionFocus::Create(Parameter * p)
{
    return new AttentionFocus(p);
}



AttentionFocus::AttentionFocus(Parameter * p):
        Module(p)
{
    output_radius   = GetIntValue("output_radius", 15);
    scale			= GetBoolValue("scale", true);
    mask            = GetBoolValue("mask", false);

    AddInput("FOCUS");
    AddInput("INPUT");
    AddOutput("OUTPUT", output_radius*2+1, output_radius*2+1);

    input		= NULL;
    output		= NULL;
    focus		= NULL;
}



void
AttentionFocus::Init()
{
    inputsize_x	 = GetInputSizeX("INPUT");
    inputsize_y	 = GetInputSizeY("INPUT");

    outputsize_x	 = GetOutputSizeX("OUTPUT");
    outputsize_y	 = GetOutputSizeY("OUTPUT");

    if (GetInputSize("FOCUS") != 2)
        Notify(msg_warning, "Input \"FOCUS\" of module \"%s\" should be of size 2 x 1.\n", GetName());

    focus		= GetInputArray("FOCUS");
    input 		= GetInputMatrix("INPUT");
    output		= GetOutputMatrix("OUTPUT");
}



AttentionFocus::~AttentionFocus()
{
//	delete [] input;
}



void
AttentionFocus::Tick()
{
    int center_x;
    int center_y;

    if (!scale)
    {
        center_x = int(focus[0]);
        center_y = int(focus[1]);
    }
    else
    {
        center_x = int(inputsize_x*focus[0]);
        center_y = int(inputsize_y*focus[1]);
    }

    if(!mask)
    {
        for (int i=0; i<outputsize_x; i++)
            for (int j=0; j<outputsize_y; j++)
            {
                int sx = i+center_x-output_radius;
                int sy = j+center_y-output_radius;

                if (0 < sx && sx < inputsize_x && 0 < sy && sy < inputsize_y)
                    output[j][i] = input[sy][sx] ;
                else
                    output[j][i] = 0;
            }
    }
    
    else
    {
        int cx = outputsize_x/2;
        int cy = outputsize_y/2;
        int r2 = cx*cy;
        for (int i=0; i<outputsize_x; i++)
            for (int j=0; j<outputsize_y; j++)
            {
                int sx = i+center_x-output_radius;
                int sy = j+center_y-output_radius;

                if (0 < sx && sx < inputsize_x && 0 < sy && sy < inputsize_y && sqr(i-cx)+sqr(j-cy) < r2)
                    output[j][i] = input[sy][sx] ;
                else
                    output[j][i] = 0;
            }
    }
}


