//
//	Delay.cc			This file is a part of the IKAROS project
// 					A generic Delay module, used to properly sync inputs from different sources
//					This module is not necessary now that connections can have delays
//
//    Copyright (C) 2002 Jan Moren
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
//	Created: 2002-01-06
//

#ifndef DELAY
#define DELAY


#include "IKAROS.h"

class Delay: public Module
{

public:
    static Module * Create(Parameter * p) {return new Delay(p);}
    
    int 	INSize;
    int 	OUTSize;
    int		delay;

    float *	input;		// in vector to the delay
    float * output;	// out vector - same size as the in vector

    float ** Cache;	// Cache, dynamically allocated.

    int fore, aft;		// indices into the cache

    Delay(Parameter * p) : Module(p) {}
    virtual ~Delay() {}
    
    void Init();
    void Tick();    
    
private:
    void CopyArray(float *from, float *to);
};

#endif
