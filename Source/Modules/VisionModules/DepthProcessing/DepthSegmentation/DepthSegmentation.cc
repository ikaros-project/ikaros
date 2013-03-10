//
//	DepthSegmentation.cc	This file is a part of the IKAROS project
//                          Segment image into depth planes
//
//    Copyright (C) 2012  Christian Balkenius
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

#include "DepthSegmentation.h"


using namespace ikaros;


void
DepthSegmentation::Init()
{
    Bind(mask_left, "mask_left");
    Bind(mask_right, "mask_right");
    
    size_x	 = GetInputSizeX("INPUT");
    size_y	 = GetInputSizeY("INPUT");

    object      = GetInputArray("OBJECT");
    input       = GetInputMatrix("INPUT");
    output		= GetOutputMatrix("OUTPUT");
}



void
DepthSegmentation::Tick()
{
    reset_matrix(output, size_x, size_y);

    int a = int(mask_left*size_x);
    int b = int(mask_right*size_x);
    
    for(int i=a; i<b; i++)
        for(int j=0; j<size_y; j++)
        {
            if(object[0] <= input[j][i] && input[j][i] <= object[1])
                output[j][i] = 1;
        }
}


static InitClass init("DepthSegmentation", &DepthSegmentation::Create, "Source/Modules/VisionModules/DepthProcessing/DepthSegmentation/");

