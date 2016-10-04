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
    int         size;           // Size of the servo[]. Size != number of servoes as in direct mode it can be gap between servos.
    int         servos;         // Number of servoes found
    
    int         init_print;
    int         index_mode;
    int         angle_unit;
    
    bool        use_feedback;
    int         start_up_delay;
    
    // index of where to find ikaros data in the dynamixel memory block
    int **      ikarosInBind;           // Array of where to store input data in the dynamixel memory block
    int **      ikarosOutBind;          // Array of where to grab output data in the dynamixel memory block
    int **      parameterInSize;        // Array of how many bytes the input parameter. This one is needed to calculate packate size in bulk_write as servoes may have different paramter size.
    int         protocol;               // The protocol used. No mixed protocol allowed.
    int *       mask;

    // Inputs
    float *     torqueEnable;
    bool        torqueEnableConnected;
    float *     LED;
    bool        LEDConnected;
    float *     dGain;
    bool        dGainConnected;
    float *     iGain;
    bool        iGainConnected;
    float *     pGain;
    bool        pGainConnected;
    float *     goalPosition;
    bool        goalPositionConnected;
    float *     movingSpeed;
    bool        movingSpeedConnected;
    float *     torqueLimit;
    bool        torqueLimitConnected;
    float *     goalTorque;
    bool        goalTorqueConnected;
    float *     goalAcceleration;
    bool        goalAccelerationConnected;
    
    // Outputs
    float *     feedbackTorqueEnable;
    float *     feedbackLED;
    float *     feedbackDGain;
    float *     feedbackIGain;
    float *     feedbackPGain;
    float *     feedbackGoalPosition;
    float *     feedbackMoving;
    float *     feedbackTorqueLimit;
    float *     feedbackPresentPosition;
    float *     feedbackPresentSpeed;
    float *     feedbackPresentLoad;
    float *     feedbackPresentVoltage;
    float *     feedbackPresentTemperature;
    float *     feedbackPresentCurrent;
    float *     feedbackGoalTorque;
    float *     feedbackGoalAcceleration;
    
    DynamixelServo **   servo; // Array of servo data
    
    // Arrays used to send commands to servos
    int             *   servoIndex;        // Array of indexes of where to find servoes in the servo[] array.
    int             *   servoId;           // Array of ids example [1,2,3] or [1,5,19]
    
    unsigned char   **  DynamixelMemoeries;
    
    const char      *   device;
    DynamixelComm   *   com;
    
    void		Print();
    void        PrintAll();
};

#endif

