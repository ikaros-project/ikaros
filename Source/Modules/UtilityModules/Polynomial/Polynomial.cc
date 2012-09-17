//
//		Polynomial.cc		This file is a part of the IKAROS project
//						Implements a modules that calcuates a polynomal function of its input
//
//    Copyright (C) 2005 Christian Balkenius
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
//	Created: 2005-04-12
//


#include "Polynomial.h"

#include "ctype.h"


void
Polynomial::Init()
{
    // Initialize as one or two dimensional
    
    order = GetIntValue("order", 1);
    c = GetArray("coefficients", order);

    input		=	GetInputArray("INPUT");
    output		=	GetOutputArray("OUTPUT");
    outputsize	=	GetInputSize("INPUT");		// Use one-dimensional representation internally
}



void
Polynomial::Tick()
{
    if (input == NULL)
        return;

    for (int i=0; i<outputsize;i++)
    {
        float x = input[i];
        output[i] = 0;
        for (int cc=0; cc<order; cc++)
        {
            output[i] += c[cc] * x;
            x *= input[i];
        }
    }
}


