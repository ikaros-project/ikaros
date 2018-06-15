//
//	MatrixScale.h		This file is a part of the IKAROS project
//
//    Copyright (C) 2015 Trond Arild Tjöstheim
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

#ifndef MatrixScale_
#define MatrixScale_

#include "IKAROS.h"

class MatrixScale: public Module
{
public:
    static Module * Create(Parameter * p) { return new MatrixScale(p); }

    MatrixScale(Parameter * p) : Module(p) {}
    virtual ~MatrixScale();

    void 		Init();
    void 		Tick();

    // pointers to inputs and outputs
    // and integers to represent their sizes

    float **    input_matrix;
    int         input_matrix_size_x;
    int         input_matrix_size_y;
    float *     x_scale;
    float *     y_scale;

    float **    output_matrix;
    int         output_matrix_size_x;
    int         output_matrix_size_y;

    // internal data storage
    ikaros::h_matrix    scale_mat;
    ikaros::h_matrix    trans_mat;
    ikaros::h_matrix    detrans_mat;
    ikaros::h_vector    element;
    ikaros::h_vector    tmp1;
    ikaros::h_vector    tmp2;


    // parameter values
	bool       	debugmode;
};

#endif
