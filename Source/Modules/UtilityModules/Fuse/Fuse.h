//
//	Fuse.h		This file is a part of the IKAROS project
//
//    Copyright (C) 2014 Christian Balkenius
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

#ifndef Fuse_
#define Fuse_

#include "IKAROS.h"

class Fuse: public Module
{
public:
    static Module * Create(Parameter * p) { return new Fuse(p); }

    Fuse(Parameter * p);
    virtual ~Fuse();

    void        SetSizes();

    void 		Init();
    void 		Tick();

    char **     input_matrix_name;
    char **     input_object_id_name;
    char **     input_frame_id_name;

    float ***   input_matrix;
    float **    input_object_id;
    float **    input_frame_id;

    float **    matrix;
    float *     object_id;
    float *     frame_id;

    int         size_x;
    int *       input_size_y;
    int         output_size_y;

    int         no_of_inputs;
};

#endif

