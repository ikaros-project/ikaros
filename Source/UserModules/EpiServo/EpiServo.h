//
//	EpiServo.h		This file is a part of the IKAROS project
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
    std::string serialPortLeftArm;
    std::string serialPortRightArm;
    std::string type;

} Robot_parameters;

#include <map>
#include <iostream>


class EpiServo: public Module
{
public:
    static Module * Create(Parameter * p) { return new EpiServo(p); }

    EpiServo(Parameter * p) : Module(p) {}
    virtual ~EpiServo();

    void 		Init();
    void 		Tick();

    bool        SetDefaultSettingServo();
    bool        TorqueingUpServo();
    bool        TorqueingDownServo();

    // Paramteters
    int robotType = 0;
    bool simulate = false;

    // Ikaros IO
    float * goalPosition;
    float * goalCurrent;    
    int goalPositionSize;
    int goalCurrentSize;
    
    float * presentPosition;
    int presentPositionSize;
    float * presentCurrent;
    int presentCurrentSize;
    
    bool EpiTorsoMode = false;
    bool EpiMode = false;

    dynamixel::PortHandler *portHandlerHead;
    dynamixel::PacketHandler *packetHandlerHead;
    int dxl_comm_resultHead;
    std::vector<uint8_t> vecHead; // Dynamixel data storages
    dynamixel::GroupSyncRead *groupSyncReadHead;
    dynamixel::GroupSyncWrite * groupSyncWriteHead;
    dynamixel::GroupSyncWrite * groupSyncWritePupil;

    dynamixel::PortHandler *portHandlerPupil;
    dynamixel::PacketHandler *packetHandlerPupil;
    int dxl_comm_resultPupil;
    std::vector<uint8_t> vecPupil; // Dynamixel data storages
    dynamixel::GroupSyncRead *groupSyncReadPupil;

    std::string robotName;
    std::map<std::string,Robot_parameters> robot;
};

#endif

