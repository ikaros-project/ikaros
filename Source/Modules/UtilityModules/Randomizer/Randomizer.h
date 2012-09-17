//
//	Randomizer.h		This file is a part of the IKAROS project
// 					Implements a modules that outputs a random array
//
//    Copyright (C) 2003 Christian Balkenius
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
//	Created: 2003-07-14
//

#ifndef RADNOMIZER
#define RADNOMIZER

#include "IKAROS.h"

class Randomizer: public Module
{
public:
    float *	output;
    int		outputsize;

    float 	minimum;
    float	maximum;

    Randomizer(Parameter * p) : Module(p) {}
    virtual ~Randomizer() {}

    static Module * Create(Parameter * p) { return new Randomizer(p); }

    void 		Init();
    void 		Tick();

};


#endif
