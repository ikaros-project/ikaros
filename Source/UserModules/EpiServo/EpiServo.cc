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

//#include "/usr/local/include/dynamixel_sdk/dynamixel_sdk.h"                                  // Uses Dynamixel SDK library
#include "dynamixel_sdk.h" // Uses Dynamixel SDK library

using namespace ikaros;

// Protocol version
#define PROTOCOL_VERSION 2.0 // See which protocol version is used in the Dynamixel

// Default setting
#define BAUDRATE 57600
#define DEVICENAME "/dev/ttyUSB0" // Check which port is being used on your controller
                                  // ex) Windows: "COM1"   Linux: "/dev/ttyUSB0" Mac: "/dev/tty.usbserial-*"

void EpiServo::Init()
{
    // Epi torso
    // =========

    // Open serialports
    // Neck/eyes 2x MX106R 2xMX28R
    portHandlerHead = dynamixel::PortHandler::getPortHandler(DEVICENAME);
    packetHandlerHead = dynamixel::PortHandler::getPacketHandler(PROTOCOL_VERSION);

    portHandlerPupil = dynamixel::PortHandler::getPortHandler(DEVICENAME);
    packetHandlerPupil = dynamixel::PortHandler::getPacketHandler(PROTOCOL_VERSION);
    
    // threads? ... YES or should we leave it to ikaros???

    // Do we need to communicate in paralell???? probably...

    int dxl_comm_result = COMM_TX_FAIL; // Communication result

    std::vector<uint8_t> vec; // Dynamixel data storages

    // Open port
    if (portHandler->openPort())
    {
        printf("Succeeded to open the port!\n");
    }
    else
    {
        printf("Failed to open the port!\n");
        printf("Press any key to terminate...\n");
        // getch();
        return;
    }

    // Set port baudrate
    if (portHandler->setBaudRate(BAUDRATE))
    {
        printf("Succeeded to change the baudrate!\n");
    }
    else
    {
        printf("Failed to change the baudrate!\n");
        printf("Press any key to terminate...\n");
        // getch();
        return;
    }

    // Try to broadcast ping the Dynamixel
    dxl_comm_result = packetHandler->broadcastPing(portHandler, vec);
    if (dxl_comm_result != COMM_SUCCESS)
        printf("%s\n", packetHandler->getTxRxResult(dxl_comm_result));

    printf("Detected Dynamixel : \n");
    for (int i = 0; i < (int)vec.size(); i++)
    {
        printf("[ID:%03d]\n", vec.at(i));
    }

    // Close port
    portHandler->closePort();

    // 1. Ping all the servos to make sure they are all there.

    // 2. Write defualt settings to servo. This is a good way to make sure we can replace a servo and not worried if all the setting are there.
}

void EpiServo::Tick()
{

    // Read servo.
    // What paramters should we read? (input of module?)

    // Write servo
    // What parameters should we write? (outputs of module)


    //		// Multi core 2

		const int nrOfCores = 4;
		// Create some threads pointers
		thread tConv[nrOfCores];
		//printf("Number of threads %i\n",thread::hardware_concurrency());
		int span = size_x*size_y/nrOfCores;

		for(int j=0; j<nrOfCores; j++)
		{
			tConv[j] = thread([&,j]()
							  {
								  float * r = red;
								  float * g = green;
								  float * b = blue;
								  float * inte = intensity;
								  int t = 0;

								  unsigned char * d = data;

								  for (int i = 0; i < j*span; i++)
								  {
									  d++;
									  d++;
									  d++;
									  r++;
									  g++;
									  b++;
									  inte++;
								  }
								  while(t++ < span)
								  {
									  *r       = convertIntToFloat[*d++];
									  *g       = convertIntToFloat[*d++];
									  *b       = convertIntToFloat[*d++];
									  *inte++ = *r++ + *g++ + *b++;
								  }
							  });

		}
		for(int j=0; j<nrOfCores; j++)
			tConv[j].join();
}

static InitClass init("EpiServo", &EpiServo::Create, "Source/UserModules/EpiServo/");
