//
//		Constant.cc		This file is a part of the IKAROS project
//						Implements a modules that outputs a constant array
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



#include "Constant.h"


void
Constant::Init()
{
    output		=	GetOutputArray("OUTPUT");
    outputsize	=	GetOutputSize("OUTPUT");        // Use one-dimensional representation internally
    
    Bind(data, outputsize, "data");    
}



void
Constant::Tick()
{
    copy_array(output, data, outputsize);   // Copy every iteration if parameter changed through the binding
}


