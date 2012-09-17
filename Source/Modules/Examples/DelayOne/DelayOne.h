//
//	DelayOne.cc		This file is a part of the IKAROS project
// 					Pointless example module delaying its input one tick.
//
//    Copyright (C) 2002 Christian Balkenius
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
//	Created: 2002-01-27
//
//	This module demonstrates how the output size can depend on the input size
//	using the SetSizes function
//

#include "IKAROS.h"

class DelayOne: public Module
{
public:

    DelayOne(Parameter * p);
    virtual ~DelayOne();

    static Module * Create(Parameter * p);

    void		SetSizes();
    void 		Init();
    void 		Tick();

    int 		theNoOfInputs;
    int     	theNoOfOutputs;

    float *	input;
    float *	output;
};

