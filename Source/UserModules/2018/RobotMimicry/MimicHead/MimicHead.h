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

#ifndef MimicHead_
#define MimicHead_

#include "IKAROS.h"

class MimicHead: public Module
{
public:
    static Module * Create(Parameter * p) { return new MimicHead(p); }

    MimicHead(Parameter * p) : Module(p) {}
    virtual ~MimicHead();

    void 		Init();
    void 		Tick();

    int     t;
    int     row;
    int     max_rows;
    int     input_length;

    //Parameters
    float       baysian_threshold;
    int         max_movements;
    float       outlier_limit_angle;
    float       outlier_limit_rotation;

    float       limit_angle;
    float       limit_rotation;

    bool        load_data;
    bool        movement_in_progress;

    float *     buffer_angle;
    float *     buffer_rotation;

    float *     output_movements_angle;
    float *     output_movements_rotation;

    float *		  out_head_angle;
    float *		  out_head_rotation;

    float *     input_movement;
    float *	    mean_value;
    float *	    variance_value;

    float *		  head_angle_in;
    float *     head_rotation_in;

    float * 	  mean_array;
    float * 	  variance_array;
    float **	  movement_matrix;

    float *     angles_out;

};

#endif
