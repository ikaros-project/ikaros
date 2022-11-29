//
//	EyeLidModel.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2016 Christian Balkenius & Birger Johansson
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


#include "EyeLidModel.h"

using namespace ikaros;



void
EyeLidModel::Init()
{
    Bind(pupil_min, "pupil_min");
    Bind(pupil_max, "pupil_max");

    Bind(epsilon, "epsilon");
    Bind(m3, "m3");
    Bind(alpha1a, "alpha1a");

    Bind(amplifier, "amplifier");
    
    gaze = GetInputArray("GAZE");

    pupil_sphincter = GetInputArray("PUPIL_SPHINCTER");
    pupil_dilator = GetInputArray("PUPIL_DILATOR");
    
    pupil_diameter = GetOutputArray("PUPIL_DIAMETER");
    
    output = GetOutputArray("OUTPUT");
}



void
EyeLidModel::Tick()
{
    if(gaze)
    {
        output[0] = gaze[0];
        output[1] = gaze[1];
    }
    float p = pupil_min + (pupil_max-pupil_min) * clip(amplifier*(alpha1a*pupil_dilator[0]-m3*pupil_sphincter[0]), 0, 1);
    pupil_diameter[0]  += epsilon * (p - pupil_diameter[0]);
    output[2] = pupil_diameter[0]; 
}



static InitClass init("EyeLidModel", &EyeLidModel::Create, "Source/Modules/BrainModels/EyeLidModel/");


