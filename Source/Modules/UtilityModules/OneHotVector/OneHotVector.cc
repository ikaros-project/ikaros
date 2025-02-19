//
//	OneHotVector.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2022 Birger Johansson
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

#include "ikaros.h"

using namespace ikaros;

class OneHotVector : public Module
{
    parameter value;
    matrix input_array;
    matrix output_array;

    void Init()
    {
        Bind(value, "value");
        
        Bind(input_array, "INPUT");
        Bind(output_array, "OUTPUT");
    }

    void Tick()
    {
        if (input_array.size() != 1)
        {
            Notify(msg_fatal_error, "Input size should be 1");
            return;
        }
        if (!input_array.connected())
        {
            Notify(msg_fatal_error, "No input");
            return;
        }
        output_array.reset();
        if (input_array(0) != -1)
            output_array(int(clip(input_array(0), 0, output_array.size_x()))) = value;
    }
};

INSTALL_CLASS(OneHotVector) // Install the class in the Ikaros kernel