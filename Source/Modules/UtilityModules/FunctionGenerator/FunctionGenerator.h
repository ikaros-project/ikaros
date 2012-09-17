//
//	FunctionGenerator.h		This file is a part of the IKAROS project
//							Generates a function of time
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

#ifndef FUNCTIONGENERATOR
#define FUNCTIONGENERATOR

#include "IKAROS.h"

class FunctionGenerator: public Module
{
public:
    static Module * Create(Parameter * p) { return new FunctionGenerator(p); }

    int		t;
    float *	output;

    int		type;

    float		offset;
    float		amplitude;
    float		frequency;
    float		shift;
    float		duty;

    int		basetime;
    int		tickduty;

    FunctionGenerator(Parameter * p) : Module(p) {}
    virtual ~FunctionGenerator() {}
    
    void 		Init();
    void 		Tick();
};

#endif
