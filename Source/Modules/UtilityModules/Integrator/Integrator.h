//
//	Integrator.h			This file is a part of the IKAROS project
//						A module that integrates its input over time
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

#ifndef INTEGRATOR
#define INTEGRATOR

#include "IKAROS.h"


class Integrator: public Module
{
public:
    static Module * Create(Parameter * p) { return new Integrator(p); }

    Integrator(Parameter * p) : Module(p) {}
    virtual ~Integrator() {}

    void		Init();
    void		Tick();

    int         size_x;
    int         size_y;

    float		alpha;
    float		beta;

    float		minimum;
    float		maximum;

    bool		usemax;
    bool		usemin;

    float **	input;
    float **	output;
};


#endif
