//
//		MatrixMultiply.cc	This file is a part of the IKAROS project
//                          Implements a modules that calculates the matrix product of two inputs
//
//    Copyright (C) 2004 Christian Balkenius
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
//	Created: 2004-03-22
//

#include "ikaros.h"

using namespace ikaros;

class MatrixMultiply : public Module
{
    matrix input1;
    matrix input2;
    matrix output;

    void
    Init()
    {
        Bind(input1, "INPUT1"); // Get the inputs and outputs and bid then to matrices
        Bind(input2, "INPUT2"); // Get the inputs and outputs and bid then to matrices
        Bind(output, "OUTPUT"); // Get the inputs and outputs and bid then to matrices
    }

    void
    Tick()
    {
        // std::cout << path_ << std::endl;
        // input1.info();
        // input1.print();
        // input2.info();
        // input2.print();
        output.matmul(input1, input2);
        //output.info();

        // output.print();
    }
};
INSTALL_CLASS(MatrixMultiply) // Install the class in the Ikaros kernel
