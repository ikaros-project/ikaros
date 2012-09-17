//
//		Min.h		This file is a part of the IKAROS project
//					Module that minimum each element of its two inputs
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

#ifndef MINCLASS
#define MINCLASS

#include "IKAROS.h"

class Min: public Module
{
public:
    float *		input1;
    float *		input2;
    float *		output;
    int			size;

    Min(Parameter * p) : Module(p) {}
    virtual		~Min() {}

    static Module * Create(Parameter * p){ return new Min(p); }

    void		Init();
    void		Tick();
};

#endif
