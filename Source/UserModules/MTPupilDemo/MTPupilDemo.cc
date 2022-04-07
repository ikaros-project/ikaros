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
#define EMOTION_NEG 52
#define EMOTION_NEG_VALUE -0.5
#define EMOTION_NEU 53
#define EMOTION_NEU_VALUE 0
#define EMOTION_POS 54
#define EMOTION_POS_VALUE 0.5
#define NOVELTY 90
#define NOVELTY_VALUE 0.5
#define SUN 51 // not implemented
#define SUN_VALUE 0.5


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

    io(eye_led_output_array, eye_led_output_array_size, "ROBOT_EYE_LED_OUTPUT");

    blinkTimer.Restart();

}

MTPupilDemo::~MTPupilDemo()
{
}

void MTPupilDemo::Tick()
{
    int id = *id_array;
    bool higherBlinkProb = false;

    reset_array(output_array,output_array_size);
    switch (id)
    {
    case MATH_EASY:
        *output_array = MATH_EASY_VALUE;
        higherBlinkProb = true;
        markerTimer.Restart();
        break;
    case MATH_HARD:
        *output_array = MATH_HARD_VALUE;
        higherBlinkProb = true;
        markerTimer.Restart();
        break;
    case EMOTION_NEG:
        *output_array = EMOTION_NEG_VALUE;
        higherBlinkProb = true;
        markerTimer.Restart();
        break;
    case EMOTION_NEU:
        *output_array = EMOTION_NEU_VALUE;
        higherBlinkProb = true;
        markerTimer.Restart();
        break;
    case EMOTION_POS:
        *output_array = EMOTION_POS_VALUE;
        higherBlinkProb = true;
        markerTimer.Restart();
        break;
    case NOVELTY:
        *output_array = NOVELTY_VALUE;
        higherBlinkProb = true;
        markerTimer.Restart();

        break;
    default:
        higherBlinkProb = false;
        break;
    }


    trackPos[0] = img_pos_array[0];
    trackPos[1] = img_pos_array[1];
    
            // Simple tracking? Track marker for a while.
    // Make this better.
    // if (higherBlinkProb) // in simple tracking mode
    // {
    //     float delta
    // }
    bool track = markerTimer.GetTime() < 2000;

    // if (track) // Look at maker for one second
    // {
    //     printf("Tracking!\n");
        robot_output_array[ROBOT_TILT_INDEX] = trackPos[1];
        robot_output_array[ROBOT_PAN_INDEX] =  (1-trackPos[0]);
        // robot_output_array[ROBOT_EYE_INDEX] =  1-trackPos[0];
        // robot_output_array[ROBOT_EYE_INDEX+1] = 1-trackPos[0];
        //robot_output_array[ROBOT_PAN_INDEX] =  0.5;
        robot_output_array[ROBOT_EYE_INDEX] =  0.5;
        robot_output_array[ROBOT_EYE_INDEX+1] = 0.5;
   
    // }
    // else
    // {
    //     printf("Not Tracking!\n");
    //     robot_output_array[ROBOT_TILT_INDEX] =  0.5;
    //     robot_output_array[ROBOT_PAN_INDEX] =  0.5;
    //     robot_output_array[ROBOT_EYE_INDEX] =  0.5;
    //     robot_output_array[ROBOT_EYE_INDEX+1] = 0.5;
            
    // }
    
    // LED
    eye_led_output_array[0] = 0.4;
    eye_led_output_array[1] = 0.3;
    eye_led_output_array[2] = 0.2;

    if (markerTimer.GetTime() < 2000) // Look at maker for one second
        eye_led_output_array[3] = 1;
    else
        eye_led_output_array[3] = 0.8; // Default intensity
    

    // Blinking
    // Blink in interval of 4-10 sec
    bool blink = false;
    if (blinkTimer.GetTime() > TimeToNextBlink)
        blink = true;
    if (blinkTimer.GetTime() > TimeToNextBlink + 200)
    {
        blink = false;
        blinkTimer.Restart();
        TimeToNextBlink = random(4000.0,10000.0);
    }
    if (blink)
        eye_led_output_array[3] = 0;


    // if (higherBlinkProb)
    //     eye_led_output_array[3] = 0;
    // else
    //     eye_led_output_array[3] = 1;

    
}

// Install the module. This code is executed during start-up.

static InitClass init("MTPupilDemo", &MTPupilDemo::Create, "Source/UserModules/MTPupilDemo/");
