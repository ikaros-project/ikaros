//
//		OuterProduct.h		This file is a part of the IKAROS project
//						Implements a modules that calculates the outer product of two inputs
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
//	Created: 2004-03-22
//

#ifndef OUTERPRODUCT
#define OUTERPRODUCT

#include "IKAROS.h"

class OuterProduct: public Module
{
public:
    float *		input1;
    float *		input2;
    float **	output;
    int			size1;
    int			size2;

    OuterProduct(Parameter * p) : Module(p) {}
    virtual		~OuterProduct() {}

    static Module * Create(Parameter * p) { return new OuterProduct(p); }

    void		SetSizes();
    void		Init();
    void		Tick();
};

#endif
