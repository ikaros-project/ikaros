//
//		MatrixMultiply.cc	This file is a part of the IKAROS project
//                          Implements a modules that calculates the matrix product of two inputs
//
//    Copyright (C) 2004 Christian Balkenius
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
//	Created: 2004-03-22
//


#include "MatrixMultiply.h"

using namespace ikaros;

void
MatrixMultiply::Init()
{
    input1		=	GetInputMatrix("INPUT1");
    input2		=	GetInputMatrix("INPUT2");
    output		=	GetOutputMatrix("OUTPUT");

    size1_x		=	GetInputSizeX("INPUT1");
    size1_y		=	GetInputSizeY("INPUT1");
    size2_x		=	GetInputSizeX("INPUT2");
    size2_y		=	GetInputSizeY("INPUT2");
}



void MatrixMultiply::SetSizes()
{
    int sz1_x = GetInputSizeX("INPUT1");
    int sz1_y = GetInputSizeY("INPUT1");
    int sz2_x = GetInputSizeX("INPUT2");
    int sz2_y = GetInputSizeY("INPUT2");

    if (sz1_x == unknown_size || sz1_y == unknown_size || sz2_x == unknown_size || sz2_y == unknown_size)
        return;

    if (sz1_x != sz2_y)
    {
        Notify(msg_fatal_error, "MatrixMultiply: The sizes of the inputs are not compatible: INPUT1 = [%d, %d] INPUT2 = [%d, %d]\n", sz1_x, sz1_y, sz2_x, sz2_y);
        return;
    }

    SetOutputSize("OUTPUT", sz2_x, sz1_y);
}



void
MatrixMultiply::Tick()
{
    multiply(output, input1, input2, size2_x, size1_y, size1_x);
}


