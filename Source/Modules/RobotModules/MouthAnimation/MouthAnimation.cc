//
//	  MouthAnimation.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2021 Christian Balkenius
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


#include "MouthAnimation.h"


using namespace ikaros;


void
MouthAnimation::Init()
{
    io(trig, "TRIG");
    io(volume, "VOLUME");
    io(mouth_top_red, "MOUTH_TOP_RED");
    io(mouth_top_green, "MOUTH_TOP_GREEN");
    io(mouth_top_blue, "MOUTH_TOP_BLUE";
    io(mouth_bottom_red, "MOUTH_BOTTOM_RED");
    io(mouth_bottom_green, "MOUTH_BOTTOM_GREEN");
    io(mouth_bottom_blue, "MOUTH_BOTTOM_BLUE");
}


void
MouthAnimation::Tick()
{

}

static InitClass init("MouthAnimation", &MouthAnimation::Create, "Source/Modules/RobotModules/MouthAnimation/");


