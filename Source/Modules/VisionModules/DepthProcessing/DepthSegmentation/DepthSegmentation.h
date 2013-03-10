//
//	DepthSegmentation.h		This file is a part of the IKAROS project
//						A module to filter image with a kernel
//
//    Copyright (C) 2002  Christian Balkenius
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

#ifndef DepthSegmentation_
#define DepthSegmentation_

#include "IKAROS.h"


class DepthSegmentation: public Module
{
public:

    DepthSegmentation(Parameter * p) : Module(p) {};
    virtual ~DepthSegmentation() {};

    static Module * Create(Parameter * p) {return new DepthSegmentation(p);};

    void Init();
    void Tick();

    float       mask_left;
    float       mask_right;

    int         size_x;
    int         size_y;

    float *     object;
    float **	input;
    float **	output;
};


#endif

