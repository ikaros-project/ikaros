//
//		 NeuralField.h		This file is a part of the IKAROS project
//
//
//    Copyright (C) 2018 Christian Balkenius
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

#ifndef  NeuralField_
#define  NeuralField_

#include "IKAROS.h"

class  NeuralField: public Module
{
public:
	static Module * Create(Parameter * p) { return new  NeuralField(p); }
	
    float       alpha;
    float       beta;

    float *     excitation;
    float *     inhibition;
    
    float *     shunting_inhibition;
    float *     activity;
    float *     output;
    
    int         size;

				 NeuralField(Parameter * p) : Module(p) {};
    virtual		~NeuralField() {};

    void		Init();
    void		Tick();
};

#endif
