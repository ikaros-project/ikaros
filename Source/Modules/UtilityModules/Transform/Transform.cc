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


#include "Transform.h"

using namespace ikaros;


void
Transform::Init()
{
    Bind(invert, "invert");
    
    transformation = GetInputMatrix("TRANSFORMATION");
    input = GetInputMatrix("INPUT");
    
    output = GetOutputMatrix("OUTPUT");
    
    size_x = GetInputSizeX("INPUT");
    size_y = GetInputSizeY("INPUT");
}



void
Transform::Tick()
{

    float t[16];
    
    if(invert)
        h_inv(t, *transformation);
    else
        h_copy(t, *transformation);
    
    copy_matrix(output, input, size_x, size_y); // copy everything to include additional columns after the matrices
    
    for(int i=0; i<size_y; i++)
        if (h_matrix_is_valid(input[i]))
            h_multiply(output[i], t, input[i]);
    
//    print_matrix("trns", output, size_x, size_y);
}


static InitClass init("Transform", &Transform::Create, "Source/UserModules/Transform/");

