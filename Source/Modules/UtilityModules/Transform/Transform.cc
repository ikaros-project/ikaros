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

// Inte klar än. Hur gör man med set size?

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

    parameter input_length;

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

        // Bind(size_y, "size_y");
    }

    void Tick()
    {
       
        // Reset input
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

        // reset_matrix(matrix_out, 16, size_y);
        // reset_array(object_id, size_y);
        // reset_array(frame_id, size_y);

        h_matrix a, b;
        h_matrix m1, m2; // Using h_matrix
        m1.set_name("M1");
        m2.set_name("M2");
        a.set_name("a");
        b.set_name("b");
        int k = 0; // target location in output
        float o1, f1, o2, f2;

        //if (path_ == "ExperimentalSetup.Epi.ForwardModel.PosBody")
        // if (path_ == "ExperimentalSetup.Epi.ForwardModel.BodyHead")
        // {
        //     std::cout << path_ << std::endl;

        //     std::cout << "matrix in size: " << matrix_1.size() << std::endl;
        //     std::cout << "matrix in size: " << matrix_1.size_x() << " " << matrix_1.size_y() << " " << matrix_1.size_z() << std::endl;
        //     // matrix_1.info();

        //     std::cout << "matrix in size: " << matrix_2.size() << std::endl;
        //     std::cout << "matrix in size: " << matrix_2.size_x() << " " << matrix_2.size_y() << " " << matrix_2.size_z() << std::endl;
        //     // matrix_2.info();

        //     matrix_1.print();
        //     matrix_2.print();
        //     // matrix_out.print();
        //     std::cout << "matrix out size: " << matrix_out.size_x() << " " << matrix_out.size_y() << " " << matrix_out.size_z() << std::endl;


        //     // invert_1.print();
        //     // invert_2.print();
        //     // object_id_1.print();
        //     // frame_id_1.print();
        //     // object_id_2.print();
        //     // frame_id_2.print();
        //     // object_id.print();
        //     // frame_id.print();

        //     object_id_1.print();
        //     frame_id_1.print();
        //     object_id_2.print();
        //     frame_id_2.print();
        // }

        // Loop over length of input matrix.
        for (int i = 0; i < matrix_1.size_z(); i++) // Detta kan ju inte funka?
        {
            m1.copy(matrix_1[i]);
            // if (path_ == "ExperimentalSetup.Epi.ForwardModel.PosBody")
            //     m1.print();

            if (m1.is_valid())
            {
                for (int j = 0; j < matrix_2.size_z(); j++)
                {

                    m2.copy(matrix_2[j]);

                    // if (path_ == "ExperimentalSetup.Epi.ForwardModel.PosBody")
                    // {
                    //     m2.print();
                    // }
                    o1 = (invert_1 ? frame_id_1[i] : object_id_1[i]);
                    f1 = (invert_1 ? object_id_1[i] : frame_id_1[i]);

                    o2 = (invert_2 ? frame_id_2[j] : object_id_2[j]);
                    f2 = (invert_2 ? object_id_2[j] : frame_id_2[j]);

                    if (m2.is_valid() && o1 == f2) // matching rule
                    {
                        if (invert_1)
                            a.inv(m1);
                        // h_inv(a, matrix_1[i]);
                        else
                            a.copy(m1);
                        // a.copy(matrix_2[j]); // Test Funkar. Det betyder att argumentet inte kan vara h_matrix. Ska man typcasta denna då?

                        if (invert_2)
                            b.inv(m2);

                        // h_inv(b, matrix_2[j]);
                        else
                            b.copy(m2);

                        // h_copy(b, matrix_2[j]);

                        matrix_out[k].matmul(a, b);

                        // h_multiply(matrix_out[k], a, b);
                        object_id[k] = o2;
                        frame_id[k] = f1;

                        k++;
                    }
                }
            }
        }
        // if (path_ == "ExperimentalSetup.Epi.ForwardModel.BodyHead")
        // {
        //     std::cout << "************* END ***************** " << std::endl;
        //     matrix_1.print();
        //     matrix_2.print();
        //     matrix_out.print();

        //     object_id.print();
        //     frame_id.print();
        // }
    }
};

INSTALL_CLASS(Transform) // Install the class in the Ikaros kernel
