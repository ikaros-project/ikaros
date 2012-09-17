//
//	PrewittEdgeDetector.cc		This file is a part of the IKAROS project
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

#include "PrewittEdgeDetector.h"

using namespace ikaros;


Module *
PrewittEdgeDetector::Create(Parameter * p)
{
    return new PrewittEdgeDetector(p);
}



PrewittEdgeDetector::PrewittEdgeDetector(Parameter * p):
    Module(p)
{
    AddInput("INPUT");
    AddOutput("OUTPUT");

    input		= NULL;
    output	= NULL;
}



void
PrewittEdgeDetector::SetSizes()
{
    int sx = GetInputSizeX("INPUT");
    int sy = GetInputSizeY("INPUT");
    if (sx != unknown_size && sy != unknown_size)
        SetOutputSize("OUTPUT", sx-2, sy-2);
}



void
PrewittEdgeDetector::Init()
{
    inputsize_x	 = GetInputSizeX("INPUT");
    inputsize_y	 = GetInputSizeY("INPUT");

    outputsize_x	= GetOutputSizeX("OUTPUT");
    outputsize_y	= GetOutputSizeY("OUTPUT");

    input			= GetInputMatrix("INPUT");
    output		= GetOutputMatrix("OUTPUT");
}



PrewittEdgeDetector::~PrewittEdgeDetector()
{
}



void
PrewittEdgeDetector::Tick()
{
    float ** in = input;	// alias
    for (int j =1; j<inputsize_y-1; j++)
        for (int i=1; i<inputsize_x-1; i++)
        {
            float dx = 	in[j-1][i-1]	-	in[j-1][i+1]	+	in[j][i-1]	-	in[j][i+1]	+	in[j-1][i-1]	-	in[j+1][i+1];
            float dy = 	in[j-1][i-1]	+	in[j-1][i]	+	in[j-1][i+1]	-	in[j-1][i-1]	-	in[j+1][i]	-	in[j+1][i+1];
            output[j-1][i-1] = sqrt(dx*dx+dy*dy);
        }
}



