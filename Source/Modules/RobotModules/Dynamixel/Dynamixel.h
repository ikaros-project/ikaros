//
//	Dynamixel.h		This file is a part of the IKAROS project
//
//    Copyright (C) 2010-2011  Christian Balkenius
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

class Servo;

class Dynamixel: public Module 
{
public:
    static Module * Create(Parameter * p) { return new Dynamixel(p); }

	Dynamixel(Parameter * p);
	virtual ~Dynamixel();
    
    void        SetSizes();
    
	void		Print();

	void		Init();
	void		Tick();

private:
    int         size;
    int         servos;
    
    int         index_mode;
    int         angle_unit;
    float       position_speed; // global speed in fraction of max

    float       timebase;    
    bool        use_feedback;
    
    int         start_up_delay;
    bool        list_servos;
    
    // Continusou rotation parameters
    
    float       gain_alpha;
    float       gain_initial;
    float       deadband_initial;

    // Inputs and outputs

    float *     input;
    float *     speed;
    float *     torque;

    float *     output;
    float *     load;
    float *     voltage;
    float *     temperature;

    // Array of servo data
    
    Servo **    servo;
    
    // Arrays used to send commands to servos // FIXME: USE separat list for continous servos

    int *       servo_index;
    int *       servo_id;
    int *       positions;
    int *       speeds;
    int *       torques;


    const char *    device;
    DynamixelComm * com;
};

#endif

