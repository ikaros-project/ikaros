//
//	DepthTransform.cc	This file is a part of the IKAROS project
//
//
//    Copyright (C) 2014  Christian Balkenius
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

#include "DepthTransform.h"



using namespace ikaros;



static inline void depth_to_sensor_coords(float & x, float & y, float & z, double xRes, double yRes, double fXToZ, double fYToZ)
{
    // compensate for perspective

    float tx = (float)((x / xRes - 0.5) * z * fXToZ);
    float ty = (float)((0.5 - y / yRes) * z * fYToZ);
    float tz = z;

    // shift to sensor coordinate system
    // x is pointing forwards and y to the side; z is up

    x = tz;
    y = -tx;
    z = -ty;
}



void
DepthTransform::Init()
{
    Bind(x_res, "x_res");
    Bind(y_res, "y_res");
    
    Bind(fov_h, "fov_h");
    Bind(fov_v, "fov_v");

    size_x          = GetInputSizeX("INPUT");
    size_y          = GetInputSizeY("INPUT");

    input           = GetInputMatrix("INPUT");
    output          = GetOutputMatrix("OUTPUT");
}



void
DepthTransform::Tick()
{
    // recalculate if parameter have changed
    
    double fXToZ = tan(fov_h/2)*2;
    double fYToZ = tan(fov_v/2)*2;

    reset_matrix(output, size_x, size_y);

    for(int j=0; j<size_y; j++)
        if(h_matrix_is_valid(input[j]))
        {
            float x = h_get_x(input[j]);
            float y = h_get_y(input[j]);
            float z = h_get_z(input[j]);

            depth_to_sensor_coords(x, y, z, x_res, y_res, fXToZ, fYToZ);
            
            h_eye(output[j]);
            output[j][3] = x;
            output[j][7] = y;
            output[j][11] = z;
        }
}



static InitClass init("DepthTransform", &DepthTransform::Create, "Source/Modules/VisionModules/DepthProcessing/DepthTransform/");

