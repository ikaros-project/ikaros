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
#define ADDR_PRESENT_CURRENT 126
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

#define TIMER_POWER_ON 2           // Timer ramping up sec
#define TIMER_POWER_OFF 5          // Timer ramping down sec
#define TIMER_POWER_OFF_EXTENDED 3 // Timer until torque enable off sec

#define HEAD_INDEX_IO 0
#define PUPIL_INDEX_IO 4
#define LEFT_ARM_INDEX_IO 6
#define RIGHT_ARM_INDEX_IO 12
#define BODY_INDEX_IO 18

#define MAX_TEMPERATURE 65

// TODO:
// Add fast sync write feature


#include <stdio.h>
#include <vector> // Data from dynamixel sdk
#include <future> // Threads

#include "ikaros.h"

// This must be after ikaros.h
#include "dynamixel_sdk.h" // Uses Dynamixel SDK library

using namespace ikaros;
typedef struct
{
    std::string serialPortPupil;
    std::string serialPortHead;
    std::string serialPortBody;
    std::string serialPortLeftArm;
    std::string serialPortRightArm;
    std::string type;

} Robot_parameters;

class EpiServos : public Module
{
    // Paramteters
    int robotType = 0;
    parameter simulate;

    // Ikaros IO
    matrix goalPosition;
    matrix goalCurrent;
    bool torqueEnable = true;

  

    matrix presentPosition;
    matrix presentCurrent;
   

    bool EpiTorsoMode = false;
    bool EpiMode = false;

    int AngleMinLimitPupil[2];
    int AngleMaxLimitPupil[2]; 

    dynamixel::PortHandler *portHandlerHead;
    dynamixel::PacketHandler *packetHandlerHead;
    dynamixel::GroupSyncRead *groupSyncReadHead;
    dynamixel::GroupSyncWrite *groupSyncWriteHead;

    dynamixel::PortHandler *portHandlerPupil;
    dynamixel::PacketHandler *packetHandlerPupil;
    dynamixel::GroupSyncRead *groupSyncReadPupil;
    dynamixel::GroupSyncWrite *groupSyncWritePupil;

    dynamixel::PortHandler *portHandlerLeftArm;
    dynamixel::PacketHandler *packetHandlerLeftArm;
    dynamixel::GroupSyncRead *groupSyncReadLeftArm;
    dynamixel::GroupSyncWrite *groupSyncWriteLeftArm;

    dynamixel::PortHandler *portHandlerRightArm;
    dynamixel::PacketHandler *packetHandlerRightArm;
    dynamixel::GroupSyncRead *groupSyncReadRightArm;
    dynamixel::GroupSyncWrite *groupSyncWriteRightArm;

    dynamixel::PortHandler *portHandlerBody;
    dynamixel::PacketHandler *packetHandlerBody;
    dynamixel::GroupSyncRead *groupSyncReadBody;
    dynamixel::GroupSyncWrite *groupSyncWriteBody;

    std::string robotName;
    std::map<std::string, Robot_parameters> robot;

    

    bool CommunicationPupil()
    {
        // Change this function.
        // No need to have torque enable.
        // Only goal position and use sync write.

        int index = 0;
        int dxl_comm_result = COMM_TX_FAIL; // Communication result
        bool dxl_addparam_result = false;   // addParam result
        bool dxl_getdata_result = false;    // GetParam result
        uint8_t dxl_error = 0;              // Dynamixel error

        // Send to pupil. No feedback

        index = PUPIL_INDEX_IO;

        for (int i = PUPIL_ID_MIN; i <= PUPIL_ID_MAX; i++)
        {
            /*if (!torqueEnable.empty())
            {
                uint8_t param_default = torqueEnable[index];
                if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, i, 24, param_default, &dxl_error))
                {
                    // Notify(msg_warning, "[ID:%03d] write1ByteTxRx failed", i);
                    portHandlerPupil->clearPort();
                    return false;
                }
                */
            
            if (!goalPosition.empty())
            {
                uint16_t param_default = goalPosition[index]; // Not using degrees.
                // Goal postiion feature/bug. If torque enable = 0 and goal position is sent. Torque enable will be 1.                
                if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, i, 30, param_default, &dxl_error))
                {
                    Notify(msg_warning, std::string("[ID:%03d] write2ByteTxRx failed") + std::to_string(i)); 
                    portHandlerPupil->clearPort();
                    return false;
                }
            
                
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

    bool Communication(int IDMin, int IDMax, int IOIndex, dynamixel::PortHandler *portHandler, dynamixel::PacketHandler *packetHandler, dynamixel::GroupSyncRead *groupSyncRead, dynamixel::GroupSyncWrite *groupSyncWrite)
    {
        if (portHandler == NULL) // If no port handler return true. Only return false if communication went wrong.
            return true;

        int index = 0;
        int dxl_comm_result = COMM_TX_FAIL; // Communication result
        bool dxl_addparam_result = false;   // addParam result
        bool dxl_getdata_result = false;    // GetParam result

        uint8_t dxl_error = 0;       // Dynamixel error
        uint8_t param_sync_write[7]; // 7 byte sync write is not supported for the DynamixelSDK

        int32_t dxl_present_position = 0;
        int16_t dxl_present_current = 0;
        int8_t dxl_present_temperature = 0;
        uint16_t dxl_goal_current = 0;

        // Add if for syncread
        for (int i = IDMin; i <= IDMax; i++){
            if (!groupSyncRead->addParam(i))
            {
                groupSyncWrite->clearParam();
                groupSyncRead->clearParam();
                return false;
            }
        }

        // Sync read
        dxl_comm_result = groupSyncRead->txRxPacket();
        if (dxl_comm_result != COMM_SUCCESS)
        {
            groupSyncWrite->clearParam();
            groupSyncRead->clearParam();
            return false;
        }

        // Check if data is available
        for (int i = IDMin; i <= IDMax; i++)
        {
            dxl_comm_result = groupSyncRead->isAvailable(i, 634, 4 + 2 + 1);
            if (!dxl_comm_result)
            {
                groupSyncWrite->clearParam();
                groupSyncRead->clearParam();
                return false;
            }
        }

        // Extract data
        index = IOIndex;
        for (int i = IDMin; i <= IDMax; i++)
        {
            dxl_present_position = groupSyncRead->getData(i, 634, 4);    // Present position
            dxl_present_current = groupSyncRead->getData(i, 638, 2); // Present current
            

            presentPosition[index] = dxl_present_position / 4095.0 * 360.0; // degrees
            presentCurrent[index] = dxl_present_current * 3.36;   // mA
            index++;
        }

        // Send (sync write)
        index = IOIndex;
        for (int i = IDMin; i <= IDMax; i++)
        {
            param_sync_write[0] = 1; // Torque on

            if (!goalPosition.empty())
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
                groupSyncWrite->clearParam();
                groupSyncRead->clearParam();
                return false;
            }

            if (goalCurrent.connected())
            {
                int value = goalCurrent[index] / 3.36;
                param_sync_write[5] = DXL_LOBYTE(DXL_LOWORD(value));
                param_sync_write[6] = DXL_HIBYTE(DXL_LOWORD(value));
            }
            else
            {
                int value = 2047.0 / 3.36;
                param_sync_write[5] = DXL_LOBYTE(DXL_LOWORD(value));
                param_sync_write[6] = DXL_HIBYTE(DXL_LOWORD(value));
            }


            dxl_addparam_result = groupSyncWrite->addParam(i, param_sync_write, 7);
            if (!dxl_addparam_result)
            {
                groupSyncWrite->clearParam();
                groupSyncRead->clearParam();
                return false;
            }

            index++;
        }

        // Syncwrite
        dxl_comm_result = groupSyncWrite->txPacket();
        if (dxl_comm_result != COMM_SUCCESS)
        {
            groupSyncWrite->clearParam();
            groupSyncRead->clearParam();
            std::cout << "Sync failed" << std::endl;
            return false;
        }

        groupSyncWrite->clearParam();
        groupSyncRead->clearParam();
        return true;
    }


    void Init()
    {
        
        // Robots configurations
        robot["EpiWhite"] = {.serialPortPupil = "/dev/cu.usbserial-FT66WV4A",
                             .serialPortHead = "/dev/cu.usbserial-FT6S4JL9",
                             .serialPortBody = "",
                             .serialPortLeftArm = "",
                             .serialPortRightArm = "",
                             .type = "EpiTorso"};

        robot["EpiRed"] = {.serialPortPupil = "/dev/cu.usbserial-FT4TCJXI",
                           .serialPortHead = "/dev/cu.usbserial-FT4TCGUT",
                           .serialPortBody = "",
                           .serialPortLeftArm = "",
                           .serialPortRightArm = "",
                           .type = "EpiTorso"};

        robot["EpiRedDemo"] = {.serialPortPupil = "/dev/ttyUSB0",
                               .serialPortHead = "/dev/ttyUSB1",
                               .serialPortBody = "",
                               .serialPortLeftArm = "",
                               .serialPortRightArm = "",
                               .type = "EpiTorso"};

        robot["EpiOrange"] = {.serialPortPupil = "/dev/cu.usbserial-FT3WI2WH",
                              .serialPortHead = "/dev/cu.usbserial-FT3WI2K2",
                              .serialPortBody = "",
                              .serialPortLeftArm = "",
                              .serialPortRightArm = "",
                              .type = "EpiTorso"};

        robot["EpiYellow"] = {.serialPortPupil = "/dev/cu.usbserial-FT6S4IFI",
                              .serialPortHead = "/dev/cu.usbserial-FT6RW7PH",
                              .serialPortBody = "",
                              .serialPortLeftArm = "",
                              .serialPortRightArm = "",
                              .type = "EpiTorso"};

        robot["EpiGreen"] = {.serialPortPupil = "/dev/cu.usbserial-FT6S4JMH",
                             .serialPortHead = "/dev/cu.usbserial-FT66WT6W",
                             .serialPortBody = "",
                             .serialPortLeftArm = "",
                             .serialPortRightArm = "",
                             .type = "EpiTorso"};

        robot["EpiBlue"] = {.serialPortPupil = "/dev/cu.usbserial-FT66WS1F",
                            .serialPortHead = "/dev/cu.usbserial-FT4THUNJ",
                            .serialPortBody = "/dev/cu.usbserial-FT4THV1M",
                            .serialPortLeftArm = "/dev/cu.usbserial-FT4TFSV0",
                            .serialPortRightArm = "/dev/cu.usbserial-FT4TCVTT",
                            .type = "Epi"};

        robot["EpiGray"] = {.serialPortPupil = "/dev/cu.usbserial-FT6S4JL5",
                            .serialPortHead = "/dev/cu.usbserial-FT66WV43",
                            .serialPortBody = "",
                            .serialPortLeftArm = "",
                            .serialPortRightArm = "",
                            .type = "EpiTorso"};

        robot["EpiBlack"] = {.serialPortPupil = "/dev/cu.usbserial-FT1SEOJY",
                             .serialPortHead = "/dev/cu.usbserial-FT3WHSCR",
                             .serialPortBody = "",
                             .serialPortLeftArm = "",
                             .serialPortRightArm = "",
                             .type = "EpiTorso"};

        robotName = GetValue("robot");

        // Check if robotname exist in configuration
        if (robot.find(robotName) == robot.end())
        {
            Notify(msg_fatal_error, std::string("%s is not supported") + robotName);
            return;
        }

        // Check type of robot
        EpiTorsoMode = (robot[robotName].type.compare("EpiTorso") == 0);
        EpiMode = (robot[robotName].type.compare("Epi") == 0);

        //Notify(msg_debug, "Connecting to %s (%s)\n", robotName.c_str(), robot[robotName].type.c_str());

        std::cout << robot[robotName].type << std::endl;
        // Ikaros input
        Bind(goalPosition, "GOAL_POSITION");
        Bind(goalCurrent, "GOAL_CURRENT");

        // Ikaros output
        Bind(presentPosition, "PRESENT_POSITION");
        Bind(presentCurrent, "PRESENT_CURRENT");


       

        // Check if the input size are correct. We do not need to have an input at all!
        if (EpiTorsoMode)
        {
            if (!goalPosition.empty())
                if (goalPosition.size_x() < EPI_TORSO_NR_SERVOS)
                    Notify(msg_fatal_error, "Input size goal position does not match robot type\n");
            if (!goalCurrent.empty())
                if (goalCurrent.size() < EPI_TORSO_NR_SERVOS)
                    Notify(msg_fatal_error, "Input size goal current does not match robot type\n");
           /* if (!torqueEnable.empty())
                if (torqueEnable.size() < EPI_TORSO_NR_SERVOS)
                    Notify(msg_fatal_error, "Input size torque enable does not match robot type\n");*/
        }
        else if (EpiMode)
        {
            if (!goalPosition.empty())
                if (goalPosition.size() < EPI_NR_SERVOS)
                    Notify(msg_fatal_error, "Input size goal position does not match robot type\n");
            if (!goalCurrent.empty())
                if (goalCurrent.size() < EPI_NR_SERVOS)
                    Notify(msg_fatal_error, "Input size goal current does not match robot type\n");
            /*if (!torqueEnable.empty())
                if (torqueEnable.size() < EPI_NR_SERVOS)
                    Notify(msg_fatal_error, "Input size torque enable does not match robot type\n");*/
        }

        // Ikaros parameter simulate
        // simulate = GetBoolValue("simulate");
        Bind(simulate, "simulate");

        if (simulate)
        {
            Notify(msg_print, "Simulate servos");
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
            // if (dxl_comm_result != COMM_SUCCESS)
            //     Notify(msg_warning, "%s\n", packetHandlerHead->getTxRxResult(dxl_comm_result));

            Notify(msg_debug, "Detected Dynamixel (Head): \n");
            // for (int i = 0; i < (int)vec.size(); i++)
            //     Notify(msg_debug, "[ID:%03d]\n", vec.at(i));

            // Pupil (id 2,3) = XL320 Left eye, right eye

            // Init Dynaxmixel SDK
            portHandlerPupil = dynamixel::PortHandler::getPortHandler(robot[robotName].serialPortPupil.c_str());
            packetHandlerPupil = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION);

            // Open port
            if (portHandlerPupil->openPort())
                Notify(msg_debug, "Succeeded to open serial port!\n");
            else
            {
                Notify(msg_fatal_error, "\n");
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
            // if (dxl_comm_result != COMM_SUCCESS)
            //     Notify(msg_warning, "%s\n", packetHandlerPupil->getTxRxResult(dxl_comm_result));

            Notify(msg_debug, "Detected Dynamixel (Pupil): \n");
            // for (int i = 0; i < (int)vec.size(); i++)
            //     Notify(msg_debug, "[ID:%03d]\n", vec.at(i));
        }
        else
        {
            Notify(msg_fatal_error, "Robot type is not yet implementet\n");
        }
        if (EpiMode)
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
            if (portHandlerLeftArm->setBaudRate(BAUDRATE3M))
                Notify(msg_debug, "Succeeded to change baudrate!\n");
            else
            {
                Notify(msg_fatal_error, "Failed to change baudrate!\n");
                return;
            }
            // Ping all the servos to make sure they are all there.
            dxl_comm_result = packetHandlerLeftArm->broadcastPing(portHandlerLeftArm, vec);
            // if (dxl_comm_result != COMM_SUCCESS)
            //     Notify(msg_warning, "%s\n", packetHandlerLeftArm->getTxRxResult(dxl_comm_result));

            Notify(msg_debug, "Detected Dynamixel (Left arm): \n");
            // for (int i = 0; i < (int)vec.size(); i++)
            //     Notify(msg_debug, "[ID:%03d]\n", vec.at(i));

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
            if (portHandlerRightArm->setBaudRate(BAUDRATE3M))
                Notify(msg_debug, "Succeeded to change baudrate!\n");
            else
            {
                Notify(msg_fatal_error, "Failed to change baudrate!\n");
                return;
            }
            // Ping all the servos to make sure they are all there.
            dxl_comm_result = packetHandlerRightArm->broadcastPing(portHandlerRightArm, vec);
            // if (dxl_comm_result != COMM_SUCCESS)
            //     Notify(msg_warning, "%s\n", packetHandlerRightArm->getTxRxResult(dxl_comm_result));

            Notify(msg_debug, "Detected Dynamixel (Right arm): \n");
            // for (int i = 0; i < (int)vec.size(); i++)
            //     Notify(msg_debug, "[ID:%03d]\n", vec.at(i));

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
            if (portHandlerBody->setBaudRate(BAUDRATE3M))
                Notify(msg_debug, "Succeeded to change baudrate!\n");
            else
            {
                Notify(msg_fatal_error, "Failed to change baudrate!\n");
                return;
            }
            // Ping all the servos to make sure they are all there.
            dxl_comm_result = packetHandlerBody->broadcastPing(portHandlerBody, vec);
            // if (dxl_comm_result != COMM_SUCCESS)
            //     Notify(msg_warning, "%s\n", packetHandlerBody->getTxRxResult(dxl_comm_result));

            // Notify(msg_debug, "Detected Dynamixel (Body): \n");
            // for (int i = 0; i < (int)vec.size(); i++)
            //     Notify(msg_debug, "[ID:%03d]\n", vec.at(i));
        }

        // Create dynamixel objects
        if (EpiTorsoMode || EpiMode)
        {
            groupSyncWriteHead = new dynamixel::GroupSyncWrite(portHandlerHead, packetHandlerHead, 224, 1 + 4 + 2); // Torque enable, goal position, goal current
            groupSyncReadHead = new dynamixel::GroupSyncRead(portHandlerHead, packetHandlerHead, 634, 4 + 2 + 1 +2);   // Present poistion, presernt current, temperature goal current
        }
        if (EpiMode)
        {
            groupSyncWriteLeftArm = new dynamixel::GroupSyncWrite(portHandlerLeftArm, packetHandlerLeftArm, 224, 1 + 4 + 2);
            groupSyncReadLeftArm = new dynamixel::GroupSyncRead(portHandlerLeftArm, packetHandlerLeftArm, 634, 4 + 2 + 1);
            groupSyncWriteRightArm = new dynamixel::GroupSyncWrite(portHandlerRightArm, packetHandlerRightArm, 224, 1 + 4 + 2);
            groupSyncReadRightArm = new dynamixel::GroupSyncRead(portHandlerRightArm, packetHandlerRightArm, 634, 4 + 2 + 1);
            groupSyncWriteBody = new dynamixel::GroupSyncWrite(portHandlerBody, packetHandlerBody, 224, 1 + 4 + 2);
            groupSyncReadBody = new dynamixel::GroupSyncRead(portHandlerBody, packetHandlerBody, 634, 4 + 2 + 1);
            groupSyncWritePupil = new dynamixel::GroupSyncWrite(portHandlerPupil, packetHandlerPupil, 30, 2); // no read..
        }

        AutoCalibratePupil();

        Notify(msg_debug, "torque down servos and prepering servos for write defualt settings\n");
        if (!PowerOffRobot())
            Notify(msg_fatal_error, "Unable torque down servos\n");

        if (!SetDefaultSettingServo())
            Notify(msg_fatal_error, "Unable to write default settings on servos\n");

        if (!PowerOnRobot())
            Notify(msg_fatal_error, "Unable torque up servos\n");
    }

    float PupilMMToDynamixel(float mm, int min, int max)
    {
        // Quick fix
        float minMM = 4.2;
        float maxMM = 19.7;
        float deltaMM = maxMM - minMM;

        float minDeg = min;
        float maxDeg = max;
        float deltDeg = maxDeg - minDeg;

        if (mm < minMM)
            mm = minMM;
        if (mm > maxMM)
            mm = maxMM;

        return (-(mm - minMM) / deltaMM * deltDeg + maxDeg); // Higher goal position gives smaller pupil
    }

    void Tick()
    {   
       
        goalPosition[PUPIL_INDEX_IO] = clip(goalPosition[PUPIL_INDEX_IO], 5, 16); // Pupil size must be between 5 mm to 16 mm.
        goalPosition[PUPIL_INDEX_IO + 1] = clip(goalPosition[PUPIL_INDEX_IO + 1], 5, 16); // Pupil size must be between 5 mm to 16 mm.

        // Special case. As pupil does not have any feedback we just return goal position
        presentPosition[PUPIL_INDEX_IO]    =     goalPosition[PUPIL_INDEX_IO];
        presentPosition[PUPIL_INDEX_IO+1]  =     goalPosition[PUPIL_INDEX_IO+1];
    
        
        
        if (simulate)
        {
            
           

            //reset_array(presentCurrent, presentCurrentSize); // 0 mA

            float maxVel = 45.0 / 1000 * 10; // No timebase

            // Comment out GetTickLength() Adding constant.
            // if (GetTickLength() != 0)
            //    maxVel = 45.0 / 1000 * GetTickLength(); // Maximum change in one second in degrees / timebase
            maxVel = 0.01; // This should be a ratio parameter

            if (EpiMode)
            {
                
                for (int i = 0; i < EPI_NR_SERVOS; i++){
                    if (std::isnan(goalPosition[i]))
                    {
                    Notify(msg_warning, "EpiServo module input is NAN\n");
                    return;
                    }
                    if (!goalPosition.empty())
                        presentPosition[i] = presentPosition[i] + 0.9 * (clip(goalPosition(i) - presentPosition(i), -maxVel, maxVel)); // adding some smoothing to prevent oscillation in simulation mode
                }
            }
            else
            {
                //Notify(msg_debug, "Simulating EpiTorso");
                for (int i = 0; i < EPI_TORSO_NR_SERVOS; i++)
                    if (!goalPosition.empty() && !goalCurrent.empty()){
                        presentCurrent(i) = presentCurrent(i) + 0.06 * (goalCurrent(i) - presentCurrent(i));
                        if (i == 0 && presentPosition(i) > 200 && presentCurrent(i) < 700){
                            presentPosition(i) = presentPosition(i);
                        }
                        else
                            presentPosition(i) = presentPosition(i) + 0.02 * (goalPosition(i) - presentPosition(i)); // adding some smoothing to prevent oscillation in simulation mode
                        
                    }
            }
            // Create fake feedback in simulation mode
            // Temporary disable GetTick()
            // if (GetTick() < 10)
            // {
            //set_array(presentPosition, 180, presentPositionSize);
            //presentPosition[PUPIL_INDEX_IO] = 12;
            //presentPosition[PUPIL_INDEX_IO + 1] = 12;

            
            // }
            return;
        }

        dictionary d;

        // Special case for pupil uses mm instead of degrees
        goalPosition[PUPIL_INDEX_IO] = PupilMMToDynamixel(goalPosition[PUPIL_INDEX_IO], AngleMinLimitPupil[0], AngleMaxLimitPupil[0]);
        goalPosition[PUPIL_INDEX_IO + 1] = PupilMMToDynamixel(goalPosition[PUPIL_INDEX_IO + 1], AngleMinLimitPupil[1], AngleMaxLimitPupil[1]);
        auto headThread = std::async(std::launch::async, &EpiServos::Communication, this, HEAD_ID_MIN, HEAD_ID_MAX, HEAD_INDEX_IO, std::ref(portHandlerHead), std::ref(packetHandlerHead), std::ref(groupSyncReadHead), std::ref(groupSyncWriteHead));
        auto pupilThread = std::async(std::launch::async, &EpiServos::CommunicationPupil, this); // Special!
    
        if (!headThread.get())
        {
            Notify(msg_warning,"Can not communicate with head");
            portHandlerHead->clearPort();
        }
        if (!pupilThread.get())
        {
            Notify(msg_warning, "Can not communicate with pupil");
            portHandlerPupil->clearPort();
        }
        if(EpiMode)
        {
            auto leftArmThread = std::async(std::launch::async, &EpiServos::Communication, this, ARM_ID_MIN, ARM_ID_MAX, LEFT_ARM_INDEX_IO, std::ref(portHandlerLeftArm), std::ref(packetHandlerLeftArm), std::ref(groupSyncReadLeftArm), std::ref(groupSyncWriteLeftArm));
            auto rightArmThread = std::async(std::launch::async, &EpiServos::Communication, this, ARM_ID_MIN, ARM_ID_MAX, RIGHT_ARM_INDEX_IO, std::ref(portHandlerRightArm), std::ref(packetHandlerRightArm), std::ref(groupSyncReadRightArm), std::ref(groupSyncWriteRightArm));
            auto bodyThread = std::async(std::launch::async, &EpiServos::Communication, this, BODY_ID_MIN, BODY_ID_MIN, BODY_INDEX_IO, std::ref(portHandlerBody), std::ref(packetHandlerBody), std::ref(groupSyncReadBody), std::ref(groupSyncWriteBody));
        
            if (!leftArmThread.get())
            {
                Notify(msg_warning, "Can not communicate with left arm");
                portHandlerLeftArm->clearPort();
            }
            if (!rightArmThread.get())
            {
                Notify(msg_warning, "Can not communicate with right arm");
                portHandlerRightArm->clearPort();
            }
            if (!bodyThread.get())
            {
                Notify(msg_warning, "Can not communicate with body");
                portHandlerBody->clearPort();
            }
        }
    }

    // A function that set importat parameters in the control table.
    // Baud rate and ID needs to be set manually.
    bool SetDefaultSettingServo() {
        uint32_t param_default_4Byte;
        uint32_t profile_acceleration = 0;
        uint32_t profile_velocity = 0;
        
        uint16_t p_gain_head = 100;
        uint16_t i_gain_head = 10;
        uint16_t d_gain_head = 1200;
        
        uint16_t p_gain_arm = 100;
        uint16_t i_gain_arm = 0;
        uint16_t d_gain_arm = 1000;
        
        uint16_t p_gain_body = 100;
        uint16_t i_gain_body = 0;
        uint16_t d_gain_body = 1000;

       
        uint16_t pupil_moving_speed = 150;
        uint8_t param_default_1Byte;
        uint8_t pupil_p_gain = 100;
        uint8_t pupil_i_gain = 20;
        uint8_t pupil_d_gain = 5;


        uint8_t dxl_error = 0; 
        int dxl_comm_result = COMM_TX_FAIL;

        Notify(msg_debug, "Setting control table on servos\n");

        // Torque Enable
        for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++) {
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, IND_ADDR_TORQUE_ENABLE, ADDR_TORQUE_ENABLE, &dxl_error)) {
                std::cout << "Failed to set torque enable for head servo ID: " << i << std::endl;
                return false;
            }
        }
        if (EpiMode) {
            for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++) {
                if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, i, IND_ADDR_TORQUE_ENABLE, ADDR_TORQUE_ENABLE, &dxl_error)) {
                    std::cout << "Failed to set torque enable for left arm servo ID: " << i << std::endl;
                    return false;
                }
            }
            for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++) {
                if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, i, IND_ADDR_TORQUE_ENABLE, ADDR_TORQUE_ENABLE, &dxl_error)) {
                    std::cout << "Failed to set torque enable for right arm servo ID: " << i << std::endl;
                    return false;
                }
            }
            for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++) {
                if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerBody, i, IND_ADDR_TORQUE_ENABLE, ADDR_TORQUE_ENABLE, &dxl_error)) {
                    std::cout << "Failed to set torque enable for body servo ID: " << i << std::endl;
                    return false;
                }
            }
        } 

        // Goal Position
        for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++) {
            for (int j = 0; j < 4; j++) {
                if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, IND_ADDR_GOAL_POSITION + (2 * j), ADDR_GOAL_POSITION + j, &dxl_error)) {
                    std::cout << "Failed to set goal position for head servo ID: " << i << ", byte: " << j << std::endl;
                    return false;
                }
            }
        }
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


        // Goal Current
        for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++) {
            for (int j = 0; j < 2; j++) {
                if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, IND_ADDR_GOAL_CURRENT + (2 * j), ADDR_GOAL_CURRENT + j, &dxl_error)) {
                    std::cout << "Goal current not set for head servo ID: " << i << ", byte: " << j << std::endl;
                    return false;
                }
            }
        }
         // Indirect adress (present position). Feedback
        for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++){
            for (int j = 0; j < 4; j++){
                if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, IND_ADDR_PRESENT_POSITION + (2 * j), ADDR_PRESENT_POSITION + j, &dxl_error)){
                    std::cout << "Present position not set for head servo ID: " << i << ", byte: " << j << std::endl;
                    return false;
                }
            }
        }
        if (EpiMode)
        {
            for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
                for (int j = 0; j < 4; j++)
                    if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, i, IND_ADDR_PRESENT_POSITION + (2 * j), ADDR_PRESENT_POSITION + j, &dxl_error))
                        return false;
            for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
                for (int j = 0; j < 4; j++)
                    if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, i, IND_ADDR_PRESENT_POSITION + (2 * j), ADDR_PRESENT_POSITION + j, &dxl_error))
                        return false;
            for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
                for (int j = 0; j < 4; j++)
                    if (COMM_SUCCESS != packetHandlerBody->write2ByteTxRx(portHandlerBody, i, IND_ADDR_PRESENT_POSITION + (2 * j), ADDR_PRESENT_POSITION + j, &dxl_error))
                        return false;
        }
        // Indirect adress (present current). Feedback. MX28 does not support current mode.
        for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++){
            for (int j = 0; j < 2; j++){
                if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, IND_ADDR_PRESENT_CURRENT + (2 * j), ADDR_PRESENT_CURRENT + j, &dxl_error)){
                    std::cout << "Present current not set for head servo ID: " << i << ", byte: " << j << std::endl;
                    return false;
                }
            }
        }
        if (EpiMode)
        {
            for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
                for (int j = 0; j < 2; j++)
                    if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, i, IND_ADDR_PRESENT_CURRENT + (2 * j), ADDR_PRESENT_CURRENT + j, &dxl_error))
                        return false;
            for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
                for (int j = 0; j < 2; j++)
                    if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, i, IND_ADDR_PRESENT_CURRENT + (2 * j), ADDR_PRESENT_CURRENT + j, &dxl_error))
                        return false;
            for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
                for (int j = 0; j < 2; j++)
                    if (COMM_SUCCESS != packetHandlerBody->write2ByteTxRx(portHandlerBody, i, IND_ADDR_PRESENT_CURRENT + (2 * j), ADDR_PRESENT_CURRENT + j, &dxl_error))
                        return false;
        }
        // Indirect adress (present temperature).
        for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++){
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, IND_ADDR_PRESENT_TEMPERATURE, ADDR_PRESENT_TEMPERATURE, &dxl_error)){
                std::cout << "Present temperature indir not set for head servo ID: " << i << ", byte: " << std::endl;
                return false;
            }
        }
        if (EpiMode)
        {
            for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
                if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, i, IND_ADDR_PRESENT_TEMPERATURE, ADDR_PRESENT_TEMPERATURE, &dxl_error))
                    return false;
            for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
                if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, i, IND_ADDR_PRESENT_TEMPERATURE, ADDR_PRESENT_TEMPERATURE, &dxl_error))
                    return false;
            for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
                if (COMM_SUCCESS != packetHandlerBody->write2ByteTxRx(portHandlerBody, i, IND_ADDR_PRESENT_TEMPERATURE, ADDR_PRESENT_TEMPERATURE, &dxl_error))
                    return false;
        }

        // Profile acceleration

    for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++){
            if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, i, ADDR_PROFILE_ACCELERATION, profile_acceleration, &dxl_error)){
                std::cout << "Profile acceleration for head servo ID: " << i << std::endl;
                return false;
            }
        }
        if (EpiMode)
        {
            for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
                if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, i, ADDR_PROFILE_ACCELERATION, profile_acceleration, &dxl_error))
                    return false;
            for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
                if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, i, ADDR_PROFILE_ACCELERATION, profile_acceleration, &dxl_error))
                    return false;
            for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
                if (COMM_SUCCESS != packetHandlerBody->write4ByteTxRx(portHandlerBody, i, ADDR_PROFILE_ACCELERATION, profile_acceleration, &dxl_error))
                    return false;
        }

        // Common settings for the servos
        // Profile velocity (210)

        for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++){
            if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, i, ADDR_PROFILE_VELOCITY, profile_velocity, &dxl_error)){
                std::cout << "Profile velocity not set for head servo ID: " << i << std::endl;
                return false;
                }
        }
        if (EpiMode)
        {
            for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
                if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, i, ADDR_PROFILE_VELOCITY, profile_velocity, &dxl_error))
                    return false;
            for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
                if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, i, ADDR_PROFILE_VELOCITY, profile_velocity, &dxl_error))
                    return false;
            for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
                if (COMM_SUCCESS != packetHandlerBody->write4ByteTxRx(portHandlerBody, i, ADDR_PROFILE_VELOCITY, profile_velocity, &dxl_error))
                    return false;
        }

        // P (100)
        for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++){
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, ADDR_P, p_gain_head, &dxl_error)){
                std::cout << "P (PID) not set for head servo ID: " << i <<std::endl;
                return false;
            }
        }
        if (EpiMode)
        {
            for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
                if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, i, ADDR_P, p_gain_arm, &dxl_error))
                    return false;
            for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
                if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, i, ADDR_P, p_gain_arm, &dxl_error))
                    return false;
            for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
                if (COMM_SUCCESS != packetHandlerBody->write2ByteTxRx(portHandlerBody, i, ADDR_P, p_gain_body, &dxl_error))
                    return false;
        }

        // I
        // The I value almost killed Epi.
        for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++){
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, ADDR_I, i_gain_head, &dxl_error)){
                std::cout << "I (PID) not set for head servo ID: " << i<< std::endl;
                return false;
            }
        }
        if (EpiMode)
        {
            for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
                if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, i, ADDR_I, i_gain_arm, &dxl_error))
                    return false;
            for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
                if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, i, ADDR_I, i_gain_arm, &dxl_error))
                    return false;
            for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
                if (COMM_SUCCESS != packetHandlerBody->write2ByteTxRx(portHandlerBody, i, ADDR_I, i_gain_body, &dxl_error))
                    return false;
        }

        // D
        for (int i = HEAD_ID_MIN; i <= HEAD_ID_MAX; i++){
            if (COMM_SUCCESS != packetHandlerHead->write2ByteTxRx(portHandlerHead, i, ADDR_D, d_gain_head, &dxl_error)){
                std::cout << "D (PID) not set for head servo ID: " << i << std::endl;
                return false;
            }
        }
        if (EpiMode)
        {
            for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
                if (COMM_SUCCESS != packetHandlerLeftArm->write2ByteTxRx(portHandlerLeftArm, i, ADDR_D, d_gain_arm, &dxl_error))
                    return false;
            for (int i = ARM_ID_MIN; i <= ARM_ID_MAX; i++)
                if (COMM_SUCCESS != packetHandlerRightArm->write2ByteTxRx(portHandlerRightArm, i, ADDR_D, d_gain_arm, &dxl_error))
                    return false;
            for (int i = BODY_ID_MIN; i <= BODY_ID_MAX; i++)
                if (COMM_SUCCESS != packetHandlerBody->write2ByteTxRx(portHandlerBody, i, ADDR_D, d_gain_body, &dxl_error))
                    return false;
        }

        // Specific setting for each servos
        // HEAD ID 2
        // Limit position max
        uint32_t limit_pos_max_tilt = 2700;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 2, 48, limit_pos_max_tilt, &dxl_error)){
            std::cout << "Max limit not set for head servo ID: 2 " << std::endl;
            return false;
            }
        // Limit position min
        uint32_t limit_pos_min_tilt = 1300;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 2, 52, limit_pos_min_tilt, &dxl_error)){
            std::cout << "Min limit not set for head servo ID: 2 " << std::endl;
            return false;
            }

        // HEAD ID 3
        // Limit position max
        uint32_t limit_pos_max_pan = 2500;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 3, 48, limit_pos_max_pan, &dxl_error))
            return false;
        // Limit position min
        uint32_t limit_pos_min_pan = 1750;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 3, 52, limit_pos_min_pan, &dxl_error))
            return false;

        // HEAD ID 4 (Left eye)
        // Limit position max
        uint32_t limit_pos_max_left_eye = 2300;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 4, 48, limit_pos_max_left_eye, &dxl_error))
            return false;
        // Limit position min
        uint32_t limit_pos_min_left_eye = 1830;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 4, 52, limit_pos_min_left_eye, &dxl_error))
            return false;

        // HEAD ID 5 (Right eye)
        // Limit position max
        uint32_t limit_pos_max_right_eye = 2200;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 5, 48, limit_pos_max_right_eye, &dxl_error))
            return false;

        // Limit position min
        uint32_t limit_pos_min_right_eye = 1780;
        if (COMM_SUCCESS != packetHandlerHead->write4ByteTxRx(portHandlerHead, 5, 52, limit_pos_min_right_eye, &dxl_error))
            return false;

        Timer t;
        double xlTimer = 0.01; // Timer in sec. XL320 need this. Not sure why.

        // PUPIL ID 2 (Left pupil)
        // Limit position min
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 6, AngleMinLimitPupil[0], &dxl_error))
            return false;
        Sleep(xlTimer);

        // Limit position max
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 8, AngleMaxLimitPupil[0], &dxl_error))
            return false;
        Sleep(xlTimer);

        // Moving speed
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 32, pupil_moving_speed, &dxl_error))
            return false;
        Sleep(xlTimer);

        // P
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 29, pupil_p_gain, &dxl_error))
            return false;
        Sleep(xlTimer);


        // I
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 28, pupil_i_gain, &dxl_error))
            return false;
        Sleep(xlTimer);


        // D
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 27, pupil_d_gain, &dxl_error))
            return false;
        Sleep(xlTimer);


        // PUPIL ID 3 (Right pupil)
        // Limit position in
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 6, AngleMinLimitPupil[1], &dxl_error))
            return false;
        Sleep(xlTimer);


        // Limit position max
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 8, AngleMaxLimitPupil[1], &dxl_error))
            return false;
        Sleep(xlTimer);


        // Moving speed
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 32, pupil_moving_speed, &dxl_error))
            return false;
        Sleep(xlTimer);

        // P
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 29, pupil_p_gain, &dxl_error))
            return false;
        Sleep(xlTimer);


        // I
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 28, pupil_i_gain, &dxl_error))
            return false;
        Sleep(xlTimer);


        // D
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 27, pupil_d_gain, &dxl_error))
            return false;
        Sleep(xlTimer);


        if (EpiMode)
        {
            // LEFT ARM ID 2
            // Limit position max
            param_default_4Byte = 3200;
            if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 2, 48, param_default_4Byte, &dxl_error))
                return false;
            // Limit position min
            param_default_4Byte = 600;
            if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 2, 52, param_default_4Byte, &dxl_error))
                return false;

            // LEFT ARM ID 3
            // Limit position max
            param_default_4Byte = 3200;
            if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 3, 48, param_default_4Byte, &dxl_error))
                return false;
            // Limit position min
            param_default_4Byte = 800;
            if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 3, 52, param_default_4Byte, &dxl_error))
                return false;

            // LEFT ARM ID 4
            // Limit position max
            param_default_4Byte = 3000;
            if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 4, 48, param_default_4Byte, &dxl_error))
                return false;
            // Limit position min
            param_default_4Byte = 1000;
            if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 4, 52, param_default_4Byte, &dxl_error))
                return false;

            // LEFT ARM ID 5
            // Limit position max
            param_default_4Byte = 2300;
            if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 5, 48, param_default_4Byte, &dxl_error))
                return false;
            // Limit position min
            param_default_4Byte = 600;
            if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 5, 52, param_default_4Byte, &dxl_error))
                return false;

            // LEFT ARM ID 6
            // Limit position max
            param_default_4Byte = 3900;
            if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 6, 48, param_default_4Byte, &dxl_error))
                return false;
            // Limit position min
            param_default_4Byte = 800;
            if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 6, 52, param_default_4Byte, &dxl_error))
                return false;

            // LEFT ARM ID 7
            // Limit position max
            param_default_4Byte = 4095;
            if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 7, 48, param_default_4Byte, &dxl_error))
                return false;
            // Limit position min
            param_default_4Byte = 0;
            if (COMM_SUCCESS != packetHandlerLeftArm->write4ByteTxRx(portHandlerLeftArm, 7, 52, param_default_4Byte, &dxl_error))
                return false;

            // RIGHT ARM ID 2
            // Limit position max
            param_default_4Byte = 3200;
            if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 2, 48, param_default_4Byte, &dxl_error))
                return false;
            // Limit position min
            param_default_4Byte = 600;
            if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 2, 52, param_default_4Byte, &dxl_error))
                return false;
            // RIGHT ARM ID 3
            // Limit position max
            param_default_4Byte = 3300;
            if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 3, 48, param_default_4Byte, &dxl_error))
                return false;
            // Limit position min
            param_default_4Byte = 900;
            if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 3, 52, param_default_4Byte, &dxl_error))
                return false;
            // RIGHT ARM ID 4
            // Limit position max
            param_default_4Byte = 3000;
            if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 4, 48, param_default_4Byte, &dxl_error))
                return false;
            // Limit position min
            param_default_4Byte = 1000;
            if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 4, 52, param_default_4Byte, &dxl_error))
                return false;
            // RIGHT ARM ID 5
            // Limit position max
            param_default_4Byte = 3600;
            if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 5, 48, param_default_4Byte, &dxl_error))
                return false;
            // Limit position min
            param_default_4Byte = 1800;
            if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 5, 52, param_default_4Byte, &dxl_error))
                return false;
            // RIGHT ARM ID 6
            // Limit position max
            param_default_4Byte = 3900;
            if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 6, 48, param_default_4Byte, &dxl_error))
                return false;
            // Limit position min
            param_default_4Byte = 800;
            if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 6, 52, param_default_4Byte, &dxl_error))
                return false;
            // RIGHT ARM ID 7
            // Limit position max
            param_default_4Byte = 4095;
            if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 7, 48, param_default_4Byte, &dxl_error))
                return false;
            // Limit position min
            param_default_4Byte = 0;
            if (COMM_SUCCESS != packetHandlerRightArm->write4ByteTxRx(portHandlerRightArm, 7, 52, param_default_4Byte, &dxl_error))
                return false;

            // BODY ID 2
            // Limit position max
            param_default_4Byte = 3900;
            if (COMM_SUCCESS != packetHandlerBody->write4ByteTxRx(portHandlerBody, 2, 48, param_default_4Byte, &dxl_error))
                return false;
            // Limit position min
            param_default_4Byte = 100;
            if (COMM_SUCCESS != packetHandlerBody->write4ByteTxRx(portHandlerBody, 2, 52, param_default_4Byte, &dxl_error))
                return false;
        }
        return true; // Yay we manage to set everything we needed.
    }




    bool PowerOn(int IDMin, int IDMax, dynamixel::PortHandler *portHandler, dynamixel::PacketHandler *packetHandler)
    {
        if (portHandler == NULL) // If no port handler return true. Only return false if communication went wrong.
            return true;

        Notify(msg_debug, "Power on servos");

        Timer t;
        const int nrOfServos = IDMax - IDMin + 1;
        int dxl_comm_result = COMM_TX_FAIL; // Communication result
        uint8_t dxl_error = 0;              // Dynamixel error
        uint16_t start_p_value[7] = {0, 0, 0, 0, 0, 0, 0};
        uint32_t present_postition_value[7] = {0, 0, 0, 0, 0, 0, 0};

        // Get P values
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandler->read2ByteTxRx(portHandler, IDMin + i, 84, &start_p_value[i], &dxl_error))
                return false;

        // Set P value to 0
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandler->write2ByteTxRx(portHandler, IDMin + i, 84, 0, &dxl_error))
                return false;
        // Set torque value to 1
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandler->write1ByteTxRx(portHandler, IDMin + i, 64, 1, &dxl_error))
                return false;
        while (t.GetTime() < TIMER_POWER_ON)
        {
            // Get present position
            for (int i = 0; i < nrOfServos; i++)
                if (COMM_SUCCESS != packetHandler->read4ByteTxRx(portHandler, IDMin + i, 132, &present_postition_value[i], &dxl_error))
                    return false;
            // Set goal position to present postiion
            for (int i = 0; i < nrOfServos; i++)
                if (COMM_SUCCESS != packetHandler->write4ByteTxRx(portHandler, IDMin + i, 116, present_postition_value[i], &dxl_error))
                    return false;
            // Ramping up P
            for (int i = 0; i < nrOfServos; i++)
                if (COMM_SUCCESS != packetHandler->write2ByteTxRx(portHandler, IDMin + i, 84, int(float(start_p_value[i]) / float(TIMER_POWER_ON) * t.GetTime()), &dxl_error))
                    return false;
        }

        // Set P value to start value
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandler->write2ByteTxRx(portHandler, IDMin + i, 84, start_p_value[i], &dxl_error))
                return false;

        return true;
    }

    bool PowerOnPupil()
    {
        uint8_t dxl_error = 0; // Dynamixel error

        // Enable torque. No fancy rampiong
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 24, 1, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 24, 1, &dxl_error))
            return false;
        return true;
    }

    bool PowerOnRobot()
    {
        // Trying to torque up the power of the servos.
        // Dynamixel protocel 2.0
        // In current base position control mode goal current can be used.
        // In poistion control mode P can be used (PID).
        // Torqing up the servos? This can not be done in 2.0 and position mode only in position-current mode.
        // 1. Set P (PID) = 0. Store start P value
        // 2. Set goal poistion to present position
        // 3. Increase current or P (PID)
        // 4. Repeat 2,3 for X seconds.

        auto headThread = std::async(std::launch::async, &EpiServos::PowerOn, this, HEAD_ID_MIN, HEAD_ID_MAX, std::ref(portHandlerHead), std::ref(packetHandlerHead));
        auto pupilThread = std::async(std::launch::async, &EpiServos::PowerOnPupil, this); // Different control table.
        auto leftArmThread = std::async(std::launch::async, &EpiServos::PowerOn, this, ARM_ID_MIN, ARM_ID_MAX, std::ref(portHandlerLeftArm), std::ref(packetHandlerLeftArm));
        auto rightArmThread = std::async(std::launch::async, &EpiServos::PowerOn, this, ARM_ID_MIN, ARM_ID_MAX, std::ref(portHandlerRightArm), std::ref(packetHandlerRightArm));
        auto bodyThread = std::async(std::launch::async, &EpiServos::PowerOn, this, BODY_ID_MIN, BODY_ID_MAX, std::ref(portHandlerBody), std::ref(packetHandlerBody));

        if (!headThread.get())
            Notify(msg_fatal_error, "Can not power on head");
        if (!pupilThread.get())
            Notify(msg_fatal_error, "Can not power on pupil");
        if (!leftArmThread.get())
            Notify(msg_fatal_error, "Can not power on left arm");
        if (!rightArmThread.get())
            Notify(msg_fatal_error, "Can not power on right arm");
        if (!bodyThread.get())
            Notify(msg_fatal_error, "Can not power on body");

        return true;
    }

    bool PowerOff(int IDMin, int IDMax, dynamixel::PortHandler *portHandler, dynamixel::PacketHandler *packetHandler)
    {
        if (portHandler == NULL) // If no port handler return true. Only return false if communication went wrong.
            return true;

        Timer t;
        const int nrOfServos = IDMax - IDMin + 1;
        int dxl_comm_result = COMM_TX_FAIL; // Communication result
        uint8_t dxl_error = 0;              // Dynamixel error
        uint16_t start_p_value[7] = {0, 0, 0, 0, 0, 0, 0};
        uint32_t present_postition_value[7] = {0, 0, 0, 0, 0, 0, 0};

        // Get P values
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandler->read2ByteTxRx(portHandler, IDMin + i, 84, &start_p_value[i], &dxl_error))
                return false;

        // t.Reset();
        Notify(msg_warning, "Power off servos. If needed, support the robot while power off the servos");

        // Ramp down p value
        // while (t.GetTime() < TIMER_POWER_OFF)
        //     for (int i = 0; i < nrOfServos; i++)
        //         if (COMM_SUCCESS != packetHandler->write2ByteTxRx(portHandler, IDMin + i, 84, int(float(start_p_value[i]) * (float(TIMER_POWER_OFF) - t.GetTime()) / float(TIMER_POWER_OFF)), &dxl_error))
        //             return false;
        // Turn p to zero
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandler->write2ByteTxRx(portHandler, IDMin + i, 84, 0, &dxl_error))
                return false;

        Sleep(TIMER_POWER_OFF);

        // Get present position
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandler->read4ByteTxRx(portHandler, IDMin + i, 132, &present_postition_value[i], &dxl_error))
                return false;
        // Set goal position to present postiion
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandler->write4ByteTxRx(portHandler, IDMin + i, 116, present_postition_value[i], &dxl_error))
                return false;

        // t.Restart();
        Sleep(TIMER_POWER_OFF_EXTENDED);

        // Enable torque off
        Notify(msg_debug, "Enable torque off");
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandler->write1ByteTxRx(portHandler, IDMin + i, 64, 0, &dxl_error))
                return false;
        // Set P value to start value
        for (int i = 0; i < nrOfServos; i++)
            if (COMM_SUCCESS != packetHandler->write2ByteTxRx(portHandler, IDMin + i, 84, start_p_value[i], &dxl_error))
                return false;

        return true;
    }

    bool PowerOffPupil()
    {
        uint8_t dxl_error = 0; // Dynamixel error

        // Torque off. No fancy rampiong
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 24, 0, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 24, 0, &dxl_error))
            return false;

        return true;
    }

    bool PowerOffRobot()
    {

        auto headThread = std::async(std::launch::async, &EpiServos::PowerOff, this, HEAD_ID_MIN, HEAD_ID_MAX, std::ref(portHandlerHead), std::ref(packetHandlerHead));
        auto pupilThread = std::async(std::launch::async, &EpiServos::PowerOffPupil, this);
        auto leftArmThread = std::async(std::launch::async, &EpiServos::PowerOff, this, ARM_ID_MIN, ARM_ID_MAX, std::ref(portHandlerLeftArm), std::ref(packetHandlerLeftArm));
        auto rightArmThread = std::async(std::launch::async, &EpiServos::PowerOff, this, ARM_ID_MIN, ARM_ID_MAX, std::ref(portHandlerRightArm), std::ref(packetHandlerRightArm));
        auto bodyThread = std::async(std::launch::async, &EpiServos::PowerOff, this, BODY_ID_MIN, BODY_ID_MAX, std::ref(portHandlerBody), std::ref(packetHandlerBody));

        if (!headThread.get())
            Notify(msg_fatal_error, "Can not power off head");
        if (!pupilThread.get())
            Notify(msg_fatal_error, "Can not power off pupil");
        if (!leftArmThread.get())
            Notify(msg_fatal_error, "Can not power off left arm");
        if (!rightArmThread.get())
            Notify(msg_fatal_error, "Can not power off right arm");
        if (!bodyThread.get())
            Notify(msg_fatal_error, "Can not power off body");

        // Power down servos.
        // 1. Store P (PID) value
        // 2. Ramp down P
        // 3. Turn of torque enable
        // 4. Set P (PID) valued from 1.

        return (true);
    }
    bool AutoCalibratePupil()
    {
        int dxl_comm_result = COMM_TX_FAIL; // Communication result
        uint8_t dxl_error = 0;              // Dynamixel error
        Timer t;
        double xlTimer = 0.010; // Timer in sec. XL320 need this. Not sure why.

        // Torque off. No fancy rampiong
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 24, 0, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 24, 0, &dxl_error))
            return false;
        Sleep(xlTimer);

        // Reset min and max limit
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 6, 0, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 6, 0, &dxl_error))
            return false;
        Sleep(xlTimer);

        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 8, 1023, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 8, 1023, &dxl_error))
            return false;
        Sleep(xlTimer);

        // Turn down torue limit
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 35, 500, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 35, 500, &dxl_error))
            return false;
        Sleep(xlTimer);

        // Torque off. No fancy rampiong
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 24, 1, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 24, 1, &dxl_error))
            return false;
        Sleep(xlTimer);

        // Go to min pos
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 30, 0, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 30, 0, &dxl_error))
            return false;
        // Sleep for 300 ms
        Sleep(xlTimer);

        // Read pressent position
        uint16_t present_postition_value[2] = {0, 0};
        if (COMM_SUCCESS != packetHandlerPupil->read2ByteTxRx(portHandlerPupil, 2, 37, &present_postition_value[0], &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->read2ByteTxRx(portHandlerPupil, 3, 37, &present_postition_value[1], &dxl_error))
            return false;

        AngleMinLimitPupil[0] = present_postition_value[0] + 10;
        AngleMinLimitPupil[1] = present_postition_value[1] + 10;

        AngleMaxLimitPupil[0] = AngleMinLimitPupil[0] + 280;
        AngleMaxLimitPupil[1] = AngleMinLimitPupil[1] + 280;

       // Not implemented.
       //Notify(msg_debug, "Position limits pupil servos (auto calibrate): min %i %i max %i %i \n", AngleMinLimitPupil[0], AngleMinLimitPupil[1], AngleMaxLimitPupil[0], AngleMaxLimitPupil[1]);

        // Torque off. No fancy rampiong
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 2, 24, 0, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write1ByteTxRx(portHandlerPupil, 3, 24, 0, &dxl_error))
            return false;
        Sleep(xlTimer);

        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 2, 35, 1023, &dxl_error))
            return false;
        if (COMM_SUCCESS != packetHandlerPupil->write2ByteTxRx(portHandlerPupil, 3, 35, 1023, &dxl_error))
            return false;
        Sleep(xlTimer);


        return true;
    }
    ~EpiServos()
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

};

INSTALL_CLASS(EpiServos)
