//
//		Nucleus.h		This file is a part of the IKAROS project
//
//
//    Copyright (C) 2015 Christian Balkenius
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

#ifndef Nucleus_
#define Nucleus_

#include "IKAROS.h"

class Nucleus: public Module
{
public:
	static Module * Create(Parameter * p) { return new Nucleus(p); }
	
    float       alpha;
    float       beta;

    float       phi;
    float       chi;
    float       psi;

    float       phi_scale;
    float       chi_scale;
    float       psi_scale;

    bool        scale;
    
    float       epsilon;

    float       x;
    
    float *		excitation;
    float *		inhibition;
    float *     shunting_inhibition;
    
    int			excitation_size;
    int			inhibition_size;
    int			shunting_inhibition_size;

    float *     output;

				Nucleus(Parameter * p) : Module(p) {};
    virtual		~Nucleus() {};

    void		Init();
    void		Tick();
};

#endif
