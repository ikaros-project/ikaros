//
//	DepthBlobList.cc	This file is a part of the IKAROS project
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

#include "DepthBlobList.h"


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
DepthBlobList::Init()
{
    size_x          = GetInputSizeX("INPUT");
    size_y          = GetInputSizeY("INPUT");

    input           = GetInputMatrix("INPUT");
    output          = GetOutputMatrix("OUTPUT");
    grid            = GetOutputMatrix("GRID");
    background      = GetOutputMatrix("BACKGROUND");
    detection       = GetOutputMatrix("DETECTION");
}



void
DepthBlobList::Tick()
{
    reset_matrix(grid, 100, 100);
    reset_matrix(detection, 100, 100);
    h_reset(*output);

    float sum_x = 0;
    float sum_y = 0;
    float sum_z = 0;
    float n = 0;

    // Fill in grid with hits

    for(int i=0; i<size_x; i++)
        for(int j=0; j<size_y; j++)
        {
            if(input[j][i] > 0)
            {
                float x = float(i);
                float y = float(j);
                float z = input[j][i];

                depth_to_world_coords(x, y, z);

                int grid_x = (int)clip(50+0.025*x, 0, 99);
                int grid_y = (int)clip(0.025*z, 0, 99);

                // Calculate height map

                n += 1;
                sum_x += x;
                sum_y += y;
                sum_z += z;

                // grid[grid_y][grid_x] += 1;

                if(y/10 > grid[grid_y][grid_x])
                    grid[grid_y][grid_x] = y/10;
            }
        }

    // Update background

    for(int j=0; j<100; j++)
        for(int i=0; i<100; i++)
            background[j][i] = (1-alpha)*background[j][i] + (grid[j][i] > 0 ? alpha : 0);

    // calculate detections

    for(int j=0; j<100; j++)
        for(int i=0; i<100; i++)
            if(background[j][i] < bg_threshold)
                detection[j][i] = grid[j][i];

    if(n == 0)
        return;

    h_eye(*output);

    (*output)[3] = sum_z/n;
    (*output)[7] = -sum_x/n;
    (*output)[11] = -sum_y/n;

    multiply(grid, 0.01, 100, 100);
}



static InitClass init("DepthBlobList", &DepthBlobList::Create, "Source/Modules/VisionModules/DepthProcessing/DepthBlobList/");

