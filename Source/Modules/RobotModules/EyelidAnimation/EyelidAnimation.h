//
//	EyelidAnimation.h		This file is a part of the IKAROS project
//
//    Copyright (C) 2012 <Author Name>
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

#ifndef EyelidAnimation_
#define EyelidAnimation_

#include "IKAROS.h"

class EyelidAnimation: public Module
{
public:
    static Module * Create(Parameter * p) { return new EyelidAnimation(p); }

    EyelidAnimation(Parameter * p) : Module(p) {}
    virtual ~EyelidAnimation();

    void 		Init();
    void 		Tick();

    void shuteye(float* r, float a, int sz);
    int *addmod(int *r, int *a, int inc, int mod, int sz);

    // pointers to inputs and outputs
    // and integers to represent their sizes

    float *     input_array;
    int         input_array_size;
    float *     rotate;

    float *     output_array;
    int         output_array_size;

    // internal data storage
    float *     internal_array;

    // parameter values

    float       size;
    float       light;
    float       dark;
	bool       	debugmode;
};

#endif
