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

    void Init()
    {
        Bind(connector, "connector");
        Bind(PreInverted, "pre_inverted");
        Bind(PostInverted, "post_inverted");
        Bind(offset, "offset");
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");

        if (input.size() != connector.size())
            Notify(msg_fatal_error, "Check connector parameters in ikc file.");
        if (input.size() != offset.size())
            Notify(msg_fatal_error, "Check offset parameters in ikc file.");
        if (input.size() != PreInverted.size())
            Notify(msg_fatal_error, "Check pre_inverted parameters in ikc file.");
        if (input.size() != PostInverted.size())
            Notify(msg_fatal_error, "Check post_inverted parameters in ikc file.");
    }
    void Tick()
    {
        // Connector
        for (int i = 0; i < input.size(); i++)
            output(i) = input(connector(i) - 1);

        // Pre Inverted
        for (int i = 0; i < input.size(); i++)
            if (PreInverted(i) == 1)
                output(i) = -output(i);

        // Offset
        for (int i = 0; i < input.size(); i++)
            output(i) = output(i) + offset(i);

        // Post Inverted
        for (int i = 0; i < input.size(); i++)
            if (PostInverted(i) == 1)
                output(i) = -output(i);
    }
};
INSTALL_CLASS(ServoConnector) // Install the class in the Ikaros kernel
