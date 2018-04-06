//
//	CalibrateCameraPosition.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2015 Birger Johansson
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


#include "CalibrateCameraPosition.h"

using namespace ikaros;

const int marker_id = 16;

void
CalibrateCameraPosition::Init()
{
    marker_number = GetIntValue("marker_number");
    
    int sx = 4;
    int sy = 4;
    int s = sx * sy;
    marker_position = GetArray("marker_position", s); // Set in the ikc file.
    
    input = GetInputMatrix("INPUT");
    size_x = GetInputSizeX("INPUT");
    size_y = GetInputSizeY("INPUT");
}

void
CalibrateCameraPosition::Tick()
{
    
    float marker_c[16];         // Transformation matrix of the marker (in camera coordinates)
    float inv_marker_c[16];     // Transformation matrix of the camera (in marker coordinates)
    float cam_pos[16];          // Transformation matrix of the camera (in marker coordinates relative origo set by marker_position)
    
    h_reset(marker_c);
    printf("%s \n", GetName());
    
    
    // Look for Marker ID
    for (int i = 0; i < size_y; i++) {
        if (input[i][marker_id] == marker_number)
        {
            h_copy(marker_c, *input);
            break;
        }
    }
    
    // No marker found
    if (mean(marker_c,16) == 0)
        return;
    
    float x,y,z;

    h_inv(inv_marker_c,marker_c);
    h_multiply(cam_pos, marker_position, inv_marker_c);
    
    h_get_euler_angles(cam_pos,x,y,z);
    h_print_matrix("Camera in relation to origo", cam_pos);
    printf("Pos (mm) x: %.2f y: %.2f z: %.2f, Rotations (degrees) x %.2f, y %.2f, z %.2f\n",h_get_x(cam_pos),h_get_y(cam_pos),h_get_z(cam_pos), angle_to_angle(x, radians, degrees),angle_to_angle(y, radians, degrees),angle_to_angle(z, radians, degrees));

}


static InitClass init("CalibrateCameraPosition", &CalibrateCameraPosition::Create, "Source/Modules/VisionModules/MarkerTracker/Utilities/CalibrateCameraPosition/");
