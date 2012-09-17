//
//	RobertsEdgeDetector.cc		This file is a part of the IKAROS project
//							A module to find edges in an image
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

#include "RobertsEdgeDetector.h"

using namespace ikaros;

Module *
RobertsEdgeDetector::Create(Parameter * p)
{
    return new RobertsEdgeDetector(p);
}



RobertsEdgeDetector::RobertsEdgeDetector(Parameter * p):
    Module(p)
{
    AddInput("INPUT");
    AddOutput("OUTPUT");

    input		= NULL;
    output	= NULL;
}



void
RobertsEdgeDetector::SetSizes()
{
    int sx = GetInputSizeX("INPUT");
    int sy = GetInputSizeY("INPUT");
    if (sx != unknown_size && sy != unknown_size)
        SetOutputSize("OUTPUT", sx-1, sy-1);
}



void
RobertsEdgeDetector::Init()
{
    inputsize_x	 	= GetInputSizeX("INPUT");
    inputsize_y	 	= GetInputSizeY("INPUT");

    outputsize_x	 	= GetOutputSizeX("OUTPUT");
    outputsize_y	 	= GetOutputSizeY("OUTPUT");

    input				= GetInputMatrix("INPUT");
    output			= GetOutputMatrix("OUTPUT");
}



RobertsEdgeDetector::~RobertsEdgeDetector()
{
}



void
RobertsEdgeDetector::Tick()
{
    for (int j =0; j<inputsize_y-1; j++)
        for (int i=0; i<inputsize_x-1; i++)
        {
            float dx = 	input[j][i+1]	- 	input[j+1][i];
            float dy = 	input[j][i]		-	input[j+1][i+1];
            output[j][i] = sqrt(dx*dx+dy*dy);
        }
}



