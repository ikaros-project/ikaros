//
//	Softmax.h		This file is a part of the IKAROS project
//				A module applies a softmax function to its input
//
//    Copyright (C) 2004  Christian Balkenius
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

#ifndef SOFTMAX
#define SOFTMAX

#include "IKAROS.h"


class Softmax: public Module
{
public:

    Softmax(Parameter * p) : Module(p) {}
    virtual ~Softmax() {}

    static Module * Create(Parameter * p) { return new Softmax(p); }

    void		Init();
    void		Tick();

    int         size_x;
    int         size_y;

    int         type;
    float		exponent;

    float **	input;
    float **	output;
};


#endif
