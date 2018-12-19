//
//	MyModule.h		This file is a part of the IKAROS project
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

#ifndef HeadTracking_
#define HeadTracking_

#include "IKAROS.h"

class HeadTracking: public Module
{
public:
    static Module * Create(Parameter * p) { return new HeadTracking(p); }

    HeadTracking(Parameter * p) : Module(p) {}
    virtual ~HeadTracking();

    void 		Init();
    void 		Tick();

    // pointers to inputs and outputs
    // and integers to represent their sizes

    float       mask_left;
    float       mask_right;
    float       rotation_factor;
    float       angle_factor;

    int         size_x;
    int         size_y;

    float       up_down_angle;
    float       lr_rotation;

    float       current_degree;
    float       target_degree;
    float       t;
    float       sum_head;

    float *		  out_head_angle;
    float *     out_head_rotation;

    float *    object;
    float **	input;
    float **	output;
};

#endif
