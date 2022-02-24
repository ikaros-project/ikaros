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
#include <future>

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

#define EPI_TORSO_NR_SERVOS 6
#define EPI_NR_SERVOS 19

int EpiServo::comSeralPortPupil()
{

    Notify(msg_debug, "Comunication Pupil");


    int index = 0;
    int dxl_comm_result = COMM_TX_FAIL; // Communication result
    bool dxl_addparam_result = false;   // addParam result
    bool dxl_getdata_result = false;    // GetParam result

    uint8_t dxl_error = 0; // Dynamixel error


     // Send to pupil. Only goal position? No feedback?
    groupSyncWritePupil = new dynamixel::GroupSyncWrite(portHandlerPupil, packetHandlerPupil, 30, 2);


    index = 4;

    for (int i = 2; i <= 3; i++)
    {
        int goal = goalPosition[index] / 360.0 * 1023.0;
        uint16_t param_default;

        param_default = goalPosition[index] / 360.0 * 1023.0;
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, i, 30, param_default, &dxl_error)) // Takes a long time 31ms. 2x16ms?
            Notify(msg_fatal_error, "[ID:%03d] write2ByteTxRx failed", i);
        // if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxOnly(portHandlerPupil, i, 30, param_default)) // Takes a long time 2x16ms
        //     Notify(msg_fatal_error, "[ID:%03d] write2ByteTxOnly failed", i);

        // No feedback = temperature from pupil

        index++;
    }
    return (2);
}
int EpiServo::comSeralPortHead()
{
    Notify(msg_debug, "Comunication Head");

    groupSyncWriteHead = new dynamixel::GroupSyncWrite(portHandlerHead, packetHandlerHead, 224, 4 + 2);
    groupSyncReadHead = new dynamixel::GroupSyncRead(portHandlerHead, packetHandlerHead, 634, 4 + 2 + 1);

    int index = 0;
    int dxl_comm_result = COMM_TX_FAIL; // Communication result
    bool dxl_addparam_result = false;   // addParam result
    bool dxl_getdata_result = false;    // GetParam result

    uint8_t dxl_error = 0; // Dynamixel error
    uint8_t param_goal_position[6];

    int32_t dxl_present_position = 0;
    int16_t dxl_present_current = 0;
    int8_t dxl_present_temperature = 0;

    // Add if for syncread
    for (int i = 2; i <= 5; i++)
        if (!groupSyncReadHead->addParam(i))
            Notify(msg_fatal_error, "[ID:%03d] groupSyncRead addparam failed", i);

    // Sync read

    dxl_comm_resultHead = groupSyncReadHead->txRxPacket();
    // dxl_comm_resultHead = groupSyncReadHead->fastSyncReadTxRxPacket(); // Servoes probably needs to be updated.

    if (dxl_comm_resultHead != COMM_SUCCESS)
        printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));

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

        // Check temp
        if (dxl_present_temperature > 55) // 55 degrees
            Notify(msg_fatal_error, "Temperature over limit %i degrees (id %i)\n", dxl_present_temperature, i);

        index++;
    }
    // Send (sync write)
    index = 0;
    for (int i = 2; i <= 5; i++)
    {
        int goal = goalPosition[index] / 360.0 * 4096.0;
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

  
    return (1);
}

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
    if (robot.find(robotName) == robot.end())
    {
        Notify(msg_fatal_error, "%s is not supported", robotName.c_str());
        return;
    }

    // Check type of robot
    EpiTorsoMode = (robot[robotName].type.compare("EpiTorso") == 0);
    EpiMode = (robot[robotName].type.compare("Epi") == 0);

    Notify(msg_debug, "Connecting to %s (%s)\n", robotName.c_str(), robot[robotName].type.c_str());

    // Ikaros input
    io(goalPosition, goalPositionSize, "GOAL_POSITION");
    io(goalCurrent, goalCurrentSize, "GOAL_CURRENT");

    // Ikaros output
    io(presentPosition, presentPositionSize, "PRESENT_POSITION");
    io(presentCurrent, presentCurrentSize, "PRESENT_CURRENT");

    // Ikaros parameter simulate
    simulate = GetBoolValue("simulate");

    if (simulate)
    {
        Notify(msg_debug, "Simulate servos"); // msg_warning not printing?? loglevel bug?
        return;
    }

    // Check if the input size are correct
    if (EpiTorsoMode)
        if (goalPositionSize < EPI_TORSO_NR_SERVOS || goalCurrentSize < EPI_TORSO_NR_SERVOS)
        {
            Notify(msg_fatal_error, "Input size does not match robot type\n");
            return;
        }
    if (EpiMode)
        if (goalPositionSize < EPI_NR_SERVOS || goalCurrentSize < EPI_NR_SERVOS)
        {
            Notify(msg_fatal_error, "Input size does not match robot type\n");
            return;
        }

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
            Notify(msg_debug, "Succeeded to open serial port!\n");
        else
        {
            Notify(msg_fatal_error, "Failed to open serial port!\n");
            return;
        }

        // Set port baudrate
        if (portHandlerHead->setBaudRate(BAUDRATE3M))
            Notify(msg_debug, "Succeeded to change baudrate!\n");
        else
        {
            Notify(msg_fatal_error, "Failed to change baudrate!\n");
            return;
        }

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
            Notify(msg_debug, "Succeeded to open serial port!\n");
        else
        {
            Notify(msg_fatal_error, "Failed to open serial port!\n");
            return;
        }
        // Set port baudrate
        if (portHandlerPupil->setBaudRate(BAUDRATE1M))
            Notify(msg_debug, "Succeeded to change baudrate!\n");
        else
        {
            Notify(msg_fatal_error, "Failed to change baudrate!\n");
            return;
        }
        // Ping all the servos to make sure they are all there.
        dxl_comm_result = packetHandlerPupil->broadcastPing(portHandlerPupil, vec);
        if (dxl_comm_result != COMM_SUCCESS)
            Notify(msg_warning, "%s\n", packetHandlerPupil->getTxRxResult(dxl_comm_result));

        Notify(msg_debug, "Detected Dynamixel (pupil) : \n");
        for (int i = 0; i < (int)vec.size(); i++)
            Notify(msg_debug, "[ID:%03d]\n", vec.at(i));
    }
    else if (EpiMode)
    {
        Notify(msg_fatal_error, "Robot type (Full epi) is not yet implementet\n");
    }
    else
    {
        Notify(msg_fatal_error, "Robot type is not yet implementet\n");
    }

    if (!SetDefaultSettingServo())
        Notify(msg_fatal_error, "Unable to write default settings on servoes\n");

    if (!TorqueingUpServo())
        Notify(msg_fatal_error, "Unable torque up servos\n");
}

void EpiServo::Tick()
{
    if (simulate)
    {
        Notify(msg_debug, "Simulate servos ´"); // msg_warning not printing?? loglevel bug?
        if (EpiTorsoMode)
        {
            // Add dynamic change depending on realtime tick.
            float maxVel = 45.0 / 1000 * 10; // No timebase
            if (GetTickLength() != 0)
                maxVel = 45.0 / 1000 * GetTickLength(); // Maximum change in one second in degrees / timebase

            int index = 0;
            for (int i = 2; i <= 7; i++)
            {
                presentPosition[index] = presentPosition[index] + clip(goalPosition[index] - presentPosition[index], -maxVel, maxVel);
                presentCurrent[index] = 0; // mA
                index++;
            }
        }
        return;
    }

    if (EpiTorsoMode)
    {

        // Still need to be done.

        // 1. Set the order of servos = EPi FULL
        // 2. Seperate the threads/serial ports
        // 3. Clean up code
        // 4. Optimize. Test with FTDI Latency
        // 5. 

        // Optimize.
        // 1. Threads for each serial port.
        // 2. Latency timer FTDI issue.
        // 3. Stuatus Return Level.

        // Avg 32ms s1000 (No threads)

        //auto f = async(&cabc::pr,cap);
        //auto f=std::async(&capc::pr,&cap);
        //std::future<int> f = std::async(&EpiServo::comSeralPort1,this);
        //std::cout << "result: " << f.get() << std::endl;

        // auto future1 = std::async(this::comSeralPort1,this);
        // auto future2 = std::async(comSeralPort2);

        auto future1 = std::async(std::launch::async,&EpiServo::comSeralPortHead, this);  // 13 ms
        auto future2 = std::async(std::launch::async,&EpiServo::comSeralPortPupil, this); //  25 ms Head and pupil = 25
        int number1 = future1.get(); 
        int number2 = future2.get(); 

        //comSeralPortHead(); // 11 ms
        //comSeralPortPupil(); // 25.17 ms Head and pupil 37ms // comSeralPortPupil makes two write commands that gives two status message. Sync_write will probaly lower this to half

        // Setting comSeralPortPupil to not respond with status should reduce the time to <1ms

        // latency timer? This needs to be tested.

        // Read
        // Read 6 bytes from each servo using indirect mode. Present position and present current

        // groupSyncWriteHead = new dynamixel::GroupSyncWrite(portHandlerHead, packetHandlerHead, 224, 4 + 2);
        // groupSyncReadHead = new dynamixel::GroupSyncRead(portHandlerHead, packetHandlerHead, 634, 4 + 2 + 1);

        // int index = 0;
        // int dxl_comm_result = COMM_TX_FAIL; // Communication result
        // bool dxl_addparam_result = false;   // addParam result
        // bool dxl_getdata_result = false;    // GetParam result
        // // int dxl_goal_position[2] = {DXL_MINIMUM_POSITION_VALUE, DXL_MAXIMUM_POSITION_VALUE};  // Goal position

        // uint8_t dxl_error = 0; // Dynamixel error
        // uint8_t param_goal_position[6];

        // int32_t dxl_present_position = 0;
        // int16_t dxl_present_current = 0;
        // int8_t dxl_present_temperature = 0;

        // // Add if for syncread
        // for (int i = 2; i <= 5; i++)
        //     if (!groupSyncReadHead->addParam(i))
        //         Notify(msg_fatal_error, "[ID:%03d] groupSyncRead addparam failed", i);

        // // Sync read

        // dxl_comm_resultHead = groupSyncReadHead->txRxPacket();
        // // dxl_comm_resultHead = groupSyncReadHead->fastSyncReadTxRxPacket(); // Servoes probably needs to be updated.

        // if (dxl_comm_resultHead != COMM_SUCCESS)
        //     printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));

        // // Check if data is available
        // for (int i = 2; i <= 5; i++)
        // {
        //     dxl_comm_resultHead = groupSyncReadHead->isAvailable(i, 634, 4 + 2 + 1);
        //     if (dxl_comm_resultHead != true)
        //     {
        //         fprintf(stderr, "[ID:%03d] groupSyncRead getdata failed\n", i);
        //     }
        // }

        // // Extract data
        // index = 0;
        // for (int i = 2; i <= 5; i++)
        // {
        //     dxl_present_position = groupSyncReadHead->getData(i, 634, 4);    // Present postiion
        //     dxl_present_current = groupSyncReadHead->getData(i, 638, 2);     // Present current
        //     dxl_present_temperature = groupSyncReadHead->getData(i, 640, 1); // Present temperature
        //     // Notify(msg_debug, "ID %i:\tPresent position:%03d\tPresent current:%03d\tPresent current:%03d\n", i, dxl_present_position, dxl_present_current, dxl_present_temperature);

        //     // Fill IO
        //     presentPosition[index] = dxl_present_position / 4095.0 * 360.0; // degrees
        //     presentCurrent[index] = dxl_present_current * 3.36;             // mA

        //     // Check temp
        //     if (dxl_present_temperature > 55) // 55 degrees
        //         Notify(msg_fatal_error,"Temperature over limit %i degrees (id %i)\n",dxl_present_temperature,i);

        //     index++;

        // }

        //     // Send (sync write)
        //     index = 0;
        //     for (int i = 2; i <= 5; i++)
        //     {
        //         int goal = goalPosition[index] / 360.0 * 4096.0;
        //         param_goal_position[0] = DXL_LOBYTE(DXL_LOWORD(goal));
        //         param_goal_position[1] = DXL_HIBYTE(DXL_LOWORD(goal));
        //         param_goal_position[2] = DXL_LOBYTE(DXL_HIWORD(goal));
        //         param_goal_position[3] = DXL_HIBYTE(DXL_HIWORD(goal));
        //         // Goal current is not available on MX28 writing

        //         int goalC = goalCurrent[index] / 3.36;
        //         // printf("Current %i\n", goalC);
        //         param_goal_position[4] = DXL_LOBYTE(DXL_HIWORD(goalC));
        //         param_goal_position[5] = DXL_HIBYTE(DXL_HIWORD(goalC));

        //         dxl_addparam_result = groupSyncWriteHead->addParam(i, param_goal_position, 6);

        //         if (dxl_addparam_result != true)
        //             Notify(msg_fatal_error, "[ID:%03d] groupSyncWrite addparam failed", i);

        //         index++;
        //     }

        //     // Syncwrite goal position
        //     dxl_comm_result = groupSyncWriteHead->txPacket();
        //     if (dxl_comm_result != COMM_SUCCESS)
        //         printf("%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));

        //     // Clear syncwrite parameter storage
        //     groupSyncWriteHead->clearParam();

        //     // Send to pupil. Only goal position? No feedback?
        //     groupSyncWritePupil = new dynamixel::GroupSyncWrite(portHandlerPupil, packetHandlerPupil, 30, 2);

        //     index = 4;

        //     for (int i = 2; i <= 3; i++)
        //     {
        //         int goal = goalPosition[index] / 360.0 * 1023.0;
        //         uint16_t param_default;

        //         param_default = goalPosition[index] / 360.0 * 1023.0;
        //         if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, i, 30, param_default, &dxl_error))
        //             Notify(msg_fatal_error, "[ID:%03d] groupSyncWrite addparam failed", i);

        //         // No feedback = temperature from pupil

        //         index++;
        //     }
    }
}

// A function that set importat parameters in the control table.
// Baud rate and ID needs to be set manually.

bool EpiServo::SetDefaultSettingServo()
{
    Notify(msg_debug, "Setting control table on servos\n");

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

            // P
            uint16_t param_default_2Byte = 850;
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 84, param_default_2Byte, &dxl_error))
                return false;

            // I
            param_default_2Byte = 0;
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 82, param_default_2Byte, &dxl_error))
                return false;

            // D
            param_default_2Byte = 0;
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, 80, param_default_2Byte, &dxl_error))
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

bool EpiServo::TorqueingUpServo()
{
    // Trying to torque up the power of the servos.
    // Dynamixel protocel 2.0
    // In current base position control mode goal current can be used.
    // In poistion control mode P can be used (PID).
    // Torqing up the servos? This can not be done in 2.0 and position mode only in position-current mode.
    // 1. Set P (PID) = 0. Store start P value
    // 2. Set goal poistion to present position
    // 3. Increase current or P (PID)
    // 4. Repeat 2,3 for 2 seconds.

    int TorqueUpTime = 4000; //ms
    Timer t;

    int dxl_comm_result = COMM_TX_FAIL; // Communication result
    uint8_t dxl_error = 0;              // Dynamixel error

    if (EpiTorsoMode)
    {

        // 1.
        uint16_t start_p_value2 = 0;
        uint16_t start_p_value3 = 0;
        uint16_t start_p_value4 = 0;
        uint16_t start_p_value5 = 0;
        uint8_t start_p_value6 = 0;
        uint8_t start_p_value7 = 0;

        // Get P values
        if (COMM_SUCCESS != packetHandlerHead->read2ByteTxRx(portHandlerHead, 2, 84, &start_p_value2, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->read2ByteTxRx(portHandlerHead, 3, 84, &start_p_value3, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->read2ByteTxRx(portHandlerHead, 4, 84, &start_p_value4, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->read2ByteTxRx(portHandlerHead, 5, 84, &start_p_value5, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->read1ByteTxRx(portHandlerPupil, 2, 29, &start_p_value6, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->read1ByteTxRx(portHandlerPupil, 3, 29, &start_p_value7, &dxl_error))
            return false;
        // Set P value to 0
        if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, 2, 84, 0, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, 3, 84, 0, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, 4, 84, 0, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, 5, 84, 0, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 29, 0, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 29, 0, &dxl_error))
            return false;

        Notify(msg_debug, "Enable torque on servos");
        // Enable torque
        if (COMM_SUCCESS != packetHandlerHead->write1ByteTxRx(portHandlerHead, 2, 64, 1, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->write1ByteTxRx(portHandlerHead, 3, 64, 1, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->write1ByteTxRx(portHandlerHead, 4, 64, 1, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->write1ByteTxRx(portHandlerHead, 5, 64, 1, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 24, 1, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 24, 1, &dxl_error))
            return false;

        while (t.GetTime() < TorqueUpTime)
        {
            Notify(msg_debug, "Power up servos");
            // Get present position
            uint32_t present_postition_value2 = 0;
            uint32_t present_postition_value3 = 0;
            uint32_t present_postition_value4 = 0;
            uint32_t present_postition_value5 = 0;
            uint16_t present_postition_value6 = 0;
            uint16_t present_postition_value7 = 0;
            if (COMM_SUCCESS != packetHandlerHead->read4ByteTxRx(portHandlerHead, 2, 132, &present_postition_value2, &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerHead->read4ByteTxRx(portHandlerHead, 3, 132, &present_postition_value3, &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerHead->read4ByteTxRx(portHandlerHead, 4, 132, &present_postition_value4, &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerHead->read4ByteTxRx(portHandlerHead, 5, 132, &present_postition_value5, &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerPupil->read2ByteTxRx(portHandlerPupil, 2, 37, &present_postition_value6, &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerPupil->read2ByteTxRx(portHandlerPupil, 3, 37, &present_postition_value7, &dxl_error))
                return false;
            // Set goal position to present postiion
            if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 2, 116, present_postition_value2, &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 3, 116, present_postition_value3, &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 4, 116, present_postition_value4, &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 5, 116, present_postition_value5, &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 30, present_postition_value6, &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 30, present_postition_value7, &dxl_error))
                return false;
            // Ramping down P
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, 2, 84, int(float(start_p_value2) / float(TorqueUpTime) * t.GetTime()), &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, 3, 84, int(float(start_p_value3) / float(TorqueUpTime) * t.GetTime()), &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, 4, 84, int(float(start_p_value4) / float(TorqueUpTime) * t.GetTime()), &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, 5, 84, int(float(start_p_value5) / float(TorqueUpTime) * t.GetTime()), &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 29, int(float(start_p_value6) / float(TorqueUpTime) * t.GetTime()), &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 29, int(float(start_p_value7) / float(TorqueUpTime) * t.GetTime()), &dxl_error))
                return false;
        }
        // Set P value to start value
        if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, 2, 84, start_p_value2, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, 3, 84, start_p_value3, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, 4, 84, start_p_value4, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, 5, 84, start_p_value5, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 29, start_p_value6, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 29, start_p_value7, &dxl_error))
            return false;
        Notify(msg_debug, "Power up servos done");
    }

    return true;
}

bool EpiServo::TorqueingDownServo()
{
    // Power down servos.
    // 1. Store P (PID) value
    // 2. Ramp down P
    // 3. Turn of torque enable
    // 4. Set P (PID) valued from 1.

    int TorqueUpTime = 4000; //ms
    Timer t;

    int dxl_comm_result = COMM_TX_FAIL; // Communication result
    uint8_t dxl_error = 0;              // Dynamixel error

    if (EpiTorsoMode)
    {

        // 1.
        uint16_t start_p_value2 = 0;
        uint16_t start_p_value3 = 0;
        uint16_t start_p_value4 = 0;
        uint16_t start_p_value5 = 0;
        uint8_t start_p_value6 = 0;
        uint8_t start_p_value7 = 0;

        // Get P values
        if (COMM_SUCCESS != packetHandlerHead->read2ByteTxRx(portHandlerHead, 2, 84, &start_p_value2, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->read2ByteTxRx(portHandlerHead, 3, 84, &start_p_value3, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->read2ByteTxRx(portHandlerHead, 4, 84, &start_p_value4, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->read2ByteTxRx(portHandlerHead, 5, 84, &start_p_value5, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->read1ByteTxRx(portHandlerPupil, 2, 29, &start_p_value6, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->read1ByteTxRx(portHandlerPupil, 3, 29, &start_p_value7, &dxl_error))
            return false;

        t.Reset();

        Notify(msg_warning, "Power down servos. If needed support the head while power off the servos");

        while (t.GetTime() < TorqueUpTime)
        {
            // Set P value
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, 2, 84, int(float(start_p_value2) * (float(TorqueUpTime) - t.GetTime()) / float(TorqueUpTime)), &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, 3, 84, int(float(start_p_value3) * (float(TorqueUpTime) - t.GetTime()) / float(TorqueUpTime)), &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, 4, 84, int(float(start_p_value4) * (float(TorqueUpTime) - t.GetTime()) / float(TorqueUpTime)), &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, 5, 84, int(float(start_p_value5) * (float(TorqueUpTime) - t.GetTime()) / float(TorqueUpTime)), &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 29, int(float(start_p_value6) * (float(TorqueUpTime) - t.GetTime()) / float(TorqueUpTime)), &dxl_error))
                return false;
            if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 29, int(float(start_p_value7) * (float(TorqueUpTime) - t.GetTime()) / float(TorqueUpTime)), &dxl_error))
                return false;
        }

        // Get present position
        uint32_t present_postition_value2 = 0;
        uint32_t present_postition_value3 = 0;
        uint32_t present_postition_value4 = 0;
        uint32_t present_postition_value5 = 0;
        uint16_t present_postition_value6 = 0;
        uint16_t present_postition_value7 = 0;
        if (COMM_SUCCESS != packetHandlerHead->read4ByteTxRx(portHandlerHead, 2, 132, &present_postition_value2, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->read4ByteTxRx(portHandlerHead, 3, 132, &present_postition_value3, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->read4ByteTxRx(portHandlerHead, 4, 132, &present_postition_value4, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->read4ByteTxRx(portHandlerHead, 5, 132, &present_postition_value5, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->read2ByteTxRx(portHandlerPupil, 2, 37, &present_postition_value6, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->read2ByteTxRx(portHandlerPupil, 3, 37, &present_postition_value7, &dxl_error))
            return false;
        // Set goal position to present postiion
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 2, 116, present_postition_value2, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 3, 116, present_postition_value3, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 4, 116, present_postition_value4, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 5, 116, present_postition_value5, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 30, present_postition_value6, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 30, present_postition_value7, &dxl_error))
            return false;

        t.Restart();
        t.Sleep(2000); // What is a good value???

        Notify(msg_debug, "Enable torque off servos");
        // Enable torque
        if (COMM_SUCCESS != packetHandlerHead->write1ByteTxRx(portHandlerHead, 2, 64, 0, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->write1ByteTxRx(portHandlerHead, 3, 64, 0, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->write1ByteTxRx(portHandlerHead, 4, 64, 0, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->write1ByteTxRx(portHandlerHead, 5, 64, 0, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 24, 0, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 24, 0, &dxl_error))
            return false;

        // Set P value to start value
        if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, 2, 84, start_p_value2, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, 3, 84, start_p_value3, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, 4, 84, start_p_value4, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, 5, 84, start_p_value5, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 29, start_p_value6, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 29, start_p_value7, &dxl_error))
            return false;
    }
    return true;
}

EpiServo::~EpiServo()
{

    if (EpiTorsoMode)
    {
        // Torque down
        TorqueingDownServo();

        // Close port
        portHandlerHead->closePort();
        portHandlerPupil->closePort();
    }
}
static InitClass init("EpiServo", &EpiServo::Create, "Source/UserModules/EpiServo/");
