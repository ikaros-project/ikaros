//
//		Constant.cc		This file is a part of the IKAROS project
//						Implements a modules that outputs a constant array
//
//    Copyright (C) 2004-2016 Christian Balkenius
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


#include "Constant.h"

void
Constant::SetSizes() // Infer output size from data if none is given
{
    if(GetValue("outputsize"))
    {
        Module::SetSizes();
        return;
    }
    
    if(GetValue("outputsize_x"))
    {
        Module::SetSizes();
        return;
    }
    
    if(GetValue("outputsize_y"))
    {
        Module::SetSizes();
        return;
    }
    
    int sx, sy;
    float ** m = create_matrix(GetValue("data"), sx, sy); // get the sizes but ignore the data
    SetOutputSize("OUTPUT", sx, sy);
    destroy_matrix(m);
}



void
Constant::Init()
{
    output          =	GetOutputMatrix("OUTPUT");
    outputsize_x	=	GetOutputSizeX("OUTPUT");
    outputsize_y	=	GetOutputSizeY("OUTPUT");
    
    Bind(data, outputsize_x, outputsize_y, "data", true);

    copy_matrix(output, data, outputsize_x, outputsize_y);   // Copy to allow output to be inspected before first tick
}



void
Constant::Tick()
{
    copy_matrix(output, data, outputsize_x, outputsize_y);   // Copy every iteration if parameter changed through the binding
}

static InitClass init("Constant", &Constant::Create, "Source/Modules/UtilityModules/Constant/");

