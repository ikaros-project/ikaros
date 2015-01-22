//
//		Sum.h		This file is a part of the IKAROS project
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

#ifndef SUM_
#define SUM_

#include "IKAROS.h"

class Sum: public Module
{
public:
	static Module * Create(Parameter * p) { return new Sum(p); }
	
    float *		input;
    int			size;

    float *     output;

				Sum(Parameter * p) : Module(p) {};
    virtual		~Sum() {};

    void		Init();
    void		Tick();
};

#endif
