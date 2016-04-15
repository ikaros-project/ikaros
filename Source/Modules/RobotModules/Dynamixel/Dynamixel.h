//
//	Dynamixel.h		This file is a part of the IKAROS project
//
//    Copyright (C) 2010-2012  Christian Balkenius and Birger Johansson
//
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

#ifndef Dynamixel_
#define Dynamixel_

#include "IKAROS.h"
#include "DynamixelComm.h"
#include "DynamixelServo.h"

#define IK_IN_TORQUE_ENABLE        0
#define IK_IN_LED                  1
#define IK_IN_D_GAIN               2
#define IK_IN_I_GAIN               3
#define IK_IN_P_GAIN               4
#define IK_IN_GOAL_POSITION        5
#define IK_IN_MOVING_SPEED         6
#define IK_IN_TORQUE_LIMIT         7
#define IK_IN_GOAL_TORQUE          8
#define IK_IN_GOAL_ACCELERATION    9

#define IK_OUT_TORQUE_ENABLE         0
#define IK_OUT_LED                   1
#define IK_OUT_D_GAIN                2
#define IK_OUT_I_GAIN                3
#define IK_OUT_P_GAIN                4
#define IK_OUT_GOAL_POSITION         5
#define IK_OUT_MOVING_SPEED          6
#define IK_OUT_TORQUE_LIMIT          7
#define IK_OUT_PRESENT_POSITION      8
#define IK_OUT_PRESENT_SPEED         9
#define IK_OUT_PRESENT_LOAD          10
#define IK_OUT_PRESENT_VOLTAGE       11
#define IK_OUT_PRESENT_TEMPERATURE   12
#define IK_OUT_PRESENT_CURRENT       13
#define IK_OUT_GOAL_TORQUE           14
#define IK_OUT_GOAL_ACCELERATION     15


class Dynamixel: public Module
{
public:
    static Module * Create(Parameter * p) { return new Dynamixel(p); }
    
	Dynamixel(Parameter * p);
	virtual ~Dynamixel();
    
    void        SetSizes();
    
	void		Init();
	void		Tick();
    
private:
    int         size;
    int         servos;
    
    int         init_print;
    int         index_mode;
    int         angle_unit;
    
    bool        use_feedback;
    int         start_up_delay;
    
    int         ikarosInBind[10];    // Where to look for ikaros data in the dynamixel memory block
    int         ikarosOutBind[16];   // Where to look for ikaros data in the dynamixel memory block

    
    // Inputs and outputs
    // Inputs
    float *     torqueEnable;
    bool        torqueEnable_connected;
    float *     LED;
    bool        LED_connected;
    float *     dGain;
    bool        dGain_connected;
    float *     iGain;
    bool        iGain_connected;
    float *     pGain;
    bool        pGain_connected;
    float *     goalPosition;
    bool        goalPosition_connected;
    float *     movingSpeed;
    bool        movingSpeed_connected;
    float *     torqueLimit;
    bool        torqueLimit_connected;
    float *     goalTorque;
    bool        goalTorque_connected;
    float *     goalAcceleration;
    bool        goalAcceleration_connected;
    
    
    // Outputs
    float * feedbackTorqueEnable;
    float * feedbackLED;
    float * feedbackDGain;
    float * feedbackIGain;
    float * feedbackPGain;
    float * feedbackGoalPosition;
    float * feedbackMoving;
    float * feedbackTorqueLimit;
    float * feedbackPresentPosition;
    float * feedbackPresentSpeed;
    float * feedbackPresentLoad;
    float * feedbackPresentVoltage;
    float * feedbackPresentTemperature;
    float * feedbackPresentCurrent;
    float * feedbackGoalTorque;
    float * feedbackGoalAcceleration;
    
    // Array of servo data
    DynamixelServo **    servo;
    
    // Arrays used to send commands to servos
    int *       servo_index;
    int *       servo_id;
    
    unsigned char ** DynamixelMemoeries;
    
    const char *    device;
    DynamixelComm * com;
    
    // Print
    void		Print();
    void        PrintAll();
};

#endif

