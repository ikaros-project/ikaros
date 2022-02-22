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
#define BAUDRATE 3000000

// Protocol version

#define DEVICENAME2 "/dev/cu.usbserial-FT4TCJXI"
#define DEVICENAME1 "/dev/cu.usbserial-FT4TCGUT"

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
#define EPI_FULL 1

#define EPI_TORSO_NR_SERVOS 4

void EpiServo::Init()
{

    robotType = GetIntValueFromList("robot");

    // Epi torso
    // =========
    if (robotType == EPI_TORSO)
    {
        Notify(msg_debug, "Connect to epi torso");
        // Neck (id 2,3) =  2x MX106R Eyes = 2xMX28R (id 3,4)

        // Init Dynaxmixel SDK
        portHandlerHead = dynamixel::PortHandler::getPortHandler(DEVICENAME1);
        packetHandlerHead = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION);

        // Open port
        if (portHandlerHead->openPort())
            Notify(msg_debug, "Succeeded to open the port!\n");
        else
            Notify(msg_fatal_error, "Failed to open the port!\n");

        // Set port baudrate
        if (portHandlerHead->setBaudRate(BAUDRATE))
            Notify(msg_debug, "Succeeded to change the baudrate!\n");
        else
            Notify(msg_fatal_error, "Failed to change the baudrate!\n");

        // Ping all the servos to make sure they are all there.
        dxl_comm_resultHead = packetHandlerHead->broadcastPing(portHandlerHead, vecHead);
        if (dxl_comm_resultHead != COMM_SUCCESS)
            Notify(msg_warning, "%s\n", packetHandlerHead->getTxRxResult(dxl_comm_resultHead));

        Notify(msg_debug, "Detected Dynamixel (Head) : \n");
        for (int i = 0; i < (int)vecHead.size(); i++)
            Notify(msg_debug, "[ID:%03d]\n", vecHead.at(i));

        // 2. Write defualt settings to servo. This is a good way to make sure we can replace a servo and not worried if all the setting are there.

        // Initialize GroupSyncWrite instance OUTPUT
    }
    else
    {
        Notify(msg_fatal_error, "Robot type is not yet implementet\n");
    }

    // Input
    io(goalPosition, goalPositionSize, "GOAL_POSITION");
    io(goalCurrent, goalCurrentSize, "GOAL_CURRENT");

    // OUTPUT
    io(presentPosition, presentPositionSize, "PRESENT_POSITION");
    io(presentCurrent, presentCurrentSize, "PRESENT_CURRENT");

    // Check if the input size are correct
    if (robotType == EPI_TORSO)
        if (goalPositionSize != goalCurrentSize && goalPositionSize != EPI_TORSO_NR_SERVOS && goalCurrentSize != EPI_TORSO_NR_SERVOS)
            Notify(msg_fatal_error, "Input size does not match robot type\n");

    // Inderect data
    int dxl_comm_result = COMM_TX_FAIL; // Communication result
    uint8_t dxl_error = 0;              // Dynamixel error

    for (int i = 2; i <= 5; i++)
    {
        printf("ID: %i\n", i);

        // Goal position 4 bytes
        dxl_comm_result = packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 168, 116, &dxl_error);
        if (dxl_comm_result != COMM_SUCCESS)
            printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
        else if (dxl_error != 0)
            printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));

        dxl_comm_result = packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 168 + 2, 116 + 1, &dxl_error);
        if (dxl_comm_result != COMM_SUCCESS)
            printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
        else if (dxl_error != 0)
            printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));

        dxl_comm_result = packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 168 + 4, 116 + 2, &dxl_error);
        if (dxl_comm_result != COMM_SUCCESS)
            printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
        else if (dxl_error != 0)
            printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));

        dxl_comm_result = packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 168 + 6, 116 + 3, &dxl_error);
        if (dxl_comm_result != COMM_SUCCESS)
            printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
        else if (dxl_error != 0)
            printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));

        if (i < 4)
        {
            // Goal current (ONLY MX 106?!!)
            dxl_comm_result = packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 176, 102, &dxl_error);
            if (dxl_comm_result != COMM_SUCCESS)
                printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
            else if (dxl_error != 0)
                printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));

            dxl_comm_result = packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 176 + 2, 102 + 1, &dxl_error);
            if (dxl_comm_result != COMM_SUCCESS)
                printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
            else if (dxl_error != 0)
                printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));
        }
        else
        {
            // For MX28. Write goal current to the control table heaven...
            dxl_comm_result = packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 178, 228, &dxl_error);
            if (dxl_comm_result != COMM_SUCCESS)
                printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
            else if (dxl_error != 0)
                printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));

            dxl_comm_result = packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 180 + 2, 228 + 1, &dxl_error);
            if (dxl_comm_result != COMM_SUCCESS)
                printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
            else if (dxl_error != 0)
                printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));
        }

        // Feedback
        // ********
        // Present position 4 bytes
        dxl_comm_result = packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 578, 132, &dxl_error);
        if (dxl_comm_result != COMM_SUCCESS)
            printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
        else if (dxl_error != 0)
            printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));

        dxl_comm_result = packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 578 + 2, 132 + 1, &dxl_error);
        if (dxl_comm_result != COMM_SUCCESS)
            printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
        else if (dxl_error != 0)
            printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));

        dxl_comm_result = packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 578 + 4, 132 + 2, &dxl_error);
        if (dxl_comm_result != COMM_SUCCESS)
            printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
        else if (dxl_error != 0)
            printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));

        dxl_comm_result = packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 578 + 6, 132 + 3, &dxl_error);
        if (dxl_comm_result != COMM_SUCCESS)
            printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
        else if (dxl_error != 0)
            printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));

        if (i < 4)
        {
            // Goal current (ONLY MX 106?!!)
            dxl_comm_result = packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 586, 128, &dxl_error);
            if (dxl_comm_result != COMM_SUCCESS)
                printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
            else if (dxl_error != 0)
                printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));

            dxl_comm_result = packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 586 + 2, 128 + 1, &dxl_error);
            if (dxl_comm_result != COMM_SUCCESS)
                printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
            else if (dxl_error != 0)
                printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));
        }
        else
        {
            dxl_comm_result = packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 586, 228, &dxl_error);
            if (dxl_comm_result != COMM_SUCCESS)
                printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
            else if (dxl_error != 0)
                printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));

            dxl_comm_result = packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 586 + 2, 228 + 1, &dxl_error);
            if (dxl_comm_result != COMM_SUCCESS)
                printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
            else if (dxl_error != 0)
                printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));
        }
        // Present tempurature
        dxl_comm_result = packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 590, 146, &dxl_error);
        if (dxl_comm_result != COMM_SUCCESS)
            printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
        else if (dxl_error != 0)
            printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));

        // Settings
        uint32_t param_default;

        // Profile acceleration
        param_default = 50;
        dxl_comm_result = packetHandlerHead->write4ByteTxRx(portHandlerHead, i, 108, param_default, &dxl_error);
        if (dxl_comm_result != COMM_SUCCESS)
            printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
        else if (dxl_error != 0)
            printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));

        // Profile velocity
        param_default = 210;
        dxl_comm_result = packetHandlerHead->write4ByteTxRx(portHandlerHead, i, 112, param_default, &dxl_error);
        if (dxl_comm_result != COMM_SUCCESS)
            printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
        else if (dxl_error != 0)
            printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));

        // Specific setting for ID
        if (i == 2) // Neck
        {
            // Limit position min
            param_default = 2660;
            dxl_comm_result = packetHandlerHead->write4ByteTxRx(portHandlerHead, i, 48, param_default, &dxl_error);
            if (dxl_comm_result != COMM_SUCCESS)
                printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
            else if (dxl_error != 0)
                printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));

            // Limit position max
            param_default = 1460;
            dxl_comm_result = packetHandlerHead->write4ByteTxRx(portHandlerHead, i, 52, param_default, &dxl_error);
            if (dxl_comm_result != COMM_SUCCESS)
                printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));
            else if (dxl_error != 0)
                printf("%s\n", packetHandlerHead->getRxPacketError(dxl_error));
        }
    }

    // Torqing up the servos?

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
}

void EpiServo::Tick()
{

    if (robotType == EPI_TORSO)
    {
        // Read
        // Read 6 bytes from each servo using indirect mode. Present position and present current

        groupSyncWriteHead = new dynamixel::GroupSyncWrite(portHandlerHead, packetHandlerHead, 224, 4 + 2);
        groupSyncReadHead = new dynamixel::GroupSyncRead(portHandlerHead, packetHandlerHead, 634, 4 + 2 + 1);

        int index = 0;
        int dxl_comm_result = COMM_TX_FAIL; // Communication result
        bool dxl_addparam_result = false;   // addParam result
        bool dxl_getdata_result = false;    // GetParam result
        //int dxl_goal_position[2] = {DXL_MINIMUM_POSITION_VALUE, DXL_MAXIMUM_POSITION_VALUE};  // Goal position

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
        //dxl_comm_resultHead = groupSyncReadHead->fastSyncReadTxRxPacket(); // Servoes probably needs to be updated. Not sure if all

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
            presentCurrent[index] = dxl_present_current * 3.36;             //mA
            index++;
        }

        //print_array("goalPosition", goalPosition, 4);

        // Send (sync write)
        index = 0;
        for (int i = 2; i <= 5; i++)
        {
            int goal = goalPosition[index] / 360.0 * 4096.0;
            //printf("GOAL %i\n", goal);
            param_goal_position[0] = DXL_LOBYTE(DXL_LOWORD(goal));
            param_goal_position[1] = DXL_HIBYTE(DXL_LOWORD(goal));
            param_goal_position[2] = DXL_LOBYTE(DXL_HIWORD(goal));
            param_goal_position[3] = DXL_HIBYTE(DXL_HIWORD(goal));
            // Goal current is not available on MX28 writing

            int goalC = goalCurrent[index] / 3.36;
            //printf("Current %i\n", goalC);
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

bool EpiServo::SetDefaultSettingServo()
{

}

EpiServo::~EpiServo()
{

    if (robotType == EPI_TORSO) // EpiTorso
    {
        // Close port
        portHandlerHead->closePort();
    }
}
static InitClass init("EpiServo", &EpiServo::Create, "Source/UserModules/EpiServo/");
