//
//	DepthBlobList.cc	This file is a part of the IKAROS project
//                      Create list of bobs in 3D
//
//    Copyright (C) 2014  Christian Balkenius
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

#include "DepthBlobList.h"


using namespace ikaros;


void
DepthBlobList::Init()
{
    size_x	 = GetInputSizeX("INPUT");
    size_y	 = GetInputSizeY("INPUT");

    input   = GetInputMatrix("INPUT");
    output  = GetOutputMatrix("OUTPUT");
}



void
DepthBlobList::Tick()
{
    h_reset(*output);

    float sum_d = 0;
    float sum_x = 0;
    float sum_y = 0;
    float n = 0;

    for(int i=0; i<size_x; i++)
        for(int j=0; j<size_y; j++)
        {
            if(input[j][i] > 0)
            {
                n += 1;
                sum_d += input[j][i];
                sum_x += float(i);
                sum_y += float(j);
            }
        }

    if(n == 0)
        return;

    sum_d /= n;
    sum_x /= n;
    sum_y /= n;

    h_eye(*output);

    (*output)[3] = sum_d;
    (*output)[7] = 320-sum_x;   // origin in the middle
    (*output)[11] = 240-sum_y;  // TODO: calculate real positions
}


static InitClass init("DepthBlobList", &DepthBlobList::Create, "Source/Modules/VisionModules/DepthProcessing/DepthBlobList/");

