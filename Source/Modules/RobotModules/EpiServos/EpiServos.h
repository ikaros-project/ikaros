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
    bool CommunicationHead();
    bool CommunicationPupil();
    bool CommunicationLeftArm();
    bool CommunicationBody();
    bool CommunicationRightArm();
    
    bool PowerOnHead();
    bool PowerOnPupil();
    bool PowerOnLeftArm();
    bool PowerOnRightArm();
    bool PowerOnBody();

    bool PowerOffHead();
    bool PowerOffPupil();
    bool PowerOffLeftArm();
    bool PowerOffRightArm();
    bool PowerOffBody();

};

#endif

