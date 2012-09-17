//
//		Shift.h		This file is a part of the IKAROS project
//					A module that shifts a matrix
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

#ifndef SHIFT
#define SHIFT

#include "IKAROS.h"



class Shift: public Module
{
public:

    Shift(Parameter * p) : Module(p) {}
    virtual ~Shift() {}

    static Module * Create(Parameter * p) { return new Shift(p); }

    void		Init();
    void		Tick();

    int         size_x;
    int         size_y;

    float		offset_x;
    float		offset_y;
    float		direction;

    float **	input;
    float **	output;

    float *     shift;
};


#endif
