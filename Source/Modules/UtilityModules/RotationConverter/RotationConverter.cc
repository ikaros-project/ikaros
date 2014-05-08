//
//	RotationConverter.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2014 Birger Johansson
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
//  This module converts between Euler angles, matrixes and quaternion
//

#include "RotationConverter.h"

#define size_xyz 3
#define size_xyzaxayaz 6
#define size_axayaz 3
#define size_matrix 16
#define size_quaternion 16

using namespace ikaros;


void
RotationConverter::Init()
{
    inputMatrix = GetInputMatrix("INPUT");
    outputMatrix = GetOutputMatrix("OUTPUT");
    
    // Do a few input checks...
    switch (inputFormat) {
        case 0: // xyz
            if (inputMatrixSizeX != size_xyz)
                Notify(msg_fatal_error,"Input size is not correct (xyz). Check your connections");
            break;
        case 1: // xyzaxayaz
            if (inputMatrixSizeX != size_xyzaxayaz)
                Notify(msg_fatal_error,"Input size is not correct (xyzaxayaz). Check your connections");
            break;
        case 2: // axayaz
            if (inputMatrixSizeX != size_axayaz)
                Notify(msg_fatal_error,"Input size is not correct (axayaz). Check your connections");
            break;
        case 3: // matrix
            if (inputMatrixSizeX != size_matrix)
                Notify(msg_fatal_error,"Input size is not correct (matrix). Check your connections");
            break;
        case 4: // quaternion
            // Not implemented yet....
            break;
        default:
            break;
    }
}

void
RotationConverter::SetSizes()
{
    // Getting parameters early as we will be using these to set output size
    inputFormat     =   GetIntValueFromList("input_format");
    outputFormat    =   GetIntValueFromList("output_format");
    angleUnit       =   GetIntValueFromList("angle_unit");

    // Find lenght of input
    inputMatrixSizeX = GetInputSizeX("INPUT");
    inputMatrixSizeY = GetInputSizeY("INPUT");
    
    if (inputMatrixSizeY != unknown_size)
    {
        printf("input_format %i\n",inputFormat);
        printf("output_format %i\n",outputFormat);
        
        // Set output size
        outputMatrixSizeY = inputMatrixSizeY;
        switch (outputFormat) {
            case 0: // xyz
                outputMatrixSizeX = size_xyz;
                break;
            case 1: // xyzaxayaz
                outputMatrixSizeX = size_xyzaxayaz;
                break;
            case 2: // axayaz
                outputMatrixSizeX = size_axayaz;
                break;
            case 3: // matrix
                outputMatrixSizeX = size_matrix;
                break;
            case 4: // quaternion
                // Not implemented yet ...
                break;
            default:
                break;
        }
        SetOutputSize("OUTPUT", outputMatrixSizeX,outputMatrixSizeY);
    }
}

RotationConverter::~RotationConverter()
{}

void
RotationConverter::Tick()
{

    // Convert any input to a matrix
    for (int i = 0; i < inputMatrixSizeY; i++)
    {
        h_eye(m);

        // Convert input to matrix
        switch (inputFormat) {
            case 0: // xyz
                m[3] = inputMatrix[i][0];
                m[7] = inputMatrix[i][1];
                m[11] = inputMatrix[i][2];
                break;
            case 1: // xyzaxayaz
                h_rotation_matrix(m,  angle_to_angle(inputMatrix[i][3], angleUnit, radians), angle_to_angle(inputMatrix[i][4], angleUnit, radians), angle_to_angle(inputMatrix[i][5], angleUnit, radians));
                m[3] = inputMatrix[i][0];
                m[7] = inputMatrix[i][1];
                m[11] = inputMatrix[i][2];
                break;
            case 2: // axayaz
                h_rotation_matrix(m, angle_to_angle(inputMatrix[i][0], angleUnit, radians), angle_to_angle(inputMatrix[i][1], angleUnit, radians), angle_to_angle(inputMatrix[i][2], angleUnit, radians));
                break;
            case 3: // matrix
                break;
            case 4: // quaternion
                // Not implemented yet ...
                break;
            default:
                break;
        }
    
        // Convert the matrix to wanted output
        float ax,ay,az;
        switch (outputFormat) {
            case 0: // xyz
                outputMatrix[i][0] = m[3];
                outputMatrix[i][1] = m[7];
                outputMatrix[i][2] = m[11];
                break;
            case 1: // xyzaxayaz
                h_get_euler_angles(m, ax, ay, az);
                outputMatrix[i][3] = angle_to_angle(ax, radians, angleUnit);
                outputMatrix[i][4] = angle_to_angle(ay, radians, angleUnit);
                outputMatrix[i][5] = angle_to_angle(az, radians, angleUnit);
                outputMatrix[i][0] = m[3];
                outputMatrix[i][1] = m[7];
                outputMatrix[i][2] = m[11];
                break;
            case 2: // axayaz
                h_get_euler_angles(m, ax, ay, az);
                outputMatrix[i][0] = angle_to_angle(ax, radians, angleUnit);
                outputMatrix[i][1] = angle_to_angle(ay, radians, angleUnit);
                outputMatrix[i][2] = angle_to_angle(az, radians, angleUnit);
                break;
            case 3: // matrix
                h_copy(outputMatrix[i], m);
                break;
            case 4: // quaternion
                // Not implemented yet....
                break;
            default:
                break;
        }
    }
}

static InitClass init("RotationConverter", &RotationConverter::Create, "Source/Modules/UtilityModules/RotationConverter/");


