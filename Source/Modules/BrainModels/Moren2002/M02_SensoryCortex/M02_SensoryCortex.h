//
//	M02_SensoryCortex.cc		This file is a part of the IKAROS project
//                              Cortex model that does abolutely nothing
//
//    Copyright (C) 2001-2002  Christian Balkenius
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
//	Created: 2001-10-22
//

#include "IKAROS.h"



class M02_SensoryCortex: public Module
{
public:

	M02_SensoryCortex(Parameter * p) : Module(p) {}
	virtual ~M02_SensoryCortex() {}
	
	static Module * Create(Parameter * p) {return new M02_SensoryCortex(p);}

	void Init();
	void Tick();

	int         size;
	float   *	input;
	float   *	output;
};


