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

#include "ikaros.h"

using namespace ikaros;

#define size_xyz 3
#define size_xyzaxayaz 6
#define size_axayaz 3
#define size_matrix 4     // 4x4
#define size_quaternion 4 // 4x4

class RotationConverter : public Module
{
    parameter inputFormat;
    parameter outputFormat;
    parameter size_x;
    parameter size_y;
    parameter size_z;

    matrix inputMatrix;
    matrix outputMatrix;
    // Internally
    h_matrix m;
    int angleUnit; // Only used in xyzaxayaz and axayaz mode.

    void Init()
    {
        Trace("ROTATION Init");

        Bind(inputFormat, "input_format");
        // Bind(outputFormat, "output_format"); // Done in setParameters ()
        Bind(inputMatrix, "INPUT");
        Bind(outputMatrix, "OUTPUT");

      
        // Do a few input checks...
        switch (inputFormat.as_int())
        {
        case 0: // xyz
            if (inputMatrix.size_x() != size_xyz)
                Error("Input size is not correct (xyz). Check your connections", path_);
            break;
        case 1: // xyzaxayaz
            if (inputMatrix.size_x() != size_xyzaxayaz)
                Error("Input size is not correct (xyzaxayaz). Check your connections", path_);
            break;
        case 2: // axayaz
            if (inputMatrix.size_x() != size_axayaz)
                Error("Input size is not correct (axayaz). Check your connections", path_);
            break;
        case 3: // matrix
            if (inputMatrix.size_x() != size_matrix || inputMatrix.size_y() != size_matrix)
                Error("Input size is not correct (matrix). Check your connections", path_);
            break;
        case 4: // quaternion
            // Not implemented yet....
            break;
        default:
            break;
        }
        Trace("ROTATION Init Done");
    }

    void SetParameters()
    {
        // Output size depend on output_format parameter.

        Bind(size_x, "size_x");
        Bind(size_y, "size_y");

        Bind(outputFormat, "output_format");                // Not working as parameter is not resolved in setParameter.
        std::string outputType = GetValue("output_format"); // Get the value manually

        // Set the x and y output size.
        if (outputType.compare("xyz") == 0)
            size_x = size_xyz;
        if (outputType.compare("xyzaxayaz") == 0)
            size_x = size_xyzaxayaz;
        if (outputType.compare("axayaz") == 0)
            size_x = size_axayaz;
        size_y = 1;
        if (outputType.compare("matrix") == 0)
            size_x = size_y = size_matrix;
    }

    void Tick()
    {
        if (path_ == "ExperimentalSetup.Epi.ForwardModel.L1_R1_C")
        {
            inputMatrix.print();
        }
        //std::cout << path_ << std::endl;

        //inputMatrix.info();
        
        // std::cout << "outformat "<< outputFormat.as_int() << std::endl;

        
        //   // Cols.
        // std::cout << "input.shape: " << inputMatrix.shape() << std::endl;
        // std::cout << "input.size: " << inputMatrix.size() << std::endl;
        // std::cout << "input.size_x: " << inputMatrix.size_x() << std::endl;
        // std::cout << "input.cols(): " << inputMatrix.cols() << std::endl;
        // // std::cout << "input.size(0): " << input.size(0) << std::endl;

        // // Rows
        // std::cout << "input.size_y: " << inputMatrix.size_y() << std::endl;
        // std::cout << "input.rows(): " << inputMatrix.rows() << std::endl;

        // // Z
        // // std::cout << "input.size(2): " << input.size(2) << std::endl;
        // std::cout << "input.size_z: " << inputMatrix.size_z() << std::endl;
        // // std::cout << "input.size(3): " << input.size(3) << std::endl;
        // std::cout << "Print Tick Done" << std::endl;


        m.eye();

        // Convert inputformat to matrix
        switch (inputFormat.as_int())
        {
        case 0: // xyz
            m(0, 3) = inputMatrix(0);
            m(1, 3) = inputMatrix(1);
            m(2, 3) = inputMatrix(2);
            break;
        case 1: // xyzaxayaz
            m.set_rotation_matrix(angle_to_angle(inputMatrix(3), angleUnit, radians), angle_to_angle(inputMatrix(4), angleUnit, radians), angle_to_angle(inputMatrix(5), angleUnit, radians));
            m(0, 3) = inputMatrix(0);
            m(1, 3) = inputMatrix(1);
            m(2, 3) = inputMatrix(2);
            break;
        case 2: // axayaz
            m.set_rotation_matrix(angle_to_angle(inputMatrix(0), angleUnit, radians), angle_to_angle(inputMatrix(1), angleUnit, radians), angle_to_angle(inputMatrix(2), angleUnit, radians));
            break;
        case 3: // matrix
            m.copy(inputMatrix); // Get a subset of input matrix.
            break;
        case 4: // quaternion
            // Not implemented yet ...
            break;
        default:
            break;
        }

        // Convert the matrix to wanted output
        float ax, ay, az;
        switch (outputFormat.as_int())
        {
        case 0: // xyz
            m(0, 3);
            outputMatrix(0, 0) = m(0, 3);
            outputMatrix(0, 1) = m(1, 3);
            outputMatrix(0, 2) = m(2, 3);
            break;
        case 1: // xyzaxayaz
            m.get_euler_angles(ax, ay, az);
            outputMatrix(0, 3) = angle_to_angle(ax, radians, angleUnit);
            outputMatrix(0, 4) = angle_to_angle(ay, radians, angleUnit);
            outputMatrix(0, 5) = angle_to_angle(az, radians, angleUnit);
            outputMatrix(0, 0) = m(0, 3);
            outputMatrix(0, 1) = m(1, 3);
            outputMatrix(0, 2) = m(2, 3);
            break;
        case 2: // axayaz
            m.get_euler_angles(ax, ay, az);
            outputMatrix(0, 0) = angle_to_angle(ax, radians, angleUnit);
            outputMatrix(0, 1) = angle_to_angle(ay, radians, angleUnit);
            outputMatrix(0, 2) = angle_to_angle(az, radians, angleUnit);
            break;
        case 3: // matrix
            //outputMatrix.copy(m); // how can i get this to work?
            for (size_t j = 0; j < 4; j++)
                for (size_t k = 0; k < 4; k++)
                    outputMatrix(j, k) = m(j, k);

            break;
        case 4: // quaternion
            // Not implemented yet....
            break;
        default:
            break;
        }
        //outputMatrix.print();
    }
};

INSTALL_CLASS(RotationConverter) // Install the class in the Ikaros kernel
