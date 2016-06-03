//
//	EyeModel.cc		This file is a part of the IKAROS project
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


#include "EyeModel.h"

using namespace ikaros;

void
EyeModel::Init()
{
    Bind(speed, "speed");

    gaze = GetInputArray("GAZE");
    pupil = GetInputArray("PUPIL");
    
    left_eye = GetOutputArray("LEFT_EYE");
    right_eye = GetOutputArray("RIGHT_EYE");
}



void
EyeModel::Tick()
{
    left_eye[0] = gaze[0];
    left_eye[1] = gaze[1];
    right_eye[0] = gaze[0];
    right_eye[1] = gaze[1];

    left_eye[2]  += speed * (pupil[0] - left_eye[2]);
    right_eye[2] += speed * (pupil[0] - right_eye[2]);
}



static InitClass init("EyeModel", &EyeModel::Create, "Source/UserModules/EyeModel/");


