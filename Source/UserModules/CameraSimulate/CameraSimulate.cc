//
//	CameraSimulate.cc		This file is a part of the IKAROS project
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
//  This example is intended as a starting point for writing new Ikaros modules
//  The example includes most of the calls that you may want to use in a module.
//  If you prefer to start with a clean example, use he module MinimalModule instead.
//

#include "CameraSimulate.h"

// use the ikaros namespace to access the math library
// this is preferred to using <cmath>

using namespace ikaros;

void CameraSimulate::Init()
{
    io(target_input, target_input_size, "TARGET");
    io(camera_input, camera_input_size, "CAMERA");

    io(r, output_size, output_size, "RED");
    io(g, output_size, output_size, "GREEN");
    io(b, output_size, output_size, "BLUE");
    io(intensity, output_size, output_size, "INTENSITY");

    Bind(HFOV, "horizontal_field_of_view");
    Bind(VFOV, "vertical_field_of_view");

    Bind(sizeX, "size_x");
    Bind(sizeY, "size_y");

    Bind(targetSize, "target_size");
    Bind(nrOfTargets, "nr_of_targets");
    Bind(target_colors_RGB, cSizeX ,cSizeY,"target_colors_RGB");
}

CameraSimulate::~CameraSimulate()
{
}

void CameraSimulate::Tick()
{
    float ** color;
    color = create_matrix(3,nrOfTargets);
    set_matrix(color,1,3,nrOfTargets);

    // Create a color matrix for targets
    if (cSizeX == 3 and cSizeY == nrOfTargets)
        copy_matrix(color,target_colors_RGB,3,nrOfTargets);


    

    // Reset output
    set_matrix(r, 1, sizeX, sizeY);
    set_matrix(g, 1, sizeX, sizeY);
    set_matrix(b, 1, sizeX, sizeY);
    set_matrix(intensity, 1, sizeX, sizeY);

    // Camera matrix
    h_matrix cam;
    h_create_matrix(cam);
    h_copy(cam, camera_input);
    // Create camera inverse
    // This will give us origo in camera coordinates.
    h_matrix cam_inv;
    h_inv(cam_inv, cam);

    for (int ix = 0; ix < nrOfTargets; ix++)
    {
        // Target matrix
        h_matrix target;
        h_create_matrix(target);
        h_copy(target, &target_input[16*ix]);

        // Multiply
        h_matrix mat;
        h_multiply(mat, cam_inv, target);

        //    X = X
        //    Y = Left/right
        //    Z = Top/bottom

        float x = h_get_x(mat);
        float y = h_get_y(mat);
        float z = h_get_z(mat);

        // Calcualte angle to target
        float angleX = atan2(y, x);
        float angleY = atan2(z, x);

        // Camera angle limitations
        float CameraAngleX = angle_to_angle(HFOV, degrees, radians) / 2;
        float CameraAngleY = angle_to_angle(VFOV, degrees, radians) / 2;

        // print_matrix("Color",target_colors_RGB, 3, 3);

        float r1 = target_colors_RGB[ix][0];
        float g1 = target_colors_RGB[ix][1];
        float b1 = target_colors_RGB[ix][2];

        // Target position in image
        // uses a propotion of FOV. This is a primitive solution.
        int x_ratio = -100; 
        int y_ratio = -100;

        // Angle to target
        if (angleX > -CameraAngleX and angleX < CameraAngleX and angleY > -CameraAngleY and angleY < CameraAngleY)
        {
            x_ratio = (sizeX / 2 - (sizeX / 2 * (angleX / CameraAngleX))); // Why negative???
            y_ratio = (sizeY / 2 - (sizeY / 2 * (angleY / CameraAngleY))); //  Image coordinates
        }

        // printf("CameraAngleX %f\t", angle_to_angle(CameraAngleX, radians, degrees));
        // printf("CameraAngleY %f\t", angle_to_angle(CameraAngleY, radians, degrees));
        // printf("angleX %f\t", angle_to_angle(angleX, radians, degrees));
        // printf("angleY %f\t", angle_to_angle(angleY, radians, degrees));
        // printf("x_ratio %i\t", x_ratio);
        // printf("y_ratio %i\n", y_ratio);

        for (int i = 0; i < targetSize; i++)
            for (int j = 0; j < targetSize; j++)
                if (y_ratio + i > 0 and y_ratio + i < 640)
                    if (x_ratio + j > 0 and x_ratio + j < 640)
                    {
                        r[y_ratio + i][x_ratio + j] = r1;
                        g[y_ratio + i][x_ratio + j] = g1;
                        b[y_ratio + i][x_ratio + j] = b1;
                    }
    }
}

// Install the module. This code is executed during start-up.

static InitClass init("CameraSimulate", &CameraSimulate::Create, "Source/UserModules/CameraSimulate/");
