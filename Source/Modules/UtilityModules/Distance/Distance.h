//
//		Distance.h	This file is a part of the IKAROS project
//					Implements a modules that calculates the euclidean distance between its two inputs
//
//    Copyright (C) 2005 Christian Balkenius
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
//	Created: 2005-04-13
//

#ifndef DISTANCE
#define DISTANCE

#include "IKAROS.h"

class Distance: public Module
{
public:
    static Module * Create(Parameter * p) {return new Distance(p); }
    
    int         type;
    float *		input1;
    float *		input2;
    float *		output;
    int			size;

    Distance(Parameter * p) : Module(p) {}
    virtual		~Distance() {}

    void		Init();
    void		Tick();
};

#endif
