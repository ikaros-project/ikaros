//
//	EdingerWestphal.h		This file is a part of the IKAROS project
// 						
//    Copyright (C) 2016 Christian Balkenius & Birger Johansson
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
//    See http://www.ikaros-project.org/ for more information.
//


#ifndef EdingerWestphal_
#define EdingerWestphal_

#include "IKAROS.h"

class EdingerWestphal: public Module
{
public:
    static Module * Create(Parameter * p) { return new EdingerWestphal(p); }

    EdingerWestphal(Parameter * p) : Module(p) {}
    virtual ~EdingerWestphal() {}

    void 		Init();
    void 		Tick();
    
    float       alpha;
    float       beta;
    float       gamma;
    float       delta;
    float       epsilon1;
    float       epsilon2;
    
    float *     input_ipsi;
    float *     input_contra;
    float *     input_cb;
    float *     inhibition;
    float *     shunting_inhibition;
    
    int         size_ipsi;
    int         size_contra;
    int         size_cb;
    int         size_inhibition;
    int         size_shunting_inhibition;

    float *     output;
};

#endif

