//
//	EpiServos.h		This file is a part of the IKAROS project
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


#ifndef EpiServo_
#define EpiServo_

#include "IKAROS.h"

#include "dynamixel_sdk.h" // Uses Dynamixel SDK library

// Dynamixel settings
#define PROTOCOL_VERSION 2.0 // See which protocol version is used in the Dynamixel
#define BAUDRATE1M 1000000   // XL-320 is limited to 1Mbit
#define BAUDRATE3M 3000000   // MX servos

// Indirect adress
#define IND_ADDR_TORQUE_ENABLE 168
#define ADDR_TORQUE_ENABLE 64
#define IND_ADDR_GOAL_POSITION 170
#define ADDR_GOAL_POSITION 116
#define IND_ADDR_GOAL_CURRENT 178
#define ADDR_GOAL_CURRENT 102
#define IND_ADDR_PRESENT_POSITION 578
#define ADDR_PRESENT_POSITION 132
#define IND_ADDR_PRESENT_CURRENT 586
#define ADDR_PRESENT_CURRENT 128
#define IND_ADDR_PRESENT_TEMPERATURE 590
#define ADDR_PRESENT_TEMPERATURE 146

// Common for the 2.0 (not XL320)
#define ADDR_PROFILE_ACCELERATION 108
#define ADDR_PROFILE_VELOCITY 112
#define ADDR_P 84
#define ADDR_I 82
#define ADDR_D 80

// ID of each dynamixel chain.
#define HEAD_ID_MIN 2
#define HEAD_ID_MAX 5

#define ARM_ID_MIN 2
#define ARM_ID_MAX 7

#define BODY_ID_MIN 2
#define BODY_ID_MAX 2

#define PUPIL_ID_MIN 2
#define PUPIL_ID_MAX 3

#define EPI_TORSO_NR_SERVOS 6
#define EPI_NR_SERVOS 19

#define TIMER_POWER_ON 2000
#define TIMER_POWER_OFF 2000          // Timer ramping down
#define TIMER_POWER_OFF_EXTENDED 1000 // Timer until torque enable off

#define HEAD_INDEX_IO 0
#define PUPIL_INDEX_IO 4
#define LEFT_ARM_INDEX_IO 6
#define RIGHT_ARM_INDEX_IO 12
#define BODY_INDEX_IO 18

#define MAX_TEMPERATURE 55

typedef struct
{   
    std::string serialPortPupil; 
    std::string serialPortHead;
    std::string serialPortBody;
    std::string serialPortLeftArm;
    std::string serialPortRightArm;
    std::string type;

} Robot_parameters;

#include <map>
#include <iostream>


class EpiServos: public Module
{
public:
    static Module * Create(Parameter * p) { return new EpiServos(p); }

    EpiServos(Parameter * p) : Module(p) {}
    virtual ~EpiServos();

    void 		Init();
    void 		Tick();

    bool        SetDefaultSettingServo();
    bool        PowerOnRobot();
    bool        PowerOffRobot();

    // Paramteters
    int robotType = 0;
    bool simulate = false;

    // Ikaros IO
    float * goalPosition;
    float * goalCurrent;    
    float * torqueEnable;

    int goalPositionSize;
    int goalCurrentSize;
    int torqueEnableSize;

    float * presentPosition;
    int presentPositionSize;
    float * presentCurrent;
    int presentCurrentSize;
    
    bool EpiTorsoMode = false;
    bool EpiMode = false;

    dynamixel::PortHandler *portHandlerHead;
    dynamixel::PacketHandler *packetHandlerHead;
    dynamixel::GroupSyncRead *groupSyncReadHead;
    dynamixel::GroupSyncWrite * groupSyncWriteHead;

    dynamixel::PortHandler *portHandlerPupil;
    dynamixel::PacketHandler *packetHandlerPupil;
    dynamixel::GroupSyncRead *groupSyncReadPupil;
    dynamixel::GroupSyncWrite * groupSyncWritePupil;

    dynamixel::PortHandler *portHandlerLeftArm;
    dynamixel::PacketHandler *packetHandlerLeftArm;
    dynamixel::GroupSyncRead *groupSyncReadLeftArm;
    dynamixel::GroupSyncWrite * groupSyncWriteLeftArm;

    dynamixel::PortHandler *portHandlerRightArm;
    dynamixel::PacketHandler *packetHandlerRightArm;
    dynamixel::GroupSyncRead *groupSyncReadRightArm;
    dynamixel::GroupSyncWrite * groupSyncWriteRightArm;

    dynamixel::PortHandler *portHandlerBody;
    dynamixel::PacketHandler *packetHandlerBody;
    dynamixel::GroupSyncRead *groupSyncReadBody;
    dynamixel::GroupSyncWrite * groupSyncWriteBody;

    std::string robotName;
    std::map<std::string,Robot_parameters> robot;


    // Functions for each serial port (used to threading´)
    bool Communication(int IDMin, int IDMax, dynamixel::PortHandler *portHandler, dynamixel::PacketHandler *packetHandler, int IOIndex);

    bool CommunicationHead();
    bool CommunicationPupil();
    bool CommunicationLeftArm();
    bool CommunicationBody();
    bool CommunicationRightArm();
    
    bool PowerOn(int IDMin, int IDMax, dynamixel::PortHandler *portHandler, dynamixel::PacketHandler *packetHandler);

    bool PowerOnHead();
    bool PowerOnPupil();
    bool PowerOnLeftArm();
    bool PowerOnRightArm();
    bool PowerOnBody();

    bool PowerOff(int IDMin, int IDMax, dynamixel::PortHandler *portHandler, dynamixel::PacketHandler *packetHandler);

    bool PowerOffHead();
    bool PowerOffPupil();
    bool PowerOffLeftArm();
    bool PowerOffRightArm();
    bool PowerOffBody();

};

#endif

