//
//	EpiServos.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2022 Birger Johansson

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

// TODO:
// Add mm input for pupil.
// Test return status for servos
// Add fast sync write feature
// Set limits for Arms and body

#include "EpiServos.h"

#include <stdio.h>
#include <vector> // Data from dynamixel sdk
#include <future> // Threads

#include "dynamixel_sdk.h" // Uses Dynamixel SDK library

using namespace ikaros;

bool EpiServos::CommunicationPupil()
{
    Notify(msg_trace, "Comunication Pupil");

    int index = 0;
    int dxl_comm_result = COMM_TX_FAIL; // Communication result
    bool dxl_addparam_result = false;   // addParam result
    bool dxl_getdata_result = false;    // GetParam result
    uint8_t dxl_error = 0;              // Dynamixel error

    // Send to pupil. No feedback

    index = PUPIL_INDEX_IO;

    for (int i = PUPIL_ID_MIN; i <= PUPIL_ID_MAX; i++)
    {
        if (torqueEnable)
        {
            uint8_t param_default = torqueEnable[index];
            if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, i, 24, param_default, &dxl_error))
                Notify(msg_fatal_error, "[ID:%03d] write1ByteTxRx failed", i);
        }
        if (goalPosition)
        {
            uint16_t param_default = goalPosition[index] / 360.0 * 1023.0;
            // Goal postiion feature/bug. If torque enable = 0 and goal position is sent. Torque enable will be 1.
            if (!torqueEnable)
            {
                if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, i, 30, param_default, &dxl_error)) // Takes a long time 31ms. 2x16ms?
                    Notify(msg_fatal_error, "[ID:%03d] write2ByteTxRx failed", i);
            }
            else if ((uint8_t)torqueEnable[index] != 0)
            {
                if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, i, 30, param_default, &dxl_error)) // Takes a long time 31ms. 2x16ms?
                    Notify(msg_fatal_error, "[ID:%03d] write2ByteTxRx failed", i);
            }
            // if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxOnly(portHandlerPupil, i, 30, param_default)) // Takes a long time 2x16ms
            //     Notify(msg_fatal_error, "[ID:%03d] write2ByteTxOnly failed", i);
        }
        else
        {
            Notify(msg_fatal_error, "Running module without a goal position input is not supported.");
            return false;
        }
        // XL 320 has no current position mode. Ignores goal current input
        // No feedback from pupils. Also no temperature check. Bad idea?
        index++;
    }
    return (true);
}
bool EpiServos::CommunicationHead()
{
    Notify(msg_debug, "Comunication Head");

    int index = 0;
    int dxl_comm_result = COMM_TX_FAIL; // Communication result
    bool dxl_addparam_result = false;   // addParam result
    bool dxl_getdata_result = false;    // GetParam result

    uint8_t dxl_error = 0;       // Dynamixel error
    uint8_t param_sync_write[7]; // 7 byte sync write is not supported for the DynamixelSDK

    int32_t dxl_present_position = 0;
    int16_t dxl_present_current = 0;
    int8_t dxl_present_temperature = 0;

    // Add if for syncread
    for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++)
        if (!groupSyncReadHead->addParam(i))
            Notify(msg_fatal_error, "[ID:%03d] groupSyncRead addparam failed", i);

    // Sync read
    dxl_comm_result = groupSyncReadHead->txRxPacket();
    // dxl_comm_result = groupSyncReadHead->fastSyncReadTxRxPacket(); // Servoes probably needs to be updated.

    if (dxl_comm_result != COMM_SUCCESS)
        Notify(msg_fatal_error, "%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));

    // Check if data is available
    for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++)
    {
        dxl_comm_result = groupSyncReadHead->isAvailable(i, 634, 4 + 2 + 1);
        if (dxl_comm_result != true)
            Notify(msg_fatal_error, "[ID:%03d] groupSyncRead getdata failed", i);
        // fprintf(stderr, "[ID:%03d] groupSyncRead getdata failed\n", i);
    }

    // Extract data
    index = HEAD_INDEX_IO;
    for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++)
    {
        dxl_present_position = groupSyncReadHead->getData(i, 634, 4);    // Present postiion
        dxl_present_current = groupSyncReadHead->getData(i, 638, 2);     // Present current
        dxl_present_temperature = groupSyncReadHead->getData(i, 640, 1); // Present temperature
        // Fill IO
        presentPosition[index] = dxl_present_position / 4095.0 * 360.0; // degrees
        presentCurrent[index] = dxl_present_current * 3.36;             // mA
        // Check temp
        if (dxl_present_temperature > MAX_TEMPERATURE)
            Notify(msg_fatal_error, "Temperature over limit %i degrees (id %i)\n", dxl_present_temperature, i);
        index++;
    }

    // Send (sync write)
    index = HEAD_INDEX_IO;
    for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++)
    {
        // We always write torque enable. If no input use torque enable = 1
        if (torqueEnable)
            param_sync_write[0] = (uint8_t)torqueEnable[index];
        else
            param_sync_write[0] = 1; // Torque on

        // If no goal position input is connected send present position
        if (goalPosition)
        {
            int value = goalPosition[index] / 360.0 * 4096.0;
            param_sync_write[1] = DXL_LOBYTE(DXL_LOWORD(value));
            param_sync_write[2] = DXL_HIBYTE(DXL_LOWORD(value));
            param_sync_write[3] = DXL_LOBYTE(DXL_HIWORD(value));
            param_sync_write[4] = DXL_HIBYTE(DXL_HIWORD(value));
        }
        else
        {
            Notify(msg_fatal_error, "Running module without a goal position input is not supported.");
            return false;
        }

        if (goalCurrent)
        {
            // Goal current is not available on MX28 writing
            int value = goalCurrent[index] / 3.36;
            param_sync_write[5] = DXL_LOBYTE(DXL_HIWORD(value));
            param_sync_write[6] = DXL_HIBYTE(DXL_HIWORD(value));
        }
        else
        {
            int value = 2047.0 / 3.36;
            param_sync_write[5] = DXL_LOBYTE(DXL_HIWORD(value));
            param_sync_write[6] = DXL_HIBYTE(DXL_HIWORD(value));
        }
        dxl_addparam_result = groupSyncWriteHead->addParam(i, param_sync_write, 7);

        if (dxl_addparam_result != true)
            Notify(msg_fatal_error, "[ID:%03d] groupSyncWrite addparam failed", i);

        index++;
    }

    // Syncwrite
    dxl_comm_result = groupSyncWriteHead->txPacket();
    if (dxl_comm_result != COMM_SUCCESS)
        Notify(msg_fatal_error, "%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));

    // Clear syncwrite parameter storage
    groupSyncWriteHead->clearParam();
    groupSyncReadHead->clearParam();

    return (true);
}

bool EpiServos::CommunicationBody()
{
    if (!EpiMode)
        return true;

    Notify(msg_debug, "Comunication Body");

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
    for (int i = BODY_ID_MIN; i <= BODY_ID_MIN; i++)
        if (!groupSyncReadBody->addParam(i))
            Notify(msg_fatal_error, "[ID:%03d] groupSyncRead addparam failed", i);

    // Sync read

    dxl_comm_result = groupSyncReadBody->txRxPacket();
    // dxl_comm_result = groupSyncReadBody->fastSyncReadTxRxPacket(); // Servoes probably needs to be updated.

    if (dxl_comm_result != COMM_SUCCESS)
        Notify(msg_fatal_error, "%s\n", packetHandlerBody->getTxRxResult(dxl_comm_result));

    // Check if data is available
    for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
    {
        dxl_comm_result = groupSyncReadBody->isAvailable(i, 634, 4 + 2 + 1);
        if (dxl_comm_result != true)
            Notify(msg_fatal_error, "[ID:%03d] groupSyncRead getdata failed\n", i);
    }

    // Extract data
    index = BODY_INDEX_IO;
    for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
    {
        dxl_present_position = groupSyncReadBody->getData(i, 634, 4);    // Present postiion
        dxl_present_current = groupSyncReadBody->getData(i, 638, 2);     // Present current
        dxl_present_temperature = groupSyncReadBody->getData(i, 640, 1); // Present temperature
        // Notify(msg_debug, "ID %i:\tPresent position:%03d\tPresent current:%03d\tPresent current:%03d\n", i, dxl_present_position, dxl_present_current, dxl_present_temperature);

        // Fill IO
        presentPosition[index] = dxl_present_position / 4095.0 * 360.0; // degrees
        presentCurrent[index] = dxl_present_current * 3.36;             // mA

        // Check temp
        if (dxl_present_temperature > MAX_TEMPERATURE)
            Notify(msg_fatal_error, "Temperature over limit %i degrees (id %i)\n", dxl_present_temperature, i);

        index++;
    }
    // Send (sync write)
    index = BODY_INDEX_IO;
    for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
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

        dxl_addparam_result = groupSyncWriteBody->addParam(i, param_goal_position, 6);

        if (dxl_addparam_result != true)
            Notify(msg_fatal_error, "[ID:%03d] groupSyncWrite addparam failed", i);

        index++;
    }

    // Syncwrite goal position
    dxl_comm_result = groupSyncWriteBody->txPacket();
    if (dxl_comm_result != COMM_SUCCESS)
        Notify(msg_fatal_error, "%s\n", packetHandlerBody->getTxRxResult(dxl_comm_result));

    // Clear syncwrite parameter storage
    groupSyncWriteBody->clearParam();

    return (true);
}
bool EpiServos::CommunicationLeftArm()
{
    if (!EpiMode)
        return true;

    Notify(msg_debug, "Comunication Left arm");

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
    for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
        if (!groupSyncReadLeftArm->addParam(i))
            Notify(msg_fatal_error, "[ID:%03d] groupSyncRead addparam failed", i);

    // Sync read
    dxl_comm_result = groupSyncReadLeftArm->txRxPacket();
    // dxl_comm_result = groupSyncReadLeftArm->fastSyncReadTxRxPacket(); // Servoes probably needs to be updated.

    if (dxl_comm_result != COMM_SUCCESS)
        Notify(msg_fatal_error, "%s\n", packetHandlerLeftArm->getTxRxResult(dxl_comm_result));

    // Check if data is available
    for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
    {
        dxl_comm_result = groupSyncReadLeftArm->isAvailable(i, 634, 4 + 2 + 1);
        if (dxl_comm_result != true)
            Notify(msg_fatal_error, "[ID:%03d] groupSyncRead getdata failed\n", i);
    }

    // Extract data
    index = LEFT_ARM_INDEX_IO;
    for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
    {
        dxl_present_position = groupSyncReadLeftArm->getData(i, 634, 4);    // Present postiion
        dxl_present_current = groupSyncReadLeftArm->getData(i, 638, 2);     // Present current
        dxl_present_temperature = groupSyncReadLeftArm->getData(i, 640, 1); // Present temperature
        // Notify(msg_debug, "ID %i:\tPresent position:%03d\tPresent current:%03d\tPresent current:%03d\n", i, dxl_present_position, dxl_present_current, dxl_present_temperature);

        // Fill IO
        presentPosition[index] = dxl_present_position / 4095.0 * 360.0; // degrees
        presentCurrent[index] = dxl_present_current * 3.36;             // mA

        // Check temp
        if (dxl_present_temperature > MAX_TEMPERATURE)
            Notify(msg_fatal_error, "Temperature over limit %i degrees (id %i)\n", dxl_present_temperature, i);

        index++;
    }
    // Send (sync write)
    index = LEFT_ARM_INDEX_IO;
    for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
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

        dxl_addparam_result = groupSyncWriteLeftArm->addParam(i, param_goal_position, 6);

        if (dxl_addparam_result != true)
            Notify(msg_fatal_error, "[ID:%03d] groupSyncWrite addparam failed", i);

        index++;
    }

    // Syncwrite goal position
    dxl_comm_result = groupSyncWriteLeftArm->txPacket();
    if (dxl_comm_result != COMM_SUCCESS)
        Notify(msg_fatal_error, "%s\n", packetHandlerLeftArm->getTxRxResult(dxl_comm_result));

    // Clear syncwrite parameter storage
    groupSyncWriteLeftArm->clearParam();

    return (true);
}
bool EpiServos::CommunicationRightArm()
{
    if (!EpiMode)
        return true;
    Notify(msg_debug, "Comunication Right arm");

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
    for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
        if (!groupSyncReadRightArm->addParam(i))
            Notify(msg_fatal_error, "[ID:%03d] groupSyncRead addparam failed", i);

    // Sync read

    dxl_comm_result = groupSyncReadRightArm->txRxPacket();
    // dxl_comm_resultHead = groupSyncReadRightArm->fastSyncReadTxRxPacket(); // Servoes probably needs to be updated.

    if (dxl_comm_result != COMM_SUCCESS)
        Notify(msg_fatal_error, "%s\n", packetHandlerRightArm->getTxRxResult(dxl_comm_result));

    // Check if data is available
    for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
    {
        dxl_comm_result = groupSyncReadRightArm->isAvailable(i, 634, 4 + 2 + 1);
        if (dxl_comm_result != true)
            Notify(msg_fatal_error, "[ID:%03d] groupSyncRead getdata failed\n", i);
    }

    // Extract data
    index = RIGHT_ARM_INDEX_IO;
    for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
    {
        dxl_present_position = groupSyncReadRightArm->getData(i, 634, 4);    // Present postiion
        dxl_present_current = groupSyncReadRightArm->getData(i, 638, 2);     // Present current
        dxl_present_temperature = groupSyncReadRightArm->getData(i, 640, 1); // Present temperature
        // Notify(msg_debug, "ID %i:\tPresent position:%03d\tPresent current:%03d\tPresent current:%03d\n", i, dxl_present_position, dxl_present_current, dxl_present_temperature);

        // Fill IO
        presentPosition[index] = dxl_present_position / 4095.0 * 360.0; // degrees
        presentCurrent[index] = dxl_present_current * 3.36;             // mA

        // Check temp
        if (dxl_present_temperature > MAX_TEMPERATURE)
            Notify(msg_fatal_error, "Temperature over limit %i degrees (id %i)\n", dxl_present_temperature, i);

        index++;
    }
    // Send (sync write)
    index = RIGHT_ARM_INDEX_IO;
    for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
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

        dxl_addparam_result = groupSyncWriteRightArm->addParam(i, param_goal_position, 6);

        if (dxl_addparam_result != true)
            Notify(msg_fatal_error, "[ID:%03d] groupSyncWrite addparam failed", i);

        index++;
    }

    // Syncwrite goal position
    dxl_comm_result = groupSyncWriteRightArm->txPacket();
    if (dxl_comm_result != COMM_SUCCESS)
        Notify(msg_fatal_error, "%s\n", packetHandlerRightArm->getTxRxResult(dxl_comm_result));

    // Clear syncwrite parameter storage
    groupSyncWriteRightArm->clearParam();

    return (true);
}

void EpiServos::Init()
{
    // Robots configurations
    robot["EpiWhite"] = {.serialPortPupil = "/dev/cu.usbserial-FT4TCJXI",
                         .serialPortHead = "/dev/cu.usbserial-FT4TCGUT",
                         .serialPortBody = "",
                         .serialPortLeftArm = "",
                         .serialPortRightArm = "",
                         .type = "EpiTorso"};

    robot["EpiYellow"] = {.serialPortPupil = "/dev/cu.usbserial-FT4TCJXI",
                          .serialPortHead = "/dev/cu.usbserial-FT4TCGUT",
                          .serialPortBody = "",
                          .serialPortLeftArm = "",
                          .serialPortRightArm = "",
                          .type = "EpiTorso"};

    robot["EpiGreen"] = {.serialPortPupil = "/dev/cu.usbserial-FT4TCJXI",
                         .serialPortHead = "/dev/cu.usbserial-FT4TCGUT",
                         .serialPortBody = "",
                         .serialPortLeftArm = "",
                         .serialPortRightArm = "",
                         .type = "EpiTorso"};

    robot["EpiBlue"] = {.serialPortPupil = "/dev/cu.usbserial-FT4TCJXI",
                        .serialPortHead = "/dev/cu.usbserial-FT4TCGUT",
                        .serialPortBody = "",
                        .serialPortLeftArm = "",
                        .serialPortRightArm = "",
                        .type = "EpiTorso"};

    robot["EpiGray"] = {.serialPortPupil = "/dev/cu.usbserial-FT4TCJXI",
                        .serialPortHead = "/dev/cu.usbserial-FT4TCGUT",
                        .serialPortBody = "",
                        .serialPortLeftArm = "",
                        .serialPortRightArm = "",
                        .type = "EpiTorso"};

    robot["EpiBlack"] = {.serialPortPupil = "/dev/cu.usbserial-FT4TCJXI",
                         .serialPortHead = "/dev/cu.usbserial-FT4TCGUT",
                         .serialPortBody = "",
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
    io(torqueEnable, torqueEnableSize, "TORQUE_ENABLE");
    io(goalPosition, goalPositionSize, "GOAL_POSITION");
    io(goalCurrent, goalCurrentSize, "GOAL_CURRENT");

    // Ikaros output
    io(presentPosition, presentPositionSize, "PRESENT_POSITION");
    io(presentCurrent, presentCurrentSize, "PRESENT_CURRENT");

    // Check if the input size are correct. We do not need to have an input at all!
    if (EpiTorsoMode)
    {
        if (goalPosition)
            if (goalPositionSize < EPI_TORSO_NR_SERVOS)
                Notify(msg_fatal_error, "Input size goal position does not match robot type\n");
        if (goalCurrent)
            if (goalCurrentSize < EPI_TORSO_NR_SERVOS)
                Notify(msg_fatal_error, "Input size goal current does not match robot type\n");
        if (torqueEnable)
            if (torqueEnableSize < EPI_TORSO_NR_SERVOS)
                Notify(msg_fatal_error, "Input size torque enable does not match robot type\n");
    }
    else if (EpiMode)
    {
        if (goalPosition)
            if (goalPositionSize < EPI_NR_SERVOS)
                Notify(msg_fatal_error, "Input size goal position does not match robot type\n");
        if (goalCurrent)
            if (goalCurrentSize < EPI_NR_SERVOS)
                Notify(msg_fatal_error, "Input size goal current does not match robot type\n");
        if (torqueEnable)
            if (torqueEnableSize < EPI_NR_SERVOS)
                Notify(msg_fatal_error, "Input size torque enable does not match robot type\n");
    }

    // Ikaros parameter simulate
    simulate = GetBoolValue("simulate");
    if (simulate)
    {
        Notify(msg_warning, "Simulate servos");
        return;
    }

    // Epi torso
    // =========
    if (EpiTorsoMode || EpiMode)
    {
        int dxl_comm_result;
        std::vector<uint8_t> vec;

        // Neck/Eyes (id 2,3) =  2x MX106R Eyes = 2xMX28R (id 3,4)

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
        int dxl_comm_result;
        std::vector<uint8_t> vec;

        // Left arm 6x MX106R 1 MX28R

        // Init Dynaxmixel SDK
        portHandlerLeftArm = dynamixel::PortHandler::getPortHandler(robot[robotName].serialPortLeftArm.c_str());
        packetHandlerLeftArm = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION);

        // Open port
        if (portHandlerLeftArm->openPort())
            Notify(msg_debug, "Succeeded to open serial port!\n");
        else
        {
            Notify(msg_fatal_error, "Failed to open serial port!\n");
            return;
        }
        // Set port baudrate
        if (portHandlerLeftArm->setBaudRate(BAUDRATE1M))
            Notify(msg_debug, "Succeeded to change baudrate!\n");
        else
        {
            Notify(msg_fatal_error, "Failed to change baudrate!\n");
            return;
        }
        // Ping all the servos to make sure they are all there.
        dxl_comm_result = packetHandlerLeftArm->broadcastPing(portHandlerLeftArm, vec);
        if (dxl_comm_result != COMM_SUCCESS)
            Notify(msg_warning, "%s\n", packetHandlerLeftArm->getTxRxResult(dxl_comm_result));

        Notify(msg_debug, "Detected Dynamixel (Left arm) : \n");
        for (int i = 0; i < (int)vec.size(); i++)
            Notify(msg_debug, "[ID:%03d]\n", vec.at(i));

        // Right arm 6x MX106R 1 MX28R

        // Init Dynaxmixel SDK
        portHandlerRightArm = dynamixel::PortHandler::getPortHandler(robot[robotName].serialPortRightArm.c_str());
        packetHandlerRightArm = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION);

        // Open port
        if (portHandlerRightArm->openPort())
            Notify(msg_debug, "Succeeded to open serial port!\n");
        else
        {
            Notify(msg_fatal_error, "Failed to open serial port!\n");
            return;
        }
        // Set port baudrate
        if (portHandlerRightArm->setBaudRate(BAUDRATE1M))
            Notify(msg_debug, "Succeeded to change baudrate!\n");
        else
        {
            Notify(msg_fatal_error, "Failed to change baudrate!\n");
            return;
        }
        // Ping all the servos to make sure they are all there.
        dxl_comm_result = packetHandlerRightArm->broadcastPing(portHandlerRightArm, vec);
        if (dxl_comm_result != COMM_SUCCESS)
            Notify(msg_warning, "%s\n", packetHandlerRightArm->getTxRxResult(dxl_comm_result));

        Notify(msg_debug, "Detected Dynamixel (Right arm) : \n");
        for (int i = 0; i < (int)vec.size(); i++)
            Notify(msg_debug, "[ID:%03d]\n", vec.at(i));

        // Body MX106R

        // Init Dynaxmixel SDK
        portHandlerBody = dynamixel::PortHandler::getPortHandler(robot[robotName].serialPortBody.c_str());
        packetHandlerBody = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION);

        // Open port
        if (portHandlerBody->openPort())
            Notify(msg_debug, "Succeeded to open serial port!\n");
        else
        {
            Notify(msg_fatal_error, "Failed to open serial port!\n");
            return;
        }
        // Set port baudrate
        if (portHandlerBody->setBaudRate(BAUDRATE1M))
            Notify(msg_debug, "Succeeded to change baudrate!\n");
        else
        {
            Notify(msg_fatal_error, "Failed to change baudrate!\n");
            return;
        }
        // Ping all the servos to make sure they are all there.
        dxl_comm_result = packetHandlerBody->broadcastPing(portHandlerBody, vec);
        if (dxl_comm_result != COMM_SUCCESS)
            Notify(msg_warning, "%s\n", packetHandlerBody->getTxRxResult(dxl_comm_result));

        Notify(msg_debug, "Detected Dynamixel (Body) : \n");
        for (int i = 0; i < (int)vec.size(); i++)
            Notify(msg_debug, "[ID:%03d]\n", vec.at(i));
    }
    else
    {
        Notify(msg_fatal_error, "Robot type is not yet implementet\n");
    }

    // Create dynamixel objects
    groupSyncWriteHead = new dynamixel::GroupSyncWrite(portHandlerHead, packetHandlerHead, 224, 4 + 2 + 1);
    groupSyncReadHead = new dynamixel::GroupSyncRead(portHandlerHead, packetHandlerHead, 634, 4 + 2 + 1);
    groupSyncWriteLeftArm = new dynamixel::GroupSyncWrite(portHandlerLeftArm, packetHandlerLeftArm, 224, 4 + 2);
    groupSyncReadLeftArm = new dynamixel::GroupSyncRead(portHandlerLeftArm, packetHandlerLeftArm, 634, 4 + 2 + 1);
    groupSyncWriteRightArm = new dynamixel::GroupSyncWrite(portHandlerRightArm, packetHandlerRightArm, 224, 4 + 2);
    groupSyncReadRightArm = new dynamixel::GroupSyncRead(portHandlerRightArm, packetHandlerRightArm, 634, 4 + 2 + 1);
    groupSyncWriteBody = new dynamixel::GroupSyncWrite(portHandlerBody, packetHandlerBody, 224, 4 + 2);
    groupSyncReadBody = new dynamixel::GroupSyncRead(portHandlerBody, packetHandlerBody, 634, 4 + 2 + 1);
    groupSyncWritePupil = new dynamixel::GroupSyncWrite(portHandlerPupil, packetHandlerPupil, 30, 2); // no read..

    if (!SetDefaultSettingServo())
        Notify(msg_fatal_error, "Unable to write default settings on servos\n");

    if (!PowerOnRobot())
        Notify(msg_fatal_error, "Unable torque up servos\n");
}

void EpiServos::Tick()
{
    if (simulate)
    {
        int index = 0;
        Notify(msg_debug, "Simulate servos");
        float maxVel = 45.0 / 1000 * 10; // No timebase
        if (GetTickLength() != 0)
            maxVel = 45.0 / 1000 * GetTickLength(); // Maximum change in one second in degrees / timebase

        for (int i = 0; i < EPI_NR_SERVOS; i++)
        {
            if (EpiTorsoMode && i > 5) // skip the last servos when running in Epi torso mode
                return;
            if (goalPosition)
                presentPosition[index] = presentPosition[index] + clip(goalPosition[index] - presentPosition[index], -maxVel, maxVel);
            presentCurrent[index] = 0; // mA
            index++;
        }
        return;
    }

    // Avg 32ms s1000 (No threads)
    auto headThread = std::async(std::launch::async, &EpiServos::CommunicationHead, this);   // 13 ms
    auto pupilThread = std::async(std::launch::async, &EpiServos::CommunicationPupil, this); //  25 ms Head and pupil = 25
    auto leftArmThread = std::async(std::launch::async, &EpiServos::CommunicationLeftArm, this);
    auto rightArmThread = std::async(std::launch::async, &EpiServos::CommunicationRightArm, this);
    auto bodyThread = std::async(std::launch::async, &EpiServos::CommunicationBody, this);

    if (!headThread.get())
        Notify(msg_fatal_error, "Can not communicate with head serial port");
    if (!pupilThread.get())
        Notify(msg_fatal_error, "Can not communicate with puil serial port");
    if (!leftArmThread.get())
        Notify(msg_fatal_error, "Can not communicate with head left arm serial port");
    if (!rightArmThread.get())
        Notify(msg_fatal_error, "Can not communicate with head right arm serial port");
    if (!bodyThread.get())
        Notify(msg_fatal_error, "Can not communicate with head bodyserial port");
}

// A function that set importat parameters in the control table.
// Baud rate and ID needs to be set manually.
bool EpiServos::SetDefaultSettingServo()
{

    uint32_t param_default_4Byte;
    uint16_t param_default_2Byte;
    uint8_t param_default_1Byte;

    uint8_t dxl_error = 0;              // Dynamixel error
    int dxl_comm_result = COMM_TX_FAIL; // Communication result

    Notify(msg_debug, "Setting control table on servos\n");

    // To be able to set some of the setting toruqe enable needs to be off.
    // Using write byte function instead of syncwrite. It may take some time.

    // Indirect adress (Torque enable) 1 bytes. Indiret mode not used for XL-320 (pupil)
    for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++)
        if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, IND_ADDR_TORQUE_ENABLE, ADDR_TORQUE_ENABLE, &dxl_error))
            return false;
    if (EpiMode)
    {
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerLeftArm, i, IND_ADDR_TORQUE_ENABLE, ADDR_TORQUE_ENABLE, &dxl_error))
                return false;
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerRightArm, i, IND_ADDR_TORQUE_ENABLE, ADDR_TORQUE_ENABLE, &dxl_error))
                return false;
        for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerBody, i, IND_ADDR_TORQUE_ENABLE, ADDR_TORQUE_ENABLE, &dxl_error))
                return false;
    }

    // Indirect adress (Goal position) 4 bytes. Indiret mode not used for XL-320 (pupil)
    for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++)
        for (int j = 0; j < 4; j++)
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, IND_ADDR_GOAL_POSITION + (2 * j), ADDR_GOAL_POSITION + j, &dxl_error))
                return false;

    if (EpiMode)
    {
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, i, IND_ADDR_GOAL_POSITION + (2 * j), ADDR_GOAL_POSITION + j, &dxl_error))
                    return false;
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, i, IND_ADDR_GOAL_POSITION + (2 * j), ADDR_GOAL_POSITION + j, &dxl_error))
                    return false;
        for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerBody->write2ByteTxRx(portHandlerBody, i, IND_ADDR_GOAL_POSITION + (2 * j), ADDR_GOAL_POSITION + j, &dxl_error))
                    return false;
    }

    // Indirect adress (Goal current). Indiret mode not used for XL-320 (pupil). MX28 does not support current mode.
    for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++)
        for (int j = 0; j < 2; j++)
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, IND_ADDR_GOAL_CURRENT, ADDR_GOAL_CURRENT, &dxl_error))
                return false;

    if (EpiMode)
    {
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, i, IND_ADDR_GOAL_CURRENT, ADDR_GOAL_CURRENT, &dxl_error))
                    return false;
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, i, IND_ADDR_GOAL_CURRENT, ADDR_GOAL_CURRENT, &dxl_error))
                    return false;
        for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerBody->write2ByteTxRx(portHandlerBody, i, IND_ADDR_GOAL_CURRENT, ADDR_GOAL_CURRENT, &dxl_error))
                    return false;
    }
    // Indirect adress (present position). Feedback
    for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++)
        for (int j = 0; j < 2; j++)
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, IND_ADDR_PRESENT_POSITION, ADDR_PRESENT_POSITION, &dxl_error))
                return false;
    if (EpiMode)
    {
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, i, IND_ADDR_PRESENT_POSITION, ADDR_PRESENT_POSITION, &dxl_error))
                    return false;
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, i, IND_ADDR_PRESENT_POSITION, ADDR_PRESENT_POSITION, &dxl_error))
                    return false;
        for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerBody->write2ByteTxRx(portHandlerBody, i, IND_ADDR_PRESENT_POSITION, ADDR_PRESENT_POSITION, &dxl_error))
                    return false;
    }
    // Indirect adress (present current). Feedback. MX28 does not support current mode.
    for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++)
        for (int j = 0; j < 2; j++)
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, IND_ADDR_PRESENT_CURRENT, ADDR_PRESENT_CURRENT, &dxl_error))
                return false;
    if (EpiMode)
    {
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, i, IND_ADDR_PRESENT_CURRENT, ADDR_PRESENT_CURRENT, &dxl_error))
                    return false;
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, i, IND_ADDR_PRESENT_CURRENT, ADDR_PRESENT_CURRENT, &dxl_error))
                    return false;
        for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerBody->write2ByteTxRx(portHandlerBody, i, IND_ADDR_PRESENT_CURRENT, ADDR_PRESENT_CURRENT, &dxl_error))
                    return false;
    }
    // Indirect adress (present temperature).
    for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++)
        for (int j = 0; j < 2; j++)
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, IND_ADDR_PRESENT_TEMPERATURE, ADDR_PRESENT_TEMPERATURE, &dxl_error))
                return false;
    if (EpiMode)
    {
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, i, IND_ADDR_PRESENT_TEMPERATURE, ADDR_PRESENT_TEMPERATURE, &dxl_error))
                    return false;
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, i, IND_ADDR_PRESENT_TEMPERATURE, ADDR_PRESENT_TEMPERATURE, &dxl_error))
                    return false;
        for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerBody->write2ByteTxRx(portHandlerBody, i, IND_ADDR_PRESENT_TEMPERATURE, ADDR_PRESENT_TEMPERATURE, &dxl_error))
                    return false;
    }

    // Profile accelration
    param_default_4Byte = 50;

    for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++)
        for (int j = 0; j < 2; j++)
            if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, i, ADDR_PROFILE_ACCELERATION, param_default_4Byte, &dxl_error))
                return false;
    if (EpiMode)
    {
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, i, ADDR_PROFILE_ACCELERATION, param_default_4Byte, &dxl_error))
                    return false;
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, i, ADDR_PROFILE_ACCELERATION, param_default_4Byte, &dxl_error))
                    return false;
        for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerBody->write4ByteTxRx(portHandlerBody, i, ADDR_PROFILE_ACCELERATION, param_default_4Byte, &dxl_error))
                    return false;
    }

    // Common settings for the seros
    // Profile velocity
    param_default_4Byte = 210;

    for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++)
        for (int j = 0; j < 2; j++)
            if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, i, ADDR_PROFILE_VELOCITY, param_default_4Byte, &dxl_error))
                return false;
    if (EpiMode)
    {
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, i, ADDR_PROFILE_VELOCITY, param_default_4Byte, &dxl_error))
                    return false;
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, i, ADDR_PROFILE_VELOCITY, param_default_4Byte, &dxl_error))
                    return false;
        for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerBody->write4ByteTxRx(portHandlerBody, i, ADDR_PROFILE_VELOCITY, param_default_4Byte, &dxl_error))
                    return false;
    }

    // P
    param_default_2Byte = 850;
    for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++)
        for (int j = 0; j < 2; j++)
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, ADDR_P, param_default_2Byte, &dxl_error))
                return false;
    if (EpiMode)
    {
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, i, ADDR_P, param_default_2Byte, &dxl_error))
                    return false;
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, i, ADDR_P, param_default_2Byte, &dxl_error))
                    return false;
        for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerBody->write2ByteTxRx(portHandlerBody, i, ADDR_P, param_default_2Byte, &dxl_error))
                    return false;
    }

    // I
    param_default_2Byte = 0;
    for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++)
        for (int j = 0; j < 2; j++)
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, ADDR_I, param_default_2Byte, &dxl_error))
                return false;

    if (EpiMode)
    {
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, i, ADDR_I, param_default_2Byte, &dxl_error))
                    return false;
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, i, ADDR_I, param_default_2Byte, &dxl_error))
                    return false;
        for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerBody->write2ByteTxRx(portHandlerBody, i, ADDR_I, param_default_2Byte, &dxl_error))
                    return false;
    }

    // D
    param_default_2Byte = 0;
    for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++)
        for (int j = 0; j < 2; j++)
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, ADDR_D, param_default_2Byte, &dxl_error))
                return false;
    if (EpiMode)
    {
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, i, ADDR_D, param_default_2Byte, &dxl_error))
                    return false;
        for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, i, ADDR_D, param_default_2Byte, &dxl_error))
                    return false;
        for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
            for (int j = 0; j < 4; j++)
                if (COMM_SUCCESS != packetHandlerBody->write2ByteTxRx(portHandlerBody, i, ADDR_D, param_default_2Byte, &dxl_error))
                    return false;
    }

    // Specific setting for each servos

    // HEAD ID 2
    // Limit position max
    param_default_4Byte = 2660;
    if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 2, 48, param_default_4Byte, &dxl_error))
        return false;
    // Limit position min
    param_default_4Byte = 1460;
    if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 2, 52, param_default_4Byte, &dxl_error))
        return false;

    // HEAD ID 3
    // Limit position max
    param_default_4Byte = 2660;
    if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 3, 48, param_default_4Byte, &dxl_error))
        return false;
    // Limit position min
    param_default_4Byte = 1500;
    if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 3, 52, param_default_4Byte, &dxl_error))
        return false;

    // HEAD ID 4 (Left eye)
    // Limit position max
    param_default_4Byte = 2300;
    if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 4, 48, param_default_4Byte, &dxl_error))
        return false;
    // Limit position min
    param_default_4Byte = 1700;
    if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 4, 52, param_default_4Byte, &dxl_error))
        return false;

    // HEAD ID 5 (Left eye)
    // Limit position max
    param_default_4Byte = 2300;
    if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 5, 48, param_default_4Byte, &dxl_error))
        return false;

    // Limit position min
    param_default_4Byte = 1700;
    if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 5, 52, param_default_4Byte, &dxl_error))
        return false;

    // PUPIL ID 2 (Left pupil)
    // Limit position max
    param_default_2Byte = 550;
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

    // PUPIL ID 3 (Right pupil)
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

    // LEFT ARM ID 2
    // LEFT ARM ID 3
    // LEFT ARM ID 4
    // LEFT ARM ID 5
    // LEFT ARM ID 6
    // LEFT ARM ID 7

    // RIGHT ARM ID 2
    // RIGHT ARM ID 3
    // RIGHT ARM ID 4
    // RIGHT ARM ID 5
    // RIGHT ARM ID 6
    // RIGHT ARM ID 7

    // BODY ID 2

    return true; // Yay we manage to set everything we needed.
}

bool EpiServos::PowerOn(int IDMin, int IDMax, dynamixel::PortHandler *portHandler, dynamixel::PacketHandler *packetHandler)
{
    Timer t;
    //int IDMin = HEAD_ID_MIN;
    const int nrOfServos = IDMax - IDMin + 1;
    int dxl_comm_result = COMM_TX_FAIL; // Communication result
    uint8_t dxl_error = 0;              // Dynamixel error
    uint16_t start_p_value[4] = {0, 0, 0, 0};
    uint32_t present_postition_value[4] = {0, 0, 0, 0};

    // Get P values
    for (int i = 0; i < nrOfServos; i++)
        if (COMM_SUCCESS != packetHandlerHead->read2ByteTxRx(portHandlerHead, IDMin + i, 84, &start_p_value[i], &dxl_error))
            return false;
    // Set P value to 0
    for (int i = 0; i < nrOfServos; i++)
        if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, IDMin + i, 84, 0, &dxl_error))
            return false;
    // Set torque value to 1
    for (int i = 0; i < nrOfServos; i++)
        if (COMM_SUCCESS != packetHandlerHead->write1ByteTxRx(portHandlerHead, IDMin + i, 64, 1, &dxl_error))
            return false;
    Notify(msg_debug, "Power up servos (Head)");
    while (t.GetTime() < TIMER_POWER_ON)
    {
        // Get present position
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandlerHead->read4ByteTxRx(portHandlerHead, IDMin + i, 132, &present_postition_value[i], &dxl_error))
                return false;
        // Set goal position to present postiion
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, IDMin + i, 116, present_postition_value[i], &dxl_error))
                return false;
        // Ramping down P
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, IDMin + i, 84, int(float(start_p_value[i]) / float(TIMER_POWER_ON) * t.GetTime()), &dxl_error))
                return false;
    }
    // Set P value to start value
    for (int i = 0; i < nrOfServos; i++)
        if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, IDMin + i, 84, start_p_value[i], &dxl_error))
            return false;

    return true;
}

bool EpiServos::PowerOnHead()
{
    Timer t;
    int IDMin = HEAD_ID_MIN;
    const int nrOfServos = HEAD_ID_MAX - HEAD_ID_MIN + 1;
    int dxl_comm_result = COMM_TX_FAIL; // Communication result
    uint8_t dxl_error = 0;              // Dynamixel error
    uint16_t start_p_value[nrOfServos] = {0, 0, 0, 0};
    uint32_t present_postition_value[nrOfServos] = {0, 0, 0, 0};

    // Get P values
    for (int i = 0; i < nrOfServos; i++)
        if (COMM_SUCCESS != packetHandlerHead->read2ByteTxRx(portHandlerHead, IDMin + i, 84, &start_p_value[i], &dxl_error))
            return false;
    // Set P value to 0
    for (int i = 0; i < nrOfServos; i++)
        if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, IDMin + i, 84, 0, &dxl_error))
            return false;
    // Set torque value to 1
    for (int i = 0; i < nrOfServos; i++)
        if (COMM_SUCCESS != packetHandlerHead->write1ByteTxRx(portHandlerHead, IDMin + i, 64, 1, &dxl_error))
            return false;
    Notify(msg_debug, "Power up servos (Head)");
    while (t.GetTime() < TIMER_POWER_ON)
    {
        // Get present position
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandlerHead->read4ByteTxRx(portHandlerHead, IDMin + i, 132, &present_postition_value[i], &dxl_error))
                return false;
        // Set goal position to present postiion
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, IDMin + i, 116, present_postition_value[i], &dxl_error))
                return false;
        // Ramping down P
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, IDMin + i, 84, int(float(start_p_value[i]) / float(TIMER_POWER_ON) * t.GetTime()), &dxl_error))
                return false;
    }
    // Set P value to start value
    for (int i = 0; i < nrOfServos; i++)
        if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, IDMin + i, 84, start_p_value[i], &dxl_error))
            return false;

    return true;
}

bool EpiServos::PowerOnPupil()
{
    Timer t;
    int dxl_comm_result = COMM_TX_FAIL;
    uint8_t dxl_error = 0;
    uint8_t start_p_value2 = 0;
    uint8_t start_p_value3 = 0;
    // Get P values
    if (COMM_SUCCESS != packetHandlerPupil->read1ByteTxRx(portHandlerPupil, 2, 29, &start_p_value2, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerPupil->read1ByteTxRx(portHandlerPupil, 3, 29, &start_p_value3, &dxl_error))
        return false;
    // Set P value to 0
    if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 29, 0, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 29, 0, &dxl_error))
        return false;
    Notify(msg_debug, "Enable torque on servos (Pupil)");
    // Enable torque
    if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 24, 1, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 24, 1, &dxl_error))
        return false;
    Notify(msg_debug, "Power up servos (Pupil)");
    while (t.GetTime() < TIMER_POWER_ON)
    {
        // Get present position
        uint16_t present_postition_value2 = 0;
        uint16_t present_postition_value3 = 0;
        if (COMM_SUCCESS != packetHandlerPupil->read2ByteTxRx(portHandlerPupil, 2, 37, &present_postition_value2, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->read2ByteTxRx(portHandlerPupil, 3, 37, &present_postition_value3, &dxl_error))
            return false;
        // Set goal position to present postiion
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 30, present_postition_value2, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 30, present_postition_value3, &dxl_error))
            return false;
        // Ramping down P
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 29, int(float(start_p_value2) / float(TIMER_POWER_ON) * t.GetTime()), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 29, int(float(start_p_value3) / float(TIMER_POWER_ON) * t.GetTime()), &dxl_error))
            return false;
    }
    // Set P value to start value
    if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 29, start_p_value2, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 29, start_p_value3, &dxl_error))
        return false;

    return true;
}
bool EpiServos::PowerOnLeftArm()
{
    if (!EpiMode)
        return true;

    Timer t;
    int dxl_comm_result = COMM_TX_FAIL; // Communication result
    uint8_t dxl_error = 0;              // Dynamixel error
    uint16_t start_p_value2 = 0;
    uint16_t start_p_value3 = 0;
    uint16_t start_p_value4 = 0;
    uint16_t start_p_value5 = 0;
    uint16_t start_p_value6 = 0;
    uint16_t start_p_value7 = 0;

    // Get P values
    if (COMM_SUCCESS != packetHandlerLeftArm->read2ByteTxRx(portHandlerLeftArm, 2, 84, &start_p_value2, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->read2ByteTxRx(portHandlerLeftArm, 3, 84, &start_p_value3, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->read2ByteTxRx(portHandlerLeftArm, 4, 84, &start_p_value4, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->read2ByteTxRx(portHandlerLeftArm, 5, 84, &start_p_value5, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->read2ByteTxRx(portHandlerLeftArm, 6, 84, &start_p_value6, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->read2ByteTxRx(portHandlerLeftArm, 7, 84, &start_p_value7, &dxl_error))
        return false;
    // Set P value to 0
    for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
        if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, i, 84, 0, &dxl_error))
            return false;
    Notify(msg_debug, "Enable torque (Arm)");
    for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
        if (COMM_SUCCESS != packetHandlerLeftArm->write1ByteTxRx(portHandlerLeftArm, i, 64, 1, &dxl_error))
            return false;
    Notify(msg_debug, "Power up servos (Arm)");
    while (t.GetTime() < TIMER_POWER_ON)
    {
        // Get present position
        uint32_t present_postition_value2 = 0;
        uint32_t present_postition_value3 = 0;
        uint32_t present_postition_value4 = 0;
        uint32_t present_postition_value5 = 0;
        uint32_t present_postition_value6 = 0;
        uint32_t present_postition_value7 = 0;

        if (COMM_SUCCESS != packetHandlerLeftArm->read4ByteTxRx(portHandlerLeftArm, 2, 132, &present_postition_value2, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerLeftArm->read4ByteTxRx(portHandlerLeftArm, 3, 132, &present_postition_value3, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerLeftArm->read4ByteTxRx(portHandlerLeftArm, 4, 132, &present_postition_value4, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerLeftArm->read4ByteTxRx(portHandlerLeftArm, 5, 132, &present_postition_value5, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerLeftArm->read4ByteTxRx(portHandlerLeftArm, 6, 132, &present_postition_value6, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerLeftArm->read4ByteTxRx(portHandlerLeftArm, 7, 132, &present_postition_value7, &dxl_error))
            return false;
        // Set goal position to present postiion
        if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 2, 116, present_postition_value2, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 3, 116, present_postition_value3, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 4, 116, present_postition_value4, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 5, 116, present_postition_value5, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 6, 116, present_postition_value6, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 7, 116, present_postition_value7, &dxl_error))
            return false;
        // Ramping down P
        if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 2, 84, int(float(start_p_value2) / float(TIMER_POWER_ON) * t.GetTime()), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 3, 84, int(float(start_p_value3) / float(TIMER_POWER_ON) * t.GetTime()), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 4, 84, int(float(start_p_value4) / float(TIMER_POWER_ON) * t.GetTime()), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 5, 84, int(float(start_p_value5) / float(TIMER_POWER_ON) * t.GetTime()), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 6, 84, int(float(start_p_value6) / float(TIMER_POWER_ON) * t.GetTime()), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 7, 84, int(float(start_p_value7) / float(TIMER_POWER_ON) * t.GetTime()), &dxl_error))
            return false;
    }
    // Set P value to start value
    if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 2, 84, start_p_value2, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 3, 84, start_p_value3, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 4, 84, start_p_value4, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 5, 84, start_p_value5, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 6, 84, start_p_value6, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 7, 84, start_p_value7, &dxl_error))
        return false;

    return true;
}
bool EpiServos::PowerOnRightArm()
{
    if (!EpiMode)
        return true;

    Timer t;
    int dxl_comm_result = COMM_TX_FAIL; // Communication result
    uint8_t dxl_error = 0;              // Dynamixel error
    uint16_t start_p_value2 = 0;
    uint16_t start_p_value3 = 0;
    uint16_t start_p_value4 = 0;
    uint16_t start_p_value5 = 0;
    uint16_t start_p_value6 = 0;
    uint16_t start_p_value7 = 0;

    // Get P values
    if (COMM_SUCCESS != packetHandlerRightArm->read2ByteTxRx(portHandlerRightArm, 2, 84, &start_p_value2, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->read2ByteTxRx(portHandlerRightArm, 3, 84, &start_p_value3, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->read2ByteTxRx(portHandlerRightArm, 4, 84, &start_p_value4, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->read2ByteTxRx(portHandlerRightArm, 5, 84, &start_p_value5, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->read2ByteTxRx(portHandlerRightArm, 6, 84, &start_p_value6, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->read2ByteTxRx(portHandlerRightArm, 7, 84, &start_p_value7, &dxl_error))
        return false;

    // Set P value to 0
    for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
        if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, i, 84, 0, &dxl_error))
            return false;
    Notify(msg_debug, "Enable torque (Arm)");
    for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
        if (COMM_SUCCESS != packetHandlerRightArm->write1ByteTxRx(portHandlerRightArm, i, 64, 1, &dxl_error))
            return false;

    Notify(msg_debug, "Power up servos (Arm)");
    while (t.GetTime() < TIMER_POWER_ON)
    {
        // Get present position
        uint32_t present_postition_value2 = 0;
        uint32_t present_postition_value3 = 0;
        uint32_t present_postition_value4 = 0;
        uint32_t present_postition_value5 = 0;
        uint32_t present_postition_value6 = 0;
        uint32_t present_postition_value7 = 0;

        if (COMM_SUCCESS != packetHandlerRightArm->read4ByteTxRx(portHandlerRightArm, 2, 132, &present_postition_value2, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerRightArm->read4ByteTxRx(portHandlerRightArm, 3, 132, &present_postition_value3, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerRightArm->read4ByteTxRx(portHandlerRightArm, 4, 132, &present_postition_value4, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerRightArm->read4ByteTxRx(portHandlerRightArm, 5, 132, &present_postition_value5, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerRightArm->read4ByteTxRx(portHandlerRightArm, 6, 132, &present_postition_value6, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerRightArm->read4ByteTxRx(portHandlerRightArm, 7, 132, &present_postition_value7, &dxl_error))
            return false;
        // Set goal position to present postiion
        if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 2, 116, present_postition_value2, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 3, 116, present_postition_value3, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 4, 116, present_postition_value4, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 5, 116, present_postition_value5, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 6, 116, present_postition_value6, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 7, 116, present_postition_value7, &dxl_error))
            return false;
        // Ramping down P
        if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 2, 84, int(float(start_p_value2) / float(TIMER_POWER_ON) * t.GetTime()), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 3, 84, int(float(start_p_value3) / float(TIMER_POWER_ON) * t.GetTime()), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 4, 84, int(float(start_p_value4) / float(TIMER_POWER_ON) * t.GetTime()), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 5, 84, int(float(start_p_value5) / float(TIMER_POWER_ON) * t.GetTime()), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 6, 84, int(float(start_p_value6) / float(TIMER_POWER_ON) * t.GetTime()), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 7, 84, int(float(start_p_value7) / float(TIMER_POWER_ON) * t.GetTime()), &dxl_error))
            return false;
    }
    // Set P value to start value
    if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 2, 84, start_p_value2, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 3, 84, start_p_value3, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 4, 84, start_p_value4, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 5, 84, start_p_value5, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 6, 84, start_p_value6, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 7, 84, start_p_value7, &dxl_error))
        return false;

    return true;
}
bool EpiServos::PowerOnBody()
{
    if (!EpiMode)
        return true;

    Timer t;
    int dxl_comm_result = COMM_TX_FAIL;
    uint8_t dxl_error = 0;
    uint16_t start_p_value2 = 0;

    // Get P values
    if (COMM_SUCCESS != packetHandlerRightArm->read2ByteTxRx(portHandlerRightArm, 2, 84, &start_p_value2, &dxl_error))
        return false;
    // Set P value to 0
    if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 2, 84, 0, &dxl_error))
        return false;
    Notify(msg_debug, "Enable torque (Arm)");
    if (COMM_SUCCESS != packetHandlerRightArm->write1ByteTxRx(portHandlerRightArm, 2, 64, 1, &dxl_error))
        return false;
    Notify(msg_debug, "Power up servos (Arm)");
    while (t.GetTime() < TIMER_POWER_ON)
    {
        // Get present position
        uint32_t present_postition_value2 = 0;
        if (COMM_SUCCESS != packetHandlerRightArm->read4ByteTxRx(portHandlerRightArm, 2, 132, &present_postition_value2, &dxl_error))
            return false;
        // Set goal position to present postiion
        if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 2, 116, present_postition_value2, &dxl_error))
            return false;
        // Ramping down P
        if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 2, 84, int(float(start_p_value2) / float(TIMER_POWER_ON) * t.GetTime()), &dxl_error))
            return false;
    }
    // Set P value to start value
    if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 2, 84, start_p_value2, &dxl_error))
        return false;

    return true;
}

bool EpiServos::PowerOnRobot()
{
    auto headThread = std::async(std::launch::async, &EpiServos::PowerOn, this, 2, 4, std::ref(portHandlerHead), std::ref(packetHandlerHead));
    auto pupilThread = std::async(std::launch::async, &EpiServos::PowerOn, this, 2, 3, std::ref(portHandlerPupil), std::ref(packetHandlerPupil));
    auto leftArmThread = std::async(std::launch::async, &EpiServos::PowerOn, this, 2, 7, std::ref(portHandlerLeftArm), std::ref(packetHandlerLeftArm));
    auto rightArmThread = std::async(std::launch::async, &EpiServos::PowerOn, this, 2, 7, std::ref(portHandlerRightArm), std::ref(packetHandlerRightArm));
    auto bodyThread = std::async(std::launch::async, &EpiServos::PowerOn, this, 2, 2, std::ref(portHandlerBody), std::ref(packetHandlerBody));

    //auto headThread = std::async(std::launch::async, &EpiServos::PowerOnHead, this);
    //auto pupilThread = std::async(std::launch::async, &EpiServos::PowerOnPupil, this);
    //auto leftArmThread = std::async(std::launch::async, &EpiServos::PowerOnLeftArm, this);
    //auto rightArmThread = std::async(std::launch::async, &EpiServos::PowerOnRightArm, this);
   // auto bodyThread = std::async(std::launch::async, &EpiServos::PowerOnBody, this);

    if (!headThread.get())
        Notify(msg_fatal_error, "Can not communicate with head serial port");
    if (!pupilThread.get())
        Notify(msg_fatal_error, "Can not communicate with puil serial port");
    if (!leftArmThread.get())
        Notify(msg_fatal_error, "Can not communicate with head left arm serial port");
    if (!rightArmThread.get())
        Notify(msg_fatal_error, "Can not communicate with head right arm serial port");
    if (!bodyThread.get())
        Notify(msg_fatal_error, "Can not communicate with head bodyserial port");

    // Trying to torque up the power of the servos.
    // Dynamixel protocel 2.0
    // In current base position control mode goal current can be used.
    // In poistion control mode P can be used (PID).
    // Torqing up the servos? This can not be done in 2.0 and position mode only in position-current mode.
    // 1. Set P (PID) = 0. Store start P value
    // 2. Set goal poistion to present position
    // 3. Increase current or P (PID)
    // 4. Repeat 2,3 for 2 seconds.

    return true;
}

bool EpiServos::PowerOffHead()
{
    Timer t;
    int IDMin = HEAD_ID_MIN;
    const int nrOfServos = HEAD_ID_MAX - HEAD_ID_MIN + 1;
    int dxl_comm_result = COMM_TX_FAIL; // Communication result
    uint8_t dxl_error = 0;              // Dynamixel error
    uint16_t start_p_value[nrOfServos] = {0, 0, 0, 0};
    uint32_t present_postition_value[nrOfServos] = {0, 0, 0, 0};

    // Get P values
    for (int i = 0; i < nrOfServos; i++)
        if (COMM_SUCCESS != packetHandlerHead->read2ByteTxRx(portHandlerHead, IDMin + i, 84, &start_p_value[i], &dxl_error))
            return false;
    t.Reset();
    Notify(msg_warning, "Power off servos. If needed, support the robot while power off the servos");

    // Ramp down p value
    while (t.GetTime() < TIMER_POWER_OFF)
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, IDMin + i, 84, int(float(start_p_value[i]) * (float(TIMER_POWER_OFF) - t.GetTime()) / float(TIMER_POWER_OFF)), &dxl_error))
                return false;
    // Get present position
    for (int i = 0; i < nrOfServos; i++)
        if (COMM_SUCCESS != packetHandlerHead->read4ByteTxRx(portHandlerHead, IDMin + i, 132, &present_postition_value[i], &dxl_error))
            return false;
    // Set goal position to present postiion
    for (int i = 0; i < nrOfServos; i++)
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, IDMin + i, 116, present_postition_value[i], &dxl_error))
            return false;

    t.Restart();
    t.Sleep(TIMER_POWER_OFF_EXTENDED);

    // Enable torque off
    Notify(msg_debug, "Enable torque (Arm)");
    for (int i = 0; i < nrOfServos; i++)
        if (COMM_SUCCESS != packetHandlerRightArm->write1ByteTxRx(portHandlerHead, IDMin + i, 64, 1, &dxl_error))
            return false;
    // Set P value to start value
    for (int i = 0; i < nrOfServos; i++)
        if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, IDMin + i, 84, start_p_value[i], &dxl_error))
            return false;

    return true;
}
bool EpiServos::PowerOffPupil()
{

    Timer t;
    int dxl_comm_result = COMM_TX_FAIL; // Communication result
    uint8_t dxl_error = 0;              // Dynamixel error
    uint8_t start_p_value2 = 0;
    uint8_t start_p_value3 = 0;
    // Get P values
    if (COMM_SUCCESS != packetHandlerPupil->read1ByteTxRx(portHandlerPupil, 2, 29, &start_p_value2, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerPupil->read1ByteTxRx(portHandlerPupil, 3, 29, &start_p_value3, &dxl_error))
        return false;
    t.Reset();
    Notify(msg_warning, "Power off servos. If needed, support the robot while power off the servos");
    while (t.GetTime() < TIMER_POWER_OFF)
    {
        // Set P value
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 29, int(float(start_p_value2) * (float(TIMER_POWER_OFF) - t.GetTime()) / float(TIMER_POWER_OFF)), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 29, int(float(start_p_value3) * (float(TIMER_POWER_OFF) - t.GetTime()) / float(TIMER_POWER_OFF)), &dxl_error))
            return false;
    }
    // Get present position
    uint16_t present_postition_value2 = 0;
    uint16_t present_postition_value3 = 0;
    if (COMM_SUCCESS != packetHandlerPupil->read2ByteTxRx(portHandlerPupil, 2, 37, &present_postition_value2, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerPupil->read2ByteTxRx(portHandlerPupil, 3, 37, &present_postition_value3, &dxl_error))
        return false;
    // Set goal position to present postiion
    if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 30, present_postition_value2, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 30, present_postition_value3, &dxl_error))
        return false;
    t.Restart();
    t.Sleep(TIMER_POWER_OFF_EXTENDED);
    // Torque enable off
    if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 24, 0, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 24, 0, &dxl_error))
        return false;
    // Set P value to start value
    if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 29, start_p_value2, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 29, start_p_value3, &dxl_error))
        return false;

    return true;
}
bool EpiServos::PowerOffLeftArm()
{
    if (!EpiMode)
        return true;

    Timer t;
    int dxl_comm_result = COMM_TX_FAIL; // Communication result
    uint8_t dxl_error = 0;              // Dynamixel error
    uint16_t start_p_value2 = 0;
    uint16_t start_p_value3 = 0;
    uint16_t start_p_value4 = 0;
    uint16_t start_p_value5 = 0;
    uint16_t start_p_value6 = 0;
    uint16_t start_p_value7 = 0;
    // Get P values
    if (COMM_SUCCESS != packetHandlerLeftArm->read2ByteTxRx(portHandlerLeftArm, 2, 84, &start_p_value2, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->read2ByteTxRx(portHandlerLeftArm, 3, 84, &start_p_value3, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->read2ByteTxRx(portHandlerLeftArm, 4, 84, &start_p_value4, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->read2ByteTxRx(portHandlerLeftArm, 5, 84, &start_p_value5, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->read2ByteTxRx(portHandlerLeftArm, 6, 84, &start_p_value6, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->read2ByteTxRx(portHandlerLeftArm, 7, 84, &start_p_value7, &dxl_error))
        return false;
    t.Reset();

    Notify(msg_warning, "Power off servos. If needed, support the robot while power off the servos");

    while (t.GetTime() < TIMER_POWER_OFF)
    {
        // Set P value
        if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 2, 84, int(float(start_p_value2) * (float(TIMER_POWER_OFF) - t.GetTime()) / float(TIMER_POWER_OFF)), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 3, 84, int(float(start_p_value3) * (float(TIMER_POWER_OFF) - t.GetTime()) / float(TIMER_POWER_OFF)), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 4, 84, int(float(start_p_value4) * (float(TIMER_POWER_OFF) - t.GetTime()) / float(TIMER_POWER_OFF)), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 5, 84, int(float(start_p_value5) * (float(TIMER_POWER_OFF) - t.GetTime()) / float(TIMER_POWER_OFF)), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 6, 84, int(float(start_p_value6) * (float(TIMER_POWER_OFF) - t.GetTime()) / float(TIMER_POWER_OFF)), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 7, 84, int(float(start_p_value7) * (float(TIMER_POWER_OFF) - t.GetTime()) / float(TIMER_POWER_OFF)), &dxl_error))
            return false;
    }

    // Get present position
    uint32_t present_postition_value2 = 0;
    uint32_t present_postition_value3 = 0;
    uint32_t present_postition_value4 = 0;
    uint32_t present_postition_value5 = 0;
    uint32_t present_postition_value6 = 0;
    uint32_t present_postition_value7 = 0;
    if (COMM_SUCCESS != packetHandlerLeftArm->read4ByteTxRx(portHandlerLeftArm, 2, 132, &present_postition_value2, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->read4ByteTxRx(portHandlerLeftArm, 3, 132, &present_postition_value3, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->read4ByteTxRx(portHandlerLeftArm, 4, 132, &present_postition_value4, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->read4ByteTxRx(portHandlerLeftArm, 5, 132, &present_postition_value5, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->read4ByteTxRx(portHandlerLeftArm, 6, 132, &present_postition_value6, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->read4ByteTxRx(portHandlerLeftArm, 7, 132, &present_postition_value7, &dxl_error))
        return false;
    // Set goal position to present postiion
    if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 2, 116, present_postition_value2, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 3, 116, present_postition_value3, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 4, 116, present_postition_value4, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 5, 116, present_postition_value5, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 6, 116, present_postition_value6, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 7, 116, present_postition_value7, &dxl_error))
        return false;
    t.Restart();
    t.Sleep(TIMER_POWER_OFF_EXTENDED);
    // Enable torque off
    Notify(msg_debug, "Enable torque (Arm)");
    for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
        if (COMM_SUCCESS != packetHandlerLeftArm->write1ByteTxRx(portHandlerLeftArm, i, 64, 1, &dxl_error))
            return false;

    // Set P value to start value
    if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 2, 84, start_p_value2, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 3, 84, start_p_value3, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 4, 84, start_p_value4, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 5, 84, start_p_value5, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 6, 84, start_p_value6, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, 7, 84, start_p_value7, &dxl_error))
        return false;
    return true;
}
bool EpiServos::PowerOffRightArm()
{
    if (!EpiMode)
        return true;

    Timer t;
    int dxl_comm_result = COMM_TX_FAIL; // Communication result
    uint8_t dxl_error = 0;              // Dynamixel error
    uint16_t start_p_value2 = 0;
    uint16_t start_p_value3 = 0;
    uint16_t start_p_value4 = 0;
    uint16_t start_p_value5 = 0;
    uint16_t start_p_value6 = 0;
    uint16_t start_p_value7 = 0;

    // Get P values
    if (COMM_SUCCESS != packetHandlerRightArm->read2ByteTxRx(portHandlerRightArm, 2, 84, &start_p_value2, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->read2ByteTxRx(portHandlerRightArm, 3, 84, &start_p_value3, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->read2ByteTxRx(portHandlerRightArm, 4, 84, &start_p_value4, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->read2ByteTxRx(portHandlerRightArm, 5, 84, &start_p_value5, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->read2ByteTxRx(portHandlerRightArm, 6, 84, &start_p_value6, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->read2ByteTxRx(portHandlerRightArm, 7, 84, &start_p_value7, &dxl_error))
        return false;
    t.Reset();

    Notify(msg_warning, "Power off servos. If needed, support the robot while power off the servos");

    while (t.GetTime() < TIMER_POWER_OFF)
    {
        // Set P value
        if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 2, 84, int(float(start_p_value2) * (float(TIMER_POWER_OFF) - t.GetTime()) / float(TIMER_POWER_OFF)), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 3, 84, int(float(start_p_value3) * (float(TIMER_POWER_OFF) - t.GetTime()) / float(TIMER_POWER_OFF)), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 4, 84, int(float(start_p_value4) * (float(TIMER_POWER_OFF) - t.GetTime()) / float(TIMER_POWER_OFF)), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 5, 84, int(float(start_p_value5) * (float(TIMER_POWER_OFF) - t.GetTime()) / float(TIMER_POWER_OFF)), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 6, 84, int(float(start_p_value6) * (float(TIMER_POWER_OFF) - t.GetTime()) / float(TIMER_POWER_OFF)), &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 7, 84, int(float(start_p_value7) * (float(TIMER_POWER_OFF) - t.GetTime()) / float(TIMER_POWER_OFF)), &dxl_error))
            return false;
    }

    // Get present position
    uint32_t present_postition_value2 = 0;
    uint32_t present_postition_value3 = 0;
    uint32_t present_postition_value4 = 0;
    uint32_t present_postition_value5 = 0;
    uint32_t present_postition_value6 = 0;
    uint32_t present_postition_value7 = 0;
    if (COMM_SUCCESS != packetHandlerRightArm->read4ByteTxRx(portHandlerRightArm, 2, 132, &present_postition_value2, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->read4ByteTxRx(portHandlerRightArm, 3, 132, &present_postition_value3, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->read4ByteTxRx(portHandlerRightArm, 4, 132, &present_postition_value4, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->read4ByteTxRx(portHandlerRightArm, 5, 132, &present_postition_value5, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->read4ByteTxRx(portHandlerRightArm, 6, 132, &present_postition_value6, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->read4ByteTxRx(portHandlerRightArm, 7, 132, &present_postition_value7, &dxl_error))
        return false;
    // Set goal position to present postiion
    if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 2, 116, present_postition_value2, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 3, 116, present_postition_value3, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 4, 116, present_postition_value4, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 5, 116, present_postition_value5, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 6, 116, present_postition_value6, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 7, 116, present_postition_value7, &dxl_error))
        return false;
    t.Restart();
    t.Sleep(TIMER_POWER_OFF_EXTENDED);
    // Enable torque off
    Notify(msg_debug, "Enable torque (Arm)");
    for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
        if (COMM_SUCCESS != packetHandlerRightArm->write1ByteTxRx(portHandlerRightArm, i, 64, 1, &dxl_error))
            return false;
    // Set P value to start value
    if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 2, 84, start_p_value2, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 3, 84, start_p_value3, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 4, 84, start_p_value4, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 5, 84, start_p_value5, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 6, 84, start_p_value6, &dxl_error))
        return false;
    if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, 7, 84, start_p_value7, &dxl_error))
        return false;
    return true;
}
bool EpiServos::PowerOffBody()
{
    if (!EpiMode)
        return true;

    Timer t;

    int dxl_comm_result = COMM_TX_FAIL; // Communication result
    uint8_t dxl_error = 0;              // Dynamixel error

    // 1.
    uint16_t start_p_value2 = 0;

    // Get P values
    if (COMM_SUCCESS != packetHandlerRightArm->read2ByteTxRx(portHandlerBody, 2, 84, &start_p_value2, &dxl_error))
        return false;
    // Set P value to 0
    if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerBody, 2, 84, 0, &dxl_error))
        return false;
    Notify(msg_debug, "Enable torque (Arm)");
    if (COMM_SUCCESS != packetHandlerRightArm->write1ByteTxRx(portHandlerBody, 2, 64, 1, &dxl_error))
        return false;
    Notify(msg_debug, "Power up servos (Arm)");
    while (t.GetTime() < TIMER_POWER_ON)
    {
        // Get present position
        uint32_t present_postition_value2 = 0;
        if (COMM_SUCCESS != packetHandlerRightArm->read4ByteTxRx(portHandlerBody, 2, 132, &present_postition_value2, &dxl_error))
            return false;
        // Set goal position to present postiion
        if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerBody, 2, 116, present_postition_value2, &dxl_error))
            return false;
        // Ramping down P
        if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerBody, 2, 84, int(float(start_p_value2) / float(TIMER_POWER_ON) * t.GetTime()), &dxl_error))
            return false;
    }
    // Set P value to start value
    if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerBody, 2, 84, start_p_value2, &dxl_error))
        return false;

    return true;
}

bool EpiServos::PowerOffRobot()
{
    // We need to take care of return value...
    auto headThread = std::async(std::launch::async, &EpiServos::PowerOffHead, this);
    auto pupilThread = std::async(std::launch::async, &EpiServos::PowerOffPupil, this);
    auto leftArmThread = std::async(std::launch::async, &EpiServos::PowerOffLeftArm, this);
    auto rightArmThread = std::async(std::launch::async, &EpiServos::PowerOffRightArm, this);
    auto bodyThread = std::async(std::launch::async, &EpiServos::PowerOffBody, this);

    if (!headThread.get())
        Notify(msg_fatal_error, "Can not communicate with head serial port");
    if (!pupilThread.get())
        Notify(msg_fatal_error, "Can not communicate with pupil serial port");
    if (!leftArmThread.get())
        Notify(msg_fatal_error, "Can not communicate with head left arm serial port");
    if (!rightArmThread.get())
        Notify(msg_fatal_error, "Can not communicate with head right arm serial port");
    if (!bodyThread.get())
        Notify(msg_fatal_error, "Can not communicate with head bodyserial port");

    // Power down servos.
    // 1. Store P (PID) value
    // 2. Ramp down P
    // 3. Turn of torque enable
    // 4. Set P (PID) valued from 1.

    return (true);
}

EpiServos::~EpiServos()
{
    if (simulate) // no memory to return
    {
        return;
    }

    // Torque down
    PowerOffRobot();

    // Close ports
    if (EpiTorsoMode || EpiMode)
    {
        portHandlerHead->closePort();
        portHandlerPupil->closePort();
    }
    if (EpiMode)
    {
        portHandlerLeftArm->closePort();
        portHandlerRightArm->closePort();
        portHandlerBody->closePort();
    }

    // Free memory
    delete groupSyncWriteHead;
    delete groupSyncReadHead;
    delete groupSyncWriteLeftArm;
    delete groupSyncReadLeftArm;
    delete groupSyncWriteRightArm;
    delete groupSyncReadRightArm;
    delete groupSyncWriteBody;
    delete groupSyncReadBody;
    delete groupSyncWritePupil;
}
static InitClass init("EpiServos", &EpiServos::Create, "Source/Modules/RobotModules/EpiServos/");
