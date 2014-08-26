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



static inline void depth_to_sensor_coords(float & x, float & y, float & z)
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
DepthBlobList::Init()
{
    Bind(pan, "pan");
    Bind(tilt, "tilt");

    size_x          = GetInputSizeX("INPUT");
    size_y          = GetInputSizeY("INPUT");

    grid_size_x     = GetOutputSizeX("GRID");
    grid_size_y     = GetOutputSizeY("GRID");

    input           = GetInputMatrix("INPUT");
    position        = GetInputMatrix("POSITION", false);

    grid            = GetOutputMatrix("GRID");
    background      = GetOutputMatrix("BACKGROUND");
    dilated_background  = GetOutputMatrix("DILATED_BACKGROUND");
    detection       = GetOutputMatrix("DETECTION");
    smoothed        = GetOutputMatrix("SMOOTHED");

    maxima          = GetOutputMatrix("MAXIMA");
    output          = GetOutputMatrix("OUTPUT");
}



void
DepthBlobList::Tick()
{
    reset_matrix(grid, grid_size_x, grid_size_y);
    reset_matrix(detection, grid_size_x, grid_size_y);
    h_reset(*output);

    // Get rotation and translation

    h_matrix rotation;
    h_matrix translation;

    h_eye(rotation);
    h_eye(translation);

    if(position) // Change coordinate system
    {
        h_copy(rotation, *position);

        rotation[3] = 0; // reset translation
        rotation[7] = 0;
        rotation[11] = 0;

        translation[3] = h_get_x(*position);
        translation[7] = h_get_y(*position);
        translation[11] = h_get_z(*position);
    }

    // Fill in grid with hits

    for(int i=0; i<size_x; i++)
        for(int j=0; j<size_y; j++)
        {
            if(input[j][i] > 0)
            {
                float x = float(i);
                float y = float(j);
                float z = input[j][i];

                depth_to_sensor_coords(x, y, z);

                if(position) // Change coordinate system; rotate according to sensor position
                {
                    h_vector p  = { x, y, z, 1 };
                    h_vector pr = { 0, 0, 0, 0 };

                    h_rotation_matrix(rotation, Y, tilt);

                    float *pp[4];
                    float ** m = h_temp_matrix(rotation, pp);   // TODO: Not correct - the matrix multiplication is probably wrong

                    multiply(pr, m, p, 4, 4);

                    x = pr[0];
                    y = pr[1];
                    z = -pr[2] + h_get_z(translation);  // TODO: Not correct - the matrix multiplication is probably wrong
                }

                int grid_x = (int)clip(grid_size_y/2-0.025*y, 0, grid_size_x-1);    // TODO: Scale automatically
                int grid_y = (int)clip(grid_size_x-0.0125*x, 0, grid_size_y-1);

                // Calculate height map

                 if(z > grid[grid_y][grid_x])
                    grid[grid_y][grid_x] = z;
            }
        }

    // Update background

    float a = alpha;
    if(GetTick() <25)   // habituate for initial 25 frames
        a *= 100;

    for(int j=0; j<grid_size_y; j++)
        for(int i=0; i<grid_size_x; i++)
            background[j][i] = (1-a)*background[j][i] + (grid[j][i] > 0 ? a : 0);

    copy_matrix(dilated_background, background, grid_size_x, grid_size_y);

    const int w = 2;
    for (int j=0; j<grid_size_y; j++)
        for (int i=0; i<grid_size_x; i++)
        {
            float t = background[j][i];
            for (int jj=clip(j-w, 0, grid_size_y); jj<clip(j+w, 0, grid_size_y); jj++)
                for (int ii=clip(i-w, 0, grid_size_x); ii<clip(i+w, 0, grid_size_x); ii++)
                    if (background[jj][ii] > t)
                        t = background[jj][ii];
            dilated_background[j][i] = t;
        }

    // calculate detections

    for(int j=0; j<grid_size_y; j++)
        for(int i=0; i<grid_size_x; i++)
            if(dilated_background[j][i] < bg_threshold)
                detection[j][i] = grid[j][i];

    // smoothing

    reset_matrix(smoothed, grid_size_x, grid_size_y);

    const int ww = 5;
    for (int j=0; j<grid_size_y; j++)
        for (int i=0; i<grid_size_x; i++)
        {
            for (int jj=clip(j-ww, 0, grid_size_y); jj<clip(j+ww, 0, grid_size_y); jj++)
                for (int ii=clip(i-ww, 0, grid_size_x); ii<clip(i+ww, 0, grid_size_x); ii++)
                    smoothed[j][i] += 0.00001*detection[jj][ii]*exp(-0.001*hypot(j-jj, i-ii));
        }

    // find local maxima

    set_matrix(maxima, -1, 2, 10);
    int c = 0;

    for (int j=1; j<grid_size_y-1; j++)
        for (int i=1; i<grid_size_x-1; i++)
        {
            if(smoothed[j][i] > smoothed[j-1][i] && smoothed[j][i] > smoothed[j+1][i] &&
               smoothed[j][i] > smoothed[j][i-1] && smoothed[j][i] > smoothed[j][i+1] &&
               smoothed[j][i] > smoothed[j-1][i-1] && smoothed[j][i] > smoothed[j+1][i+1] &&
               smoothed[j][i] > smoothed[j+1][i-1] && smoothed[j][i] > smoothed[j-1][i+1])
            {
                if(c < 10 && smoothed[j][i] > 0.1)
                {
                    maxima[c][0] = float(i)/float(grid_size_x);
                    maxima[c][1] = float(j)/float(grid_size_y);

                    printf("local maximum: %f %f @ %.4f\n", maxima[c][0], maxima[c][1], smoothed[j][i]);

                    c++;
                }
            }
        }

//    printf("Min = %f, max = %f\n", min(grid, 100, 100), max(grid, 100, 100));
    
    printf("-----------------\n\n");

    reset_matrix(output, 17, 10);

     for(int i=0; i<10; i++)
    {
        if(maxima[i][0] != -1 && maxima[i][1] != -1) // TODO: Final rotation is missing
        {
            h_eye(output[i]);
            output[i][3] = maxima[i][0] + translation[3];
            output[i][7] = maxima[i][1] + translation[7];
            output[i][11] = 1500; // grid[j][i] ?
        }
    }

    multiply(grid, 1.0/max(grid, grid_size_x, grid_size_y), grid_size_x, grid_size_y);

//    print_matrix("grid2", grid, 10, 10);
}



static InitClass init("DepthBlobList", &DepthBlobList::Create, "Source/Modules/VisionModules/DepthProcessing/DepthBlobList/");

