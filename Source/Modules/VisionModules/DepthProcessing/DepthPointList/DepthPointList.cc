//
//	DepthPointList.cc	This file is a part of the IKAROS project
//                      Create list of blobs in 3D
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

#include "DepthPointList.h"


using namespace ikaros;

const double xRes = 640;
const double yRes = 480;

const double FOV_h = 1.35; // horizontal field of view, in radians.
const double FOV_v = 1.35; // vertical field of view, in radians.

const double fXToZ = tan(FOV_h/2)*2;
const double fYToZ = tan(FOV_v/2)*2;



static void depth_to_world_coords(float & x, float & y, float & z)
{
    x = (float)((x / xRes - 0.5) * z * fXToZ);
    y = (float)((0.5 - y / yRes) * z * fYToZ);
    // leave z as it is
}



void
DepthPointList::Init()
{
    size_x	 = GetInputSizeX("INPUT");
    size_y	 = GetInputSizeY("INPUT");

    input   = GetInputMatrix("INPUT");
    output  = GetOutputMatrix("OUTPUT");
    grid    = GetOutputMatrix("GRID");
}



void
DepthPointList::Tick()
{
    reset_matrix(grid, 100, 100);
    h_reset(*output);

    float sum_x = 0;
    float sum_y = 0;
    float sum_z = 0;
    float n = 0;

    for(int i=0; i<size_x; i++)
        for(int j=0; j<size_y; j++)
        {
            if(input[j][i] > 0)
            {
                float x = float(i);
                float y = float(j);
                float z = input[j][i];

                depth_to_world_coords(x, y, z);

                n += 1;
                sum_x += x;
                sum_y += y;
                sum_z += z;

                int grid_x = (int)clip(50+0.05*x, 0, 99); // was 0.025
                int grid_y = (int)clip(0.015*z, 0, 99);

                // grid[grid_y][grid_x] += 1;

                if(grid[grid_y][grid_x] < z)
                    grid[grid_y][grid_x] = 0.1;
            }
        }

    if(n == 0)
        return;

    h_eye(*output);

    (*output)[3] = sum_z/n;
    (*output)[7] = -sum_x/n;
    (*output)[11] = -sum_y/n;

    multiply(grid, 0.01, 100, 100);
}



static InitClass init("DepthPointList", &DepthPointList::Create, "Source/Modules/VisionModules/DepthProcessing/DepthPointList/");

