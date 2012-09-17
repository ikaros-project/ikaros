//
//		Normalize.h		This file is a part of the IKAROS project
//						A modules that Normalizes its input to the range 0-1
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
//	Created: 2004-03-22
//

#ifndef NORMALIZE
#define NORMALIZE

#include "IKAROS.h"

class Normalize: public Module
{
public:
    float *		input;
    float *		output;
    int			size;

    int         type;

    Normalize(Parameter * p) : Module(p) {}
    virtual		~Normalize() {}

    static Module * Create(Parameter * p) { return new Normalize(p); }

    void		Init();
    void		Tick();
};

#endif

