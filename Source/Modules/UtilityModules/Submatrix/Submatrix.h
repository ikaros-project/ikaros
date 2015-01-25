//
//		Submatrix.h		This file is a part of the IKAROS project
//
//
//    Copyright (C) 2013  Christian Balkenius
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

#ifndef Submatrix_
#define Submatrix_

#include "IKAROS.h"



class Submatrix: public Module
{
public:

    Submatrix(Parameter * p) : Module(p) {}
    virtual ~Submatrix() {}
    
    static Module * Create(Parameter * p) { return new Submatrix(p); }

    void        SetSizes();

    void		Init();
    void		Tick();

    int         size_x;
    int         size_y;

    float		offset_x;
    float		offset_y;
    float		direction;

    int         x0;
    int         x1;
    int         y0;
    int         y1;
    int         kernel_size;

    float **	input;
    float *     shift;
    
    float **	output;
};


#endif
