//
//	ChangeDetector.cc		This file is a part of the IKAROS project
//						A module to find changes in an image
//
//    Copyright (C) 2006  Christian Balkenius
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

#include "ChangeDetector.h"

using namespace ikaros;

Module *
ChangeDetector::Create(Parameter * p)
{
    return new ChangeDetector(p);
}



ChangeDetector::ChangeDetector(Parameter * p):
        Module(p)
{
    AddInput("INPUT");
    AddOutput("OUTPUT");

    last_input		= NULL;
    input			= NULL;
    output			= NULL;

    border = GetIntValue("border", 0);
}



void
ChangeDetector::SetSizes()
{
    int sx = GetInputSizeX("INPUT");
    int sy = GetInputSizeY("INPUT");
    if (sx != unknown_size && sy != unknown_size)
        SetOutputSize("OUTPUT", sx-2*border, sy-2*border);
}



void
ChangeDetector::Init()
{
    inputsize_x	 	= GetInputSizeX("INPUT");
    inputsize_y	 	= GetInputSizeY("INPUT");

    outputsize_x	 = GetOutputSizeX("OUTPUT");
    outputsize_y	 = GetOutputSizeY("OUTPUT");

    last_input		= create_matrix(inputsize_x, inputsize_y);

    input			= GetInputMatrix("INPUT");
    output			= GetOutputMatrix("OUTPUT");
}



ChangeDetector::~ChangeDetector()
{
}



void
ChangeDetector::Tick()
{
    for (int j =0; j<outputsize_y; j++)
        for (int i=0; i<outputsize_x; i++)
            output[j][i] = abs(input[j+border][i+border] - last_input[j+border][i+border]);

    copy_matrix(last_input, input, inputsize_x, inputsize_y);
}



