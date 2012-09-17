//
//	Dynamixel.cc	This file is a part of the IKAROS project
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
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
//

#include "Dynamixel.h"
#include "DynamixelComm.h"

using namespace ikaros;


class Servo
{
public:
    int ID;
    int servo_model;
    const char * servo_model_string;
    int position_max;
    int speed_max;
    int torque_max;
    float angle_max;
    float nominal_speed; // RPM
    bool continuous_mode;

    Servo(DynamixelComm *com, int id)
    {
        unsigned char inbuf[256];
        
        ID = id;
        
        // Get data from servo
        
        com->ReadMemoryBlock(ID, inbuf, 0, 50);

        // Get servo type and data from servo
        
        servo_model = inbuf[0]+256*inbuf[1];
        continuous_mode = (inbuf[6] == 0 && inbuf[7] == 0 && inbuf[8] == 0 && inbuf[9] == 0);
//        float voltage = float(inbuf[0x2A])/10.0; // will be used to calculate nominal speed
        
        switch (servo_model)
        {
            case 12:
                servo_model_string = "AX-12";
                position_max = 0x3ff;
                speed_max = 0x3ff;
                torque_max = 0x3ff;
                angle_max = 300; // degrees
                nominal_speed = 51; // RPM @ 10V  // TODO: calculate based on voltage (0.269/60 deg @ 7V, 0.196/60 deg @ 10V); or 114 @ 0x3ff ???
                break;

            case 24:
                servo_model_string = "RX-24F";
                position_max = 0x3ff;
                speed_max = 0x3ff; // .111rpm per step
                torque_max = 0x3ff;
                angle_max = 300; // degrees
                nominal_speed = 51; // RPM @ 10V  // TODO: calculate based on voltage (0.269/60 deg @ 7V, 0.196/60 deg @ 10V) or 114 @ 0x3ff ???
                break;

            case 28:
                servo_model_string = "RX-28";
                position_max = 0x3ff;
                speed_max = 0x3ff;
                torque_max = 0x3ff;
                angle_max = 300; // degrees
                nominal_speed = 59.9; // RPM @ 12V  // TODO: calculate based on voltage (0.269/60 deg @ 7V, 0.196/60 deg @ 10V) or 114 @ 0x3ff ???
                break;

            case 29:
                servo_model_string = "MX-28";
                position_max = 0xfff; // 0.088 deg per step
                speed_max = 0x3ff;
                torque_max = 0x3ff;
                angle_max = 300; // degrees
                nominal_speed = 54; // RPM @ ??V  // TODO: calculate based on voltage (0.269/60 deg @ 7V, 0.196/60 deg @ 10V) or 114 @ 0x3ff ???
                break;

            case 64:
                servo_model_string = "EX-64";
                position_max = 0x3ff;
                speed_max = 0x3ff;
                torque_max = 0x3ff;
                angle_max = 300; // degrees
                nominal_speed = 53.2; // RPM @ 15V  // TODO: calculate based on voltage (69.9 @ 18.5V)
                break;

            case 107:
                servo_model_string = "EX-106";
                position_max = 0xfff;
                speed_max = 0x3ff;
                torque_max = 0x3ff;
                angle_max = 280.6; // degrees
                nominal_speed = 54.9; // RPM @ 14.8V  // TODO: calculate based on voltage (63.69 @ 18V)
                break;

            default:
                servo_model_string = "Unknown";
                position_max = 0x3ff;
                torque_max = 0x3ff;
                angle_max = 300; // degrees
                nominal_speed = 114; // RPM
                break;
            }
    }
    
    ~Servo()
    {
    }
    
    float ConvertToAngle(int position, int angle_unit);
    int ConvertToPosition(float angle, int angle_unit);
};



float
Servo::ConvertToAngle(int position, int angle_unit)
{
    float angle = angle_max * float(position)/float(position_max);
    switch(angle_unit)
    {
        case 0: 
        default: return angle;
        case 1:  return 2.0*pi*angle/360.0;
        case 2:  return angle/360.0;            
        case 3:  return angle/angle_max;            
    }
    
    return 0;
}



int
Servo::ConvertToPosition(float angle, int angle_unit)
{
    switch(angle_unit)
    {
        case 0: 
        default: break; // angle = angle; 
        case 1:  angle = 360*angle/(2*pi); break;
        case 2:  angle = 360*angle;            
        case 3:  angle = angle_max*angle;            
    }

    angle = clip(angle, 0, angle_max)/angle_max;
    return int(float(position_max)*angle);
}



Dynamixel::Dynamixel(Parameter * p):
    Module(p)
{
    Bind(position_speed, "position_speed");

    device = GetValue("device");
    
    if(!device)
    {
        Notify(msg_warning, "Dynamixel serial device name is not set.");
        size = 0;
        return;
    }
    
    com = new DynamixelComm(device, 1000000); // FIXME: should be a parameter
    if(!com)
    {
        Notify(msg_warning, "Dynamixel serial device \"%s\" could not be opened.", device);
        return;
    }
    
    use_feedback = GetBoolValue("feedback");
    
    start_up_delay = GetIntValue("start_up_delay");
    
    index_mode = GetIntValueFromList("index_mode");
    angle_unit = GetIntValueFromList("angle_unit");
    int max_servo_id = GetIntValue("max_servo_id");
    int servo_id_list_size = 0;
    int * servo_id_list = GetIntArray("servo_id", servo_id_list_size);

    servo = new Servo * [256]; // maximum number of servos
    for(int i=0; i<256; i++)
        servo[i] = NULL;

    // Alt 1: create a list of servos with the IDs read from servo_id
    
    if(servo_id_list_size > 0 && index_mode == 0)
    {
        for(int i=0; i<servo_id_list_size; i++)
        {
            if(com->Ping(servo_id_list[i]))
            {
                servo[servo_id_list[i]] = new Servo(com, servo_id_list[i]);
            }
            else
            {
                servo[servo_id_list[i]] = NULL;
                Notify(msg_warning, "Dynamixel servo with ID = %d could not be found\n", servo_id_list[i]);
            }
            if(servo_id_list[i] > size)
                size = servo_id_list[i]+1;
        }
    }
    
    else if(servo_id_list_size > 0 && index_mode == 1)
    {
        for(int i=0; i<servo_id_list_size; i++)
        {
            if(com->Ping(servo_id_list[i]))
            {
                servo[i] = new Servo(com, servo_id_list[i]);
            }
            else
            {
                servo[i] = NULL;
                Notify(msg_warning, "Dynamixel servo with ID = %d could not be found\n", servo_id_list[i]);
            }
        }
        size = servo_id_list_size;
    }
    
    // Alt 2: scan for servos
    
    else if(index_mode == 0)
    {
        size = 0;
        for(int i=1; i<max_servo_id; i++)
        {
            if(com->Ping(i))
            {
                servo[i] = new Servo(com, i);
                size = i+1;
            }
            else
                servo[i] = NULL;
        }
    }
    else
    {
        size = 0;
        for(int i=1; i<max_servo_id; i++)
        {
            if(com->Ping(i))
            {
                servo[size] = new Servo(com, i);
                size += 1;
            }
        }
    }
    
    // Adjust size if set as a parameter

    int psize = GetIntValue("size");
    if(psize > 256)
        psize = 256;
    if(psize != 0)
        size = psize;
        
    // Count servos
    
    servos = 0;
    for(int i=0; i<size; i++)
        if(servo[i])
            servos++;
            
    // Allocate servo maps and parameter structures
    
    servo_index  = new int [servos];
    servo_id     = new int [servos];
    positions    = new int [servos];
    speeds       = new int [servos];
    torques      = new int [servos];

    int j=0;
    for(int i=0; i<size; i++)
        if(servo[i])
        {
            servo_index[i] = j++;
            servo_id[i] = servo[i]->ID;
        }
}



void
Dynamixel::Print()
{
    printf("Size: %d\n", size);
    printf("Servos: %d\n\n", servos);
    printf("ix  ID  Model     Mode  MaxPos  MaxSpeed  MaxAngle  NomSpeed \n");
    printf("------------------------------------------------------------\n");
    for(int i=0; i<size; i++)
        if(servo[i])
        {
            printf("%-2d  ", i);
            printf("%3d ", servo[i]->ID);
            printf("%-10s", servo[i]->servo_model_string);
            printf("%-6s", (servo[i]->continuous_mode ? "wheel" : "servo" ));

            printf("  %4d    ", servo[i]->position_max);
            printf("  %4d   ", servo[i]->speed_max);
            printf(" %6.1f   ", servo[i]->angle_max);
            printf("  %5.2f ", servo[i]->nominal_speed);
            printf("\n");
        }
    printf("\n");
}

/*
int position_max;
int speed_max;
int torque_max;
float angle_max;
float nominal_speed; // RPM
bool continuous_mode;
*/

void
Dynamixel::SetSizes()
{
    SetOutputSize("OUTPUT", size);
    SetOutputSize("LOAD", size);
    SetOutputSize("VOLTAGE", size);
    SetOutputSize("TEMPERATURE", size);
}



void
Dynamixel::Init()
{
	input		= GetInputArray("INPUT");
	speed		= GetInputArray("SPEED", false);
	torque		= GetInputArray("TORQUE", false);

	output		= GetOutputArray("OUTPUT");

    load        = GetOutputArray("LOAD");
    voltage     = GetOutputArray("VOLTAGE");
    temperature = GetOutputArray("TEMPERATURE");

    // Get the initial position values
    
    for(int i=0; i<size; i++)
        if(servo[i])
            output[i] = servo[i]->ConvertToAngle(com->GetPosition(servo[i]->ID), angle_unit);
        else
            output[i] = 0;
    
    timebase    = (float)GetTickLength();

    Print();
}



Dynamixel::~Dynamixel()
{
    // Turn off torque for all servos
    
    com->SetTorque(254, 0); // TODO: Rampa ner torque!!!
    
    delete servo_id;
    delete positions;
    delete speeds;
    delete torques;    
}



void
Dynamixel::Tick()
{
    // check that the device exists and is open

    if(!device)
        return;

    if(!com)
        return;

    // do not move if the input contains only zeros

    if(norm(input, size) == 0)
        return;

    // send position commands (for servos in position mode)
    
    for(int i=0; i<size; i++)
        if(servo[i])
        {
            if(servo[i]->continuous_mode && speed)
            {
                positions[servo_index[i]] = 0;  // Do different servis have different direction bit?
                speeds[servo_index[i]] = int((servo[i]->speed_max-1)*clip(abs(input[i]), 0, 1)) | (input[i] < 0  ? 0x400 : 0x000);
                torques[i] = 0;
            }
            else // servo mode
            {
                positions[servo_index[i]] = servo[i]->ConvertToPosition(input[i], angle_unit);
                speeds[servo_index[i]] = int(1+position_speed*float(servo[i]->speed_max-1));      // avoid speed of 0 which would turn off speed control
                if(torque)
                    torques[i] = servo[i]->torque_max*clip(torque[i], 0, 1);
                else
                    torques[i] = 0;
            }
        }
    
    if(timebase != 0) // Calculate speeds based on distance to target if we are running in real-time mode
        for(int i=0; i<size; i++)
            if(servo[i])
            {   
                float speed_p = 60*300*1000*servo[i]->speed_max / (360*timebase*servo[i]->nominal_speed);
                float speed_extra = 0.1;
                int s = int(speed_p*abs(output[i]-input[i])) + speed_extra;
                if(s > servo[i]->speed_max-1)
                    s = servo[i]->speed_max-1;
                speeds[servo_index[i]] = 1+s;
            }
    
    if(GetTick() >= start_up_delay) // Do not send commands the first few ticks
    {
        if(torque)
            com->SyncMoveWithIdSpeedAndTorque(servo_id, positions, speeds, torques, servos);
        else
            com->SyncMoveWithIdAndSpeed(servo_id, positions, speeds, servos);
    }
    
    if(use_feedback)
    {
        unsigned char inbuf[256]; // temp buffer
        for(int i=0; i<size; i++)
            if(servo[i])
            {
                com->ReadAllData(servo[i]->ID, inbuf); // The whole block is fetched but only a few of the values are used in ikaros.

                output[i] = servo[i]->ConvertToAngle(inbuf[36]+256*inbuf[37], angle_unit); //Extract the Position
                load[i] = float((inbuf[41] & 1024) == 0 ? -1: 1)*float((inbuf[40]+256*inbuf[41]) & 1023)/1023.0; //Extract the Load
                voltage[i] = 0.1*float(inbuf[42]); //Extract the Voltage
                temperature[i] = float(inbuf[43]);  //Extract the Temperature in degrees Celcius    
            }
    }
    
    else
    {
        copy_array(output, input, size);
    }
}


