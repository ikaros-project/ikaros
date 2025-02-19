//
//	ServoConnector.cc		This file is a part of the IKAROS project
//
//
//    Copyright (C) 2016 Birger Johansson
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
//	Created: 2016
//

#include "ikaros.h"

using namespace ikaros;

class ServoConnector : public Module
{

    matrix connector;
    matrix PreInverted;
    matrix PostInverted;
    matrix offset;

    matrix input;
    matrix output;

    parameter outputSize;
    void Init()
    {
        // std::cout << "ServoConnector START" << std::endl<< std::endl;

        Bind(connector, "connector");
        Bind(PreInverted, "pre_inverted");
        Bind(PostInverted, "post_inverted");
        Bind(offset, "offset");

        Bind(outputSize, "output_size");

        Bind(input, "INPUT");
        Bind(output, "OUTPUT");


        // Check sizes here!

        // if (!connector.connected())
        //     Notify(msg_fatal_error, "Check connector parameters in ikc file.");

        // if (!PreInverted.connected())
        //     Notify(msg_fatal_error, "Check inverted parameters in ikc file.");

        // if (!offset.connected())
        //     Notify(msg_fatal_error, "Check offset parameters in ikc file.");

        if (outputSize != connector.size())
            Notify(msg_fatal_error, "Output_size does not match connector size");

        if (outputSize != offset.size())
            Notify(msg_fatal_error, "Output_size does not match offset size");

        if (outputSize != PreInverted.size())
            Notify(msg_fatal_error, "Output_size does not match pre_inverted size");

        if (outputSize != PostInverted.size())
            Notify(msg_fatal_error, "Output_size does not match post_inverted size");
        // std::cout << "ServoConnector DONE" << std::endl<< std::endl;
    }
    void Tick()
    {
        //std::cout << "connector" << std::endl;
        //connector.print();
        // std::cout << "input" << std::endl;
        // input.info();
        // input.print();
        // std::cout << "input.size(): " << input.size() << std::endl;
        // std::cout << "input.size_x(): " << input.size_x() << std::endl;

        //std::cout << "output" << std::endl;

        //output.print();

        // Connector
        for (int i = 0; i < input.size(); i++)
        {
            output(i);
            input(connector(i) - 1);
            output(i) = input(connector(i) - 1); // Kolla detta!?
        }
        // Pre Inverted
        for (int i = 0; i < input.size(); i++)
            if (PreInverted(i) == 1)
                output[i] = -output[i];

        // print_array("ServoConnector: output1", output, outputSize);

        // Offset
        for (int i = 0; i < input.size(); i++)
            output(i) = output(i) + offset(i);

        // print_array("ServoConnector: output2", output, outputSize);

        // Post Inverted
        for (int i = 0; i < input.size(); i++)
            if (PostInverted(i) == 1)
                output(i) = -output(i);

        // print_array("ServoConnector: output", output, outputSize);
    }
};
INSTALL_CLASS(ServoConnector) // Install the class in the Ikaros kernel
