//
//	MTPupilDemo.cc		This file is a part of the IKAROS project
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
//  This example is intended as a starting point for writing new Ikaros modules
//  The example includes most of the calls that you may want to use in a module.
//  If you prefer to start with a clean example, use he module MinimalModule instead.
//

#include "MTPupilDemo.h"


#define MATH_EASY 85
#define MATH_EASY_VALUE 0.2 
#define MATH_HARD 86
#define MATH_HARD_VALUE 0.5
#define EMOTION_NEG 53
#define EMOTION_NEG_VALUE 0.5
#define EMOTION_NEU 88
#define EMOTION_NEU_VALUE 0
#define EMOTION_POS 89
#define EMOTION_POS_VALUE 0.5
#define NOVELTY 90
#define NOVELTY_VALUE 0.5

#define ROBOT_TILT_INDEX 0
#define ROBOT_PAN_INDEX 1
#define ROBOT_EYE_INDEX 2 // Only use one eye?

using namespace ikaros;

void MTPupilDemo::Init()
{
    io(id_array, id_array_size, "ID");
    io(img_pos_array, img_pos_array_size, "IMAGE_POS");
    io(output_array, output_array_size, "OUTPUT");
    io(robot_output_array, robot_output_array_size, "ROBOT_OUTPUT");
}

MTPupilDemo::~MTPupilDemo()
{
}

void MTPupilDemo::Tick()
{
    print_array("id_array", id_array, id_array_size);
    print_array("img_pos_array", img_pos_array, img_pos_array_size);
    int id = *id_array;
    reset_array(output_array,output_array_size);
    printf("ID %i\n",id);
    switch (id)
    {
    case MATH_EASY:
        *output_array = MATH_EASY_VALUE;
        break;
    case MATH_HARD:
        *output_array = MATH_HARD_VALUE;
        break;
    case EMOTION_NEG:
        *output_array = EMOTION_NEG_VALUE;
        break;
    case EMOTION_NEU:
        *output_array = EMOTION_NEU_VALUE;
        break;
    case EMOTION_POS:
        *output_array = EMOTION_POS_VALUE;
        break;
    case NOVELTY:
        *output_array = NOVELTY_VALUE;
        break;
    default:
        break;
    }

    if (id == 0)
    {
       img_pos_array[0] = 0.5; 
       img_pos_array[1] = 0.5; 
    }
    // Simple tracking? Track marker for a while.
    robot_output_array[ROBOT_TILT_INDEX] = img_pos_array[1];
    robot_output_array[ROBOT_PAN_INDEX] =  img_pos_array[0];
    robot_output_array[ROBOT_EYE_INDEX] =  img_pos_array[0];

    print_array("output", output_array, output_array_size);
}

// Install the module. This code is executed during start-up.

static InitClass init("MTPupilDemo", &MTPupilDemo::Create, "Source/UserModules/MTPupilDemo/");
