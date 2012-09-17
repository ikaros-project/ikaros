//
//		MatrixMultiply.h	This file is a part of the IKAROS project
//						Implements a modules that calculates the matrix product of two inputs
//
//    Copyright (C) 2004 Christian Balkenius
///
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
//	Created: 2004-11-15
//

#ifndef MATRIXMULTIPLY
#define MATRIXMULTIPLY

#include "IKAROS.h"

class MatrixMultiply: public Module
{
public:
    float **	input1;
    float **	input2;
    float **	output;
    int			size1_x;
    int			size1_y;
    int			size2_x;
    int			size2_y;

    MatrixMultiply(Parameter * p) : Module(p) {}
    virtual		~MatrixMultiply() {}

    static Module * Create(Parameter * p) { return new MatrixMultiply(p); }

    void		SetSizes();
    void		Init();
    void		Tick();
};

#endif
