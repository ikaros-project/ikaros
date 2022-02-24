//
//	EpiServo.cc		This file is a part of the IKAROS project
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

#include "EpiServo.h"

#include <stdio.h>
#include <vector>

#include "dynamixel_sdk.h" // Uses Dynamixel SDK library

using namespace ikaros;

// Dynamixel settings
#define PROTOCOL_VERSION 2.0 // See which protocol version is used in the Dynamixel
#define BAUDRATE1M 1000000
#define BAUDRATE3M 3000000

// Control table address (ikaros input uses indirect adresses)
#define ADDR_GOAL_POSITION 168
#define ADDR_GOAL_CURRENT 224

#define LEN_GOAL_POSITION 168
#define LEN_GOAL_CURRENT 224

#define ADDR_PRO_LED_RED 65
#define ADDR_PRO_PRESENT_POSITION 224

// Data Byte Length
#define LEN_PRO_LED_RED 1
#define LEN_PRO_PRESENT_POSITION 4

// Protocol version
#define PROTOCOL_VERSION 2.0 // See which protocol version is used in the Dynamixel

#define EPI_TORSO 0
// #define EPI_FULL 1

#define EPI_TORSO_NR_SERVOS 6
#define EPI_NR_SERVOS 10

void EpiServo::Init()
{
      // Robots configurations
    robot["EpiWhite"] = {.serialPortPupil = "/dev/cu.usbserial-FT4TCJXI",
                         .serialPortHead = "/dev/cu.usbserial-FT4TCGUT",
                         .serialPortLeftArm = "",
                         .serialPortRightArm = "",
                         .type = "EpiTorso"};

    robot["EpiBlue"] = {.serialPortPupil = "/dev/cu.usbserial-FT4TCJXI",
                        .serialPortHead = "/dev/cu.usbserial-FT4TCGUT",
                        .serialPortLeftArm = "",
                        .serialPortRightArm = "",
                        .type = "EpiTorso"};

    robot["EpiBlack"] = {.serialPortPupil = "/dev/cu.usbserial-FT4TCJXI",
                         .serialPortHead = "/dev/cu.usbserial-FT4TCGUT",
                         .serialPortLeftArm = "",
                         .serialPortRightArm = "",
                         .type = "Epi"};

    robotName = GetValue("robot");


    // Check if robotname exist in configuration
    if(robot.find(robotName) == robot.end())
        {
            Notify(msg_fatal_error,"%s is not supported", robotName.c_str());
            return;
        }

    EpiTorsoMode = (robot[robotName].type.compare("EpiTorso") == 0);
    EpiMode = (robot[robotName].type.compare("Epi") == 0);

    Notify(msg_debug, "Connecting to %s (%s)\n", robotName.c_str(), robot[robotName].type.c_str());


    // Input
    io(goalPosition, goalPositionSize, "GOAL_POSITION");
    io(goalCurrent, goalCurrentSize, "GOAL_CURRENT");

    // OUTPUT
    io(presentPosition, presentPositionSize, "PRESENT_POSITION");
    io(presentCurrent, presentCurrentSize, "PRESENT_CURRENT");

    // Simulate
    simulate = GetBoolValue("simulate");

    if (simulate)
    {
        Notify(msg_debug, "Simulate servos"); // msg_warning not printing?? loglevel bug?
        return;
    }

    // NOT WORKIGN CHECK!!
    // Check if the input size are correct
    if (EpiTorsoMode)
        if (goalPositionSize != goalCurrentSize && goalPositionSize != EPI_TORSO_NR_SERVOS && goalCurrentSize != EPI_TORSO_NR_SERVOS)
            Notify(msg_fatal_error, "Input size does not match robot type\n");

    if (EpiMode)
        if (goalPositionSize != goalCurrentSize && goalPositionSize != EPI_NR_SERVOS && goalCurrentSize != EPI_NR_SERVOS)
            Notify(msg_fatal_error, "Input size does not match robot type\n");



    // Add more robots here

   
    // Epi torso
    // =========
    if (EpiTorsoMode)
    {
        // Neck (id 2,3) =  2x MX106R Eyes = 2xMX28R (id 3,4)

        int dxl_comm_result;
        std::vector<uint8_t> vec; // Dynamixel data storages

        // Init Dynaxmixel SDK
        portHandlerHead = dynamixel::PortHandler::getPortHandler(robot[robotName].serialPortHead.c_str());
        packetHandlerHead = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION);

        // Open port
        if (portHandlerHead->openPort())
            Notify(msg_debug, "Succeeded to open the port!\n");
        else
            Notify(msg_fatal_error, "Failed to open the port!\n");

        // Set port baudrate
        if (portHandlerHead->setBaudRate(BAUDRATE3M))
            Notify(msg_debug, "Succeeded to change the baudrate!\n");
        else
            Notify(msg_fatal_error, "Failed to change the baudrate!\n");

        // Ping all the servos to make sure they are all there.
        dxl_comm_result = packetHandlerHead->broadcastPing(portHandlerHead, vec);
        if (dxl_comm_result != COMM_SUCCESS)
            Notify(msg_warning, "%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));

        Notify(msg_debug, "Detected Dynamixel (Head) : \n");
        for (int i = 0; i < (int)vec.size(); i++)
            Notify(msg_debug, "[ID:%03d]\n", vec.at(i));

        // Pupil (id 2,3) = XL320 Left eye, right eye
        // Init Dynaxmixel SDK
        portHandlerPupil = dynamixel::PortHandler::getPortHandler(robot[robotName].serialPortPupil.c_str());
        packetHandlerPupil = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION);

        // Open port
        if (portHandlerPupil->openPort())
            Notify(msg_debug, "Succeeded to open the port!\n");
        else
            Notify(msg_fatal_error, "Failed to open the port!\n");

        // Set port baudrate
        if (portHandlerPupil->setBaudRate(BAUDRATE1M))
            Notify(msg_debug, "Succeeded to change the baudrate!\n");
        else
            Notify(msg_fatal_error, "Failed to change the baudrate!\n");

        // Ping all the servos to make sure they are all there.
        dxl_comm_result = packetHandlerPupil->broadcastPing(portHandlerPupil, vec);
        if (dxl_comm_result != COMM_SUCCESS)
            Notify(msg_warning, "%s\n", packetHandlerPupil->getTxRxResult(dxl_comm_result));

        Notify(msg_debug, "Detected Dynamixel (pupil) : \n");
        for (int i = 0; i < (int)vec.size(); i++)
            Notify(msg_debug, "[ID:%03d]\n", vec.at(i));
    }
    else
    {
        Notify(msg_fatal_error, "Robot type is not yet implementet\n");
    }

    if (!SetDefaultSettingServo())
        Notify(msg_fatal_error, "Unable to write default settings on servoes\n");

    // Torqing up the servos? This can not be done in 2.0 and position mode only in position-current mode.
    int dxl_comm_result = COMM_TX_FAIL; // Communication result
    uint8_t dxl_error = 0;              // Dynamixel error

    // Enable DYNAMIXEL#1 Torque
    dxl_comm_result = packetHandlerHead->write1ByteTxRx(portHandlerHead, 2, 64, 1, &dxl_error);
    if (dxl_comm_result != COMM_SUCCESS)
    {
        printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
    }
    else if (dxl_error != 0)
    {
        printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));
    }
    else
    {
        printf("DYNAMIXEL#%d has been successfully connected \n", 1);
    }

    dxl_comm_result = packetHandlerHead->write1ByteTxRx(portHandlerHead, 3, 64, 1, &dxl_error);
    if (dxl_comm_result != COMM_SUCCESS)
    {
        printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
    }
    else if (dxl_error != 0)
    {
        printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));
    }
    else
    {
        printf("DYNAMIXEL#%d has been successfully connected \n", 1);
    }

    dxl_comm_result = packetHandlerHead->write1ByteTxRx(portHandlerHead, 4, 64, 1, &dxl_error);
    if (dxl_comm_result != COMM_SUCCESS)
    {
        printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
    }
    else if (dxl_error != 0)
    {
        printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));
    }
    else
    {
        printf("DYNAMIXEL#%d has been successfully connected \n", 1);
    }

    dxl_comm_result = packetHandlerHead->write1ByteTxRx(portHandlerHead, 5, 64, 1, &dxl_error);
    if (dxl_comm_result != COMM_SUCCESS)
    {
        printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
    }
    else if (dxl_error != 0)
    {
        printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));
    }
    else
    {
        printf("DYNAMIXEL#%d has been successfully connected \n", 1);
    }

    // Pupil
    dxl_comm_result = packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 24, 1, &dxl_error);
    if (dxl_comm_result != COMM_SUCCESS)
    {
        printf("%s\n", packetHandlerPupil->getTxRxResult(dxl_comm_result));
    }
    else if (dxl_error != 0)
    {
        printf("%s\n", packetHandlerPupil->getRxPacketError(dxl_error));
    }
    else
    {
        printf("DYNAMIXEL#%d has been successfully connected \n", 1);
    }

    dxl_comm_result = packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 24, 1, &dxl_error);
    if (dxl_comm_result != COMM_SUCCESS)
    {
        printf("%s\n", packetHandlerPupil->getTxRxResult(dxl_comm_result));
    }
    else if (dxl_error != 0)
    {
        printf("%s\n", packetHandlerPupil->getRxPacketError(dxl_error));
    }
    else
    {
        printf("DYNAMIXEL#%d has been successfully connected \n", 1);
    }
}

void EpiServo::Tick()
{

    if (simulate)
    {
        Notify(msg_debug, "Simulate servos ´"); // msg_warning not printing?? loglevel bug?
        if (EpiTorsoMode)
        {
            // Add dynamic change depending on realtime tick.
            float maxVel = 45.0/1000*10; // No timebase
            if (GetTickLength()!= 0)
                maxVel = 45.0/1000*GetTickLength(); // Maximum change in one second in degrees / timebase

            int index = 0;
            for (int i = 2; i <= 7; i++)
            {
                presentPosition[index] = presentPosition[index] + clip(goalPosition[index] - presentPosition[index], -maxVel, maxVel); 
                presentCurrent[index] = 0;                                                     // mA
                index++;
            }
        }
        return;
    }

    if (EpiTorsoMode)
    {
        // Read
        // Read 6 bytes from each servo using indirect mode. Present position and present current

        groupSyncWriteHead = new dynamixel::GroupSyncWrite(portHandlerHead, packetHandlerHead, 224, 4 + 2);
        groupSyncReadHead = new dynamixel::GroupSyncRead(portHandlerHead, packetHandlerHead, 634, 4 + 2 + 1);

        int index = 0;
        int dxl_comm_result = COMM_TX_FAIL; // Communication result
        bool dxl_addparam_result = false;   // addParam result
        bool dxl_getdata_result = false;    // GetParam result
        // int dxl_goal_position[2] = {DXL_MINIMUM_POSITION_VALUE, DXL_MAXIMUM_POSITION_VALUE};  // Goal position

        uint8_t dxl_error = 0; // Dynamixel error
        uint8_t param_goal_position[6];

        int32_t dxl_present_position = 0;
        int16_t dxl_present_current = 0;
        int8_t dxl_present_temperature = 0;

        // Add if for syncread
        for (int i = 2; i <= 5; i++)
            if (!groupSyncReadHead->addParam(i))
                Notify(msg_fatal_error, "[ID:%03d] groupSyncRead addparam failed", i);
        // else
        //     Notify(msg_debug, "[ID:%03d] groupSyncRead addparam YAY", i);

        // Sync read

        dxl_comm_resultHead = groupSyncReadHead->txRxPacket();
        // dxl_comm_resultHead = groupSyncReadHead->fastSyncReadTxRxPacket(); // Servoes probably needs to be updated. Not sure if all

        if (dxl_comm_resultHead != COMM_SUCCESS)
            printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
        // else
        //     printf("SUCCESS: %s\n", packetHandlerHead->getTxRxResult(dxl_comm_resultHead));

        // Check if data is available
        for (int i = 2; i <= 5; i++)
        {
            dxl_comm_resultHead = groupSyncReadHead->isAvailable(i, 634, 4 + 2 + 1);
            if (dxl_comm_resultHead != true)
            {
                fprintf(stderr, "[ID:%03d] groupSyncRead getdata failed\n", i);
            }
        }

        // Extract data
        index = 0;
        for (int i = 2; i <= 5; i++)
        {
            dxl_present_position = groupSyncReadHead->getData(i, 634, 4);    // Present postiion
            dxl_present_current = groupSyncReadHead->getData(i, 638, 2);     // Present current
            dxl_present_temperature = groupSyncReadHead->getData(i, 640, 1); // Present temperature
            // Notify(msg_debug, "ID %i:\tPresent position:%03d\tPresent current:%03d\tPresent current:%03d\n", i, dxl_present_position, dxl_present_current, dxl_present_temperature);

            // Fill IO
            presentPosition[index] = dxl_present_position / 4095.0 * 360.0; // degrees
            presentCurrent[index] = dxl_present_current * 3.36;             // mA
            index++;
        }

        print_array("goalPosition", goalPosition, 6);

        // Send (sync write)
        index = 0;
        for (int i = 2; i <= 5; i++)
        {
            int goal = goalPosition[index] / 360.0 * 4096.0;
            // printf("GOAL %i\n", goal);
            param_goal_position[0] = DXL_LOBYTE(DXL_LOWORD(goal));
            param_goal_position[1] = DXL_HIBYTE(DXL_LOWORD(goal));
            param_goal_position[2] = DXL_LOBYTE(DXL_HIWORD(goal));
            param_goal_position[3] = DXL_HIBYTE(DXL_HIWORD(goal));
            // Goal current is not available on MX28 writing

            int goalC = goalCurrent[index] / 3.36;
            // printf("Current %i\n", goalC);
            param_goal_position[4] = DXL_LOBYTE(DXL_HIWORD(goalC));
            param_goal_position[5] = DXL_HIBYTE(DXL_HIWORD(goalC));

            dxl_addparam_result = groupSyncWriteHead->addParam(i, param_goal_position, 6);

            if (dxl_addparam_result != true)
                Notify(msg_fatal_error, "[ID:%03d] groupSyncWrite addparam failed", i);

            index++;
        }

        // Syncwrite goal position
        dxl_comm_result = groupSyncWriteHead->txPacket();
        if (dxl_comm_result != COMM_SUCCESS)
            printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));

        // Clear syncwrite parameter storage
        groupSyncWriteHead->clearParam();

        // Send to pupil. Only goal position? No feedback?
        groupSyncWritePupil = new dynamixel::GroupSyncWrite(portHandlerPupil, packetHandlerPupil, 30, 2);

        index = 4;

        for (int i = 2; i <= 3; i++)
        {
            printf("SKICKAR");
            int goal = goalPosition[index] / 360.0 * 1023.0;
            printf("GOAL:%i\n", goal);
            // param_goal_position[0] = DXL_LOBYTE(DXL_HIWORD(goal));
            // param_goal_position[1] = DXL_HIBYTE(DXL_HIWORD(goal));
            uint16_t param_default;

            param_default = goalPosition[index] / 360.0 * 1023.0;
            if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, i, 30, param_default, &dxl_error))
                Notify(msg_fatal_error, "[ID:%03d] groupSyncWrite addparam failed", i);

            // dxl_addparam_result = groupSyncWritePupil->addParam(i, param_goal_position, 2);

            // if (dxl_addparam_result != true)
            //     Notify(msg_fatal_error, "[ID:%03d] groupSyncWrite addparam failed", i);

            index++;
        }

        // Syncwrite goal position
        // dxl_comm_result = groupSyncWritePupil->txPacket();
        // if (dxl_comm_result != COMM_SUCCESS)
        //     printf("%s\n", packetHandlerPupil->getTxRxResult(dxl_comm_result));

        // Clear syncwrite parameter storage
        // groupSyncWritePupil->clearParam();

        // Read servo.
        // What paramters should we read? (input of module?)

        // Write servo
        // What parameters should we write? (outputs of module)

        //		// Multi core 2

        // const int nrOfCores = 4;
        // // Create some threads pointers
        // thread tConv[nrOfCores];
        // //printf("Number of threads %i\n",thread::hardware_concurrency());
        // int span = size_x*size_y/nrOfCores;

        // for(int j=0; j<nrOfCores; j++)
        // {
        // 	tConv[j] = thread([&,j]()
        // 					  {
        // 						  float * r = red;
        // 						  float * g = green;
        // 						  float * b = blue;
        // 						  float * inte = intensity;
        // 						  int t = 0;

        // 						  unsigned char * d = data;

        // 						  for (int i = 0; i < j*span; i++)
        // 						  {
        // 							  d++;
        // 							  d++;
        // 							  d++;
        // 							  r++;
        // 							  g++;
        // 							  b++;
        // 							  inte++;
        // 						  }
        // 						  while(t++ < span)
        // 						  {
        // 							  *r       = convertIntToFloat[*d++];
        // 							  *g       = convertIntToFloat[*d++];
        // 							  *b       = convertIntToFloat[*d++];
        // 							  *inte++ = *r++ + *g++ + *b++;
        // 						  }
        // 					  });

        // }
        // for(int j=0; j<nrOfCores; j++)
        // 	tConv[j].join();
    }
}

// A function that set importat parameters in the control table.
// Baud rate and ID needs to be set manually.

bool EpiServo::SetDefaultSettingServo()
{
    printf("Setting control table on servos\n");

    // To be able to set some of the setting toruqe enable needs to be off.

    if (EpiTorsoMode)
    {
        // Indirect data
        int dxl_comm_result = COMM_TX_FAIL; // Communication result
        uint8_t dxl_error = 0;              // Dynamixel error

        // NECK/EYES
        for (int i = 2; i <= 5; i++)
        {
            // Indirect adresses for ikaros input
            // Goal position
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 168, 116, &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 168 + 2, 116 + 1, &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 168 + 4, 116 + 2, &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 168 + 6, 116 + 3, &dxl_error))
                return false;

            // Goal current (Only availabe on MX-106)
            if (i == 2 || i == 3)
            {
                if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 176, 102, &dxl_error))
                    return false;
                if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 176 + 2, 102 + 1, &dxl_error))
                    return false;
            }

            // For MX28. Write goal current to the control table heaven...
            if (i == 4 || i == 5)
            {
                if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 178, 228, &dxl_error))
                    return false;
                if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 178 + 2, 228 + 1, &dxl_error))
                    return false;
            }

            // Feedback
            // ********
            // Present position
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 578, 132, &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 578 + 2, 132 + 1, &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 578 + 4, 132 + 2, &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 578 + 6, 132 + 3, &dxl_error))
                return false;

            // Present current (Only availabe on MX-106)
            if (i == 2 || i == 3)
            {
                if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 586, 128, &dxl_error))
                    return false;
                if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 586 + 2, 128 + 1, &dxl_error))
                    return false;
            }
            // For MX28. Write goal current to the control table heaven...
            if (i == 4 || i == 5)
            {

                if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 586, 228, &dxl_error))
                    return false;
                if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 586 + 2, 228 + 1, &dxl_error))
                    return false;
            }

            // Present tempurature
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 590, 146, &dxl_error))
                return false;

            // Common settings for the seros
            uint32_t param_default;

            // Profile velocity
            param_default = 210;
            if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, i, 112, param_default, &dxl_error))
                return false;
        }

        // Specific setting for the servos
        // NECK ID 2
        uint32_t param_default;

        // Limit position max
        param_default = 2660;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 2, 48, param_default, &dxl_error))
            return false;

        // Limit position min
        param_default = 1460;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 2, 52, param_default, &dxl_error))
            return false;

        // Profile acceleration
        param_default = 50;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 2, 108, param_default, &dxl_error))
            return false;

        // NECK ID 3
        // Limit position max
        param_default = 2660;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 3, 48, param_default, &dxl_error))
            return false;

        // Limit position min
        param_default = 1500;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 3, 52, param_default, &dxl_error))
            return false;

        // Profile acceleration
        param_default = 50;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 3, 108, param_default, &dxl_error))
            return false;

        // Left eye 4
        // Limit position max
        param_default = 2300;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 4, 48, param_default, &dxl_error))
            return false;

        // Limit position min
        param_default = 1700;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 4, 52, param_default, &dxl_error))
            return false;

        // Profile acceleration
        param_default = 150;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 4, 108, param_default, &dxl_error))
            return false;

        // Right eye 5
        // Limit position max
        param_default = 2300;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 5, 48, param_default, &dxl_error))
            return false;

        // Limit position min
        param_default = 1700;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 5, 52, param_default, &dxl_error))
            return false;

        // Profile acceleration
        param_default = 150;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 5, 108, param_default, &dxl_error))
            return false;

        // PUPIL
        // Left
        // Limit position max
        uint16_t param_default_2Byte = 550;
        uint8_t param_default_1Byte;

        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 6, param_default_2Byte, &dxl_error))
            return false;

        // Limit position min
        param_default_2Byte = 801;
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 8, param_default_2Byte, &dxl_error))
            return false;

        // Moving speed
        param_default_2Byte = 150;
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 32, param_default_2Byte, &dxl_error))
            return false;

        // P
        param_default_1Byte = 100;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 29, param_default_1Byte, &dxl_error))
            return false;
        // I
        param_default_1Byte = 20;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 28, param_default_1Byte, &dxl_error))
            return false;
        // D
        param_default_1Byte = 5;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 27, param_default_1Byte, &dxl_error))
            return false;

        // Right
        // Limit position max
        param_default_2Byte = 351;
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 6, param_default_2Byte, &dxl_error))
            return false;

        // Limit position min
        param_default_2Byte = 601;
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 8, param_default_2Byte, &dxl_error))
            return false;

        // Moving speed
        param_default_2Byte = 150;
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 32, param_default_2Byte, &dxl_error))
            return false;

        // P
        param_default_1Byte = 100;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 29, param_default_1Byte, &dxl_error))
            return false;
        // I
        param_default_1Byte = 20;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 28, param_default_1Byte, &dxl_error))
            return false;
        // D
        param_default_1Byte = 5;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 27, param_default_1Byte, &dxl_error))
            return false;

        // LEFT ARM

        // RIGHT ARM

        // ...
    }
    return true;
}

EpiServo::~EpiServo()
{

    if (EpiTorsoMode)
    {
        // Close port
        portHandlerHead->closePort();
        portHandlerPupil->closePort();
    }
}
static InitClass init("EpiServo", &EpiServo::Create, "Source/UserModules/EpiServo/");
