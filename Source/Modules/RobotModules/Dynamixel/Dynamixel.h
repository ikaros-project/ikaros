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



//class Servo;

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
    int         baud_rate;
    int         start_up_delay;
    
    
    // Inputs and outputs
    // Inputs
    float *     torqueEnable;
    bool        allocated_torqueEnable;
    float *     LED;
    bool        allocated_LED;
    float *     dGain;
    bool        allocated_dGain;
    float *     iGain;
    bool        allocated_iGain;
    float *     pGain;
    bool        allocated_pGain;
    float *     goalPosition;
    bool        allocated_goalPosition;
    float *     movingSpeed;
    bool        allocated_movingSpeed;
    float *     torqueLimit;
    bool        allocated_torqueLimit;
    
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
    
    void parseControlTable(Dictionary * dic, unsigned char* buf, int from, int to);
    int checkBaudRate(int br);
};

#endif

