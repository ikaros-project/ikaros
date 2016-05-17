//
//	EdgeSegmentation.cc	This file is a part of the IKAROS project
//
//
//    Copyright (C) 2016  Christian Balkenius
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

#include "EdgeSegmentation.h"

using namespace ikaros;



void
EdgeSegmentation::Init()
{
    inputsize_x	 	= GetInputSizeX("INPUT");
    inputsize_y	 	= GetInputSizeY("INPUT");

    outputsize_x	= GetOutputSizeX("OUTPUT");
    outputsize_y	= GetOutputSizeY("OUTPUT");

    input			= GetInputMatrix("INPUT");
    output			= GetOutputMatrix("OUTPUT");

    edge_list       = GetOutputMatrix("EDGE_LIST");
    edge_list_size  = GetOutputArray("EDGE_LIST");
}



void
EdgeSegmentation::Tick()
{
 

    
}



static InitClass init("EdgeSegmentation", &EdgeSegmentation::Create, "Source/Modules/VisionModules/ImageOperators/EdgeDetectors/EdgeSegmentation/");


