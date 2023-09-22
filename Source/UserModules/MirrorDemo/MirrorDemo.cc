//
//	MirrorDemo.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2012 <Author Name>
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


#include "MirrorDemo.h"

using namespace ikaros;

void
MirrorDemo::Init()
{
    io(DepthImage, DepthImageSizeX, DepthImageSizeXY, "INPUT");
    io(output, outputSize, "OUTPUT");
}



void
MirrorDemo::Tick()
{
    int x = 0;
    int y = 0;

    // Filter out point under 400mm
    for (size_t i = 0; i < DepthImageSizeX; i++)
     for (size_t j = 0; j < DepthImageSizeXY; j++)
    {
    if (DepthImage[j][i] < 400)
        DepthImage[j][i] = 1000000000;
    }
    

    arg_min(x,y,DepthImage,DepthImageSizeX,DepthImageSizeXY);
    output[0] = (float)x/(float)DepthImageSizeX;
    output[1] = (float)y/(float)DepthImageSizeXY;
    output[2] = DepthImage[y][x];

}



static InitClass init("MirrorDemo", &MirrorDemo::Create, "Source/UserModules/MirrorDemo/");


