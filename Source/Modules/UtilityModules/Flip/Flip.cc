//
//		Flip.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2016 Christian Balkenius
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


#include "Flip.h"

using namespace ikaros;

void
Flip::Init()
{
    Bind(type, "type");
    
    input		=	GetInputMatrix("INPUT");
    output		=	GetOutputMatrix("OUTPUT");
    size_x		=	GetInputSizeX("INPUT");
    size_y		=	GetInputSizeY("INPUT");
}



void
Flip::Tick()
{
    if(type == 0)
    {
        for(int j=0; j<size_y; j++)
            copy_array(output[size_y-j-1], input[j], size_x);
    }

    else if(type == 1)
    {
        for(int j=0; j<size_y; j++)
            for(int i=0; i<size_x; i++)
                output[j][size_x-i-1] = input[j][i];
    }
    else // none
    {
        copy_matrix(output, input, size_x, size_y);
    }
}


static InitClass init("Flip", &Flip::Create, "Source/Modules/UtilityModules/Flip/");
