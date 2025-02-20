//
//	Transform.cc		This file is a part of the IKAROS project
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

// This module expeCt a Zx4x4 matrix as input.

#include "ikaros.h"

using namespace ikaros;

class Transform : public Module
{
    parameter invert_1;
    parameter invert_2;

    matrix matrix_1;
    matrix object_id_1;
    matrix frame_id_1;

    matrix matrix_2;
    matrix object_id_2;
    matrix frame_id_2;

    matrix matrix_out;
    matrix object_id;
    matrix frame_id;

    void Init()
    {
        Bind(invert_1, "invert_1");
        Bind(invert_2, "invert_2");

        Bind(matrix_1, "MATRIX_1");
        Bind(object_id_1, "OBJECT_ID_1");
        Bind(frame_id_1, "FRAME_ID_1");

        Bind(matrix_2, "MATRIX_2");
        Bind(object_id_2, "OBJECT_ID_2");
        Bind(frame_id_2, "FRAME_ID_2");

        Bind(matrix_out, "MATRIX");
        Bind(object_id, "OBJECT_ID");
        Bind(frame_id, "FRAME_ID");

        if (matrix_1.rank() != 3 && matrix_1.size_x() != 4 && matrix_1.size_y() != 4)
            Notify(msg_fatal_error, "MATRIX_1 must be a Zx4x4 matrix");
        if (matrix_2.rank() != 3 && matrix_2.size_x() != 4 && matrix_2.size_y() != 4)
            Notify(msg_fatal_error, "MATRIX_2 must be a Zx4x4 matrix");
    }
    void Tick()
    {
        // Reset output
        matrix_out.reset();
        object_id.reset();
        frame_id.reset();

        for (int i = 0; i < matrix_out.size_z(); i++) // This is 4x4 matricies. Should we add this to matrix class?
        {
            matrix_out(i, 0, 0) = 1;
            matrix_out(i, 1, 1) = 1;
            matrix_out(i, 2, 2) = 1;
            matrix_out(i, 3, 3) = 1;
        }

        h_matrix a, b;
        h_matrix m1, m2; // Using h_matrix
        m1.set_name("M1");
        m2.set_name("M2");
        a.set_name("a");
        b.set_name("b");
        int k = 0; // target location in output
        float o1, f1, o2, f2;

        // Loop over length of input matrix.
        for (int i = 0; i < matrix_1.size_z(); i++) // this only works if we expect Xx4x4 matricies
        {
            m1.copy(matrix_1[i]);
            if (m1.is_valid())
            {
                for (int j = 0; j < matrix_2.size_z(); j++)
                {
                    m2.copy(matrix_2[j]);

                    o1 = (invert_1 ? frame_id_1[i] : object_id_1[i]);
                    f1 = (invert_1 ? object_id_1[i] : frame_id_1[i]);

                    o2 = (invert_2 ? frame_id_2[j] : object_id_2[j]);
                    f2 = (invert_2 ? object_id_2[j] : frame_id_2[j]);

                    if (m2.is_valid() && o1 == f2) // matching rule
                    {
                        if (invert_1)
                            a.inv(m1);
                        else
                            a.copy(m1);

                        if (invert_2)
                            b.inv(m2);
                        else
                            b.copy(m2);

                        matrix_out[k].matmul(a, b);

                        object_id[k] = o2;
                        frame_id[k] = f1;

                        k++;
                    }
                }
            }
        }
    }
};

INSTALL_CLASS(Transform) // Install the class in the Ikaros kernel
