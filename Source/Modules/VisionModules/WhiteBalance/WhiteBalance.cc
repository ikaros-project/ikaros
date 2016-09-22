//
//	WhiteBalance.cc	This file is a part of the IKAROS project
//					A module to white balance an image
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
//	2005-01-12 Name changed from VonKries to WhiteBalance
//

#include "WhiteBalance.h"

using namespace ikaros;

void
WhiteBalance::Init()
{
    red_target      =	GetFloatValue("red_target", 255.0);
    green_target	=	GetFloatValue("green_target", 255.0);
    blue_target     =	GetFloatValue("blue_target", 255.0);
    
    reference_x0	=	GetIntValue("x0", 10);
    reference_y0	=	GetIntValue("y0", 10);
    reference_x1	=	GetIntValue("x1", 20);
    reference_y1	=	GetIntValue("y1", 20);
    
    log_x0		=	GetIntValue("log_x0", 0);
    log_y0		=	GetIntValue("log_y0", 0);
    log_x1		=	GetIntValue("log_x1", 0);
    log_y1		=	GetIntValue("log_y1", 0);

    size_x	 	= GetInputSizeX("INPUT0");
    size_y	 	= GetInputSizeY("INPUT0");

    input0		= GetInputMatrix("INPUT0");
    input1		= GetInputMatrix("INPUT1");
    input2		= GetInputMatrix("INPUT2");

    output0		= GetOutputMatrix("OUTPUT0");
    output1		= GetOutputMatrix("OUTPUT1");
    output2		= GetOutputMatrix("OUTPUT2");
}



void
WhiteBalance::Tick()
{
    float	red_ref = 0;
    float	green_ref = 0;
    float	blue_ref = 0;

    // Calculate Reference Colour

    for (int i=reference_x0; i<reference_x1; i++)
        for (int j=reference_y0; j<reference_y1; j++)
        {
            red_ref += input0[j][i];
            green_ref += input1[j][i];
            blue_ref += input2[j][i];
        }

    float area = float(reference_x1-reference_x0) *  float(reference_y1-reference_y0);

    red_ref 	/=	area;
    green_ref /=	area;
    blue_ref 	/=	area;

    // Calculate Color Correction

    for (int i=0; i<size_x; i++)
        for (int j=0; j<size_y; j++)
        {
            output0[j][i] = input0[j][i] * (red_target/red_ref);
            output1[j][i] = input1[j][i] * (green_target/green_ref);
            output2[j][i] = input2[j][i] * (blue_target/blue_ref);
        }

    // Calculate average RGB values for the logged area (before correction)

    if (log_x0 == log_x1 || log_y0 == log_y1)
        return;

    float rr0=0, gg0=0, bb0=0;
    for (int i=log_x0; i<log_x1; i++)
        for (int j=log_y0; j<log_y1; j++)
        {
            rr0 += input0[j][i];
            gg0 += input1[j][i];
            bb0 += input2[j][i];
        }

    float log_area = (log_x1-log_x0)*(log_y1-log_y0);

    rr0 /= log_area;
    gg0 /= log_area;
    bb0 /= log_area;


    // Calculate average RGB values for the logged area (after correction)

    float rr=0, gg=0, bb=0;
    for (int i=log_x0; i<log_x1; i++)
        for (int j=log_y0; j<log_y1; j++)
        {
            rr += output0[j][i];
            gg += output1[j][i];
            bb += output2[j][i];
        }

    rr /= log_area;
    gg /= log_area;
    bb /= log_area;

    Notify(msg_verbose, "WhiteBalance: RGB before and after\t%.2f\t%.2f\t%.2f\t=>\t\t%.2f\t%.2f\t%.2f\n", rr0, gg0, bb0, rr, gg, bb);
}

static InitClass init("WhiteBalance", &WhiteBalance::Create, "Source/Modules/VisionModules/WhiteBalance/");


