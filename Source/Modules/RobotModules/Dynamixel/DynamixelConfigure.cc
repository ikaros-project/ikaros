//
//	DynamixelConfigure.cc
//
//    Copyright (C) 2018  Birger Johansson
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
#include "DynamixelConfigure.h"

using namespace ikaros;

DynamixelConfigure::DynamixelConfigure(Parameter * p):
Module(p)
{
	forceModel = GetIntValue("force_model"); // Overide the read model
	
	device = GetValue("device");
	if(!device)
	{
		Notify(msg_warning, "Dynamixel serial device name is not set.");
		size = 0;
		return;
	}
	std::string csvPath = GetClassPath();
	
	// Reset mode
	resetMode = GetBoolValue("reset_mode");
	if (resetMode)
	{
		printf("In RESET MODE\n");
		printf("No other parameters will be used. Only reset command to one servo will be sent\n");
	}
	
	// Special scan mode
	if (GetBoolValue("scan_mode"))
	{
		printf("DynamixelConfigure is in scan mode. It will scan all possible baud rates for servos.");
		
		int max_servo_id = 254;
		
		if (GetBoolValue("quick_scan"))
			max_servo_id = 20;
		
		// Testing all baudrates available
		for (int i = 0; i < 254; i++)
		{
			switch (i) {
				case 250:
					baud_rate = 2250000;
					break;
				case 251:
					baud_rate = 2500000;
					break;
				case 252:
					baud_rate = 3000000;
					break;
				case 253:
					baud_rate = 3500000;
					//case 254:
					//    baud_rate = 4000000; // guessing
					break;
					
				default:
					baud_rate = 2000000/(float(i)+1);
					break;
			}
			printf("\n%-3i\tScannning baud rate \t %i\n",i, baud_rate);
			com = new DynamixelComm(device, baud_rate);
			if(com)
			{
				size = 0;
				for(int j=0; j<max_servo_id; j++)
				{
					if(com->Ping(j) > 0)
					{
						printf(" %i ",j);
						size += 1;
					}
					else
						printf(".");
				}
				delete com;
			}
		}
		Notify(msg_terminate, "Scan completed. quiting ikaros...");
	}
	
	baud_rate = GetIntValue("baud_rate");
	com = new DynamixelComm(device, baud_rate);
	if(!com)
	{
		Notify(msg_warning, "Dynamixel serial device \"%s\" could not be opened.", device);
		return;
	}
	
	int max_servo_id = GetIntValue("max_servo_id");
	
	servo = new DynamixelServo * [max_servo_id]; // maximum number of servos
	for(int i=0; i<max_servo_id; i++)
		servo[i] = NULL;
	
	// Alt 2: scan for servos
	printf("Scanning for servos. ID: ....");
	size = 0;
	for(int i=1; i<max_servo_id; i++)
	{
		if(com->Ping(i) > 0)
		{
			printf(" %i ",i);
			servo[size] = new DynamixelServo(com, i, csvPath.c_str(),forceModel);
			size += 1;
		}
	}
	printf("\n\n");
	
	nrOfServos = size;
	
	// Allocate servo maps and parameter structures
	servoIndex  = new int [nrOfServos];
	servoId     = new int [nrOfServos];
	
	int j=0;
	for(int i=0; i<size; i++)
		if(servo[i])
		{
			servoId[j] = servo[i]->extraInfo.GetInt("ID");
			servoIndex[j++] = i;
		}
	
	// If we did not find any servos kill ikaros.
	if (size == 0)
		Notify(msg_fatal_error, "Did not find any Dynamixel servos.\n\n");
	
	changeAdress = GetIntValue("adress");
	newValue = GetIntValue("value");
}

void DynamixelConfigure::PrintChange(int i)
{
	printf("\n\n(ID:%i) %s %i -> %i: ", servo[i]->extraInfo.GetInt("ID"), servo[i]->controlTable[changeAdress].Name.c_str(), servo[i]->GetValueAtAdress(changeAdress), newValue);
}

void DynamixelConfigure::SetSizes()
{
	SetOutputSize("RESET_MODE", size);
	SetOutputSize("CHANGE_MODE", size);
}

void DynamixelConfigure::Init()
{
	set	= GetInputArray("SET");
	if (!set)
		Notify(msg_fatal_error, "DynamixelConfigure: Input SET must be connected.");
	
	active	= GetInputArray("ACTIVE");
	if (!active)
		Notify(msg_fatal_error, "DynamixelConfigure: Input ACTIVE must be connected.");
	
	resetModeOut = GetOutputArray("RESET_MODE");
	changeModeOut = GetOutputArray("CHANGE_MODE");
	blink = 0;
	
	// Create memory for binding map. These are specified in the csv file and correspond to where ikaros should look for IO in the dynamixel memory block
	inAdress = new int * [nrOfServos];
	for(int i=0; i<nrOfServos; i++)
		inAdress[i] = new int [IK_INPUTS];
	
	inAdressSize  = new int * [nrOfServos];
	for(int i=0; i<nrOfServos; i++)
		inAdressSize[i] = new int [IK_INPUTS];
	
	for(int i=0; i<nrOfServos; i++)
		for(int j=0; j<IK_INPUTS; j++)
			if(inAdress[i][j]!= -1)
				inAdressSize[i][j] =  servo[servoIndex[i]]->controlTable[inAdress[i][j]].Size;
			else
				inAdressSize[i][j] =  -1;
	
	// Reset array
	for (int i = 0; i < nrOfServos; i++)
		for (int j = 0; j < IK_INPUTS; j++)
			inAdress[i][j] = -1;
	
	// Fill in where to look for data
	for (int i = 0; i < nrOfServos; i++)
		for (int j = 0; j < C_TABLE_SIZE; j++)
		{
			if (servo[servoIndex[i]]->controlTable[j].IkarosInputs >= 0)
				inAdress[i][servo[servoIndex[i]]->controlTable[j].IkarosInputs] = servo[servoIndex[i]]->controlTable[j].Adress;
		}

	// Do not allow mixed protocols
	protocol = -1;
	if (nrOfServos > 0)
		for (int i = 0; i < nrOfServos; i++)
		{
			if (protocol == -1 && servo[servoIndex[i]]->protocol != 0)
				protocol = servo[servoIndex[i]]->protocol; // Set protocol.
			
			if (protocol != servo[servoIndex[i]]->protocol)
				Notify(msg_fatal_error, "Dynamixel uses different protocols. This is not allowed.\n\n");
		}
	
	PrintAll();
}

void DynamixelConfigure::Tick()
{
	Timer timer;
	int selectedServo = int(active[0]);
	
	if (selectedServo < 0)
		selectedServo = 0;
	
	if (selectedServo >= nrOfServos)
		selectedServo = nrOfServos-1;
	
	if (resetMode)
	{
		reset_array(resetModeOut, nrOfServos);
		resetModeOut[selectedServo] = 1;
		
		if (set[0] == 1)
		{
			printf("\n\nDynamixel with ID %i will be restored to factory settings\n\n", servoId[int(active[0])]);
			com->Reset(servoId[selectedServo],servo[int(active[0])]->protocol);
			
			// quit ikaros
			Notify(msg_terminate, "DynamixelConfigure: Settings written to dynamixel(s). Quiting Ikaros...");
		}
	}
	else
	{
		PrintChange(selectedServo);
		reset_array(changeModeOut, nrOfServos);
		changeModeOut[selectedServo] = 1;
		
		if(set[0] == 1) // Write to a single dynamixel ID
		{
			servo[servoIndex[selectedServo]]->SetValueAtAdress(changeAdress, newValue);
			com->WriteToServo(servoId[selectedServo], protocol, changeAdress, &servo[servoIndex[selectedServo]]->dynamixelMemory[changeAdress], servo[servoIndex[selectedServo]]->controlTable[changeAdress].Size);

			timer.Sleep(500); // to make sure we have time to write changes before shuting down ikaros.
			// Shutdown ikaros.
			Notify(msg_terminate, "DynamixelConfigure: Settings written to dynamixel(s). Quiting Ikaros...");
		}
	}
	
	for(int i=0; i<nrOfServos; i++)
		servo[i]->SetValueAtAdress(inAdress[i][IK_IN_LED], 0); // Lights off on all servos
	servo[selectedServo]->SetValueAtAdress(inAdress[selectedServo][IK_IN_LED], blink); // Lights on on selected servo
	for(int i=0; i<nrOfServos; i++)
		com->WriteToServo(servoId[i], protocol, inAdress[i][IK_IN_LED], &servo[servoIndex[i]]->dynamixelMemory[inAdress[i][IK_IN_LED]], inAdressSize[i][IK_IN_LED]);
	blink = 1 - blink; // Blinking
	timer.Sleep(100);
	
}
DynamixelConfigure::~DynamixelConfigure()
{
	Timer timer;
	for(int i=0; i<nrOfServos; i++)
		servo[i]->SetValueAtAdress(inAdress[i][IK_IN_LED], 0); // Lights off on all servos
	for(int i=0; i<nrOfServos; i++)
		com->WriteToServo(servoId[i], protocol, inAdress[i][IK_IN_LED], &servo[servoIndex[i]]->dynamixelMemory[inAdress[i][IK_IN_LED]], inAdressSize[i][IK_IN_LED]);
	
	timer.Sleep(200);
	
	// Free memory
	delete servoIndex;
	delete servoId;
	delete com;
	delete servo; // Also deletes servo's memorybuffer
}

void
DynamixelConfigure::PrintAll()
{
	printf("\nDYNAMIXEL\n");
	printf("Number of servos: %d\n\n", nrOfServos);
	printf("Control table:\n");
	printf("%-8s %-32s|", "Adress", "Name");
	for(int i=0; i<nrOfServos; i++)
		printf(" %6s|",servo[servoIndex[i]]->extraInfo.Get("Servo Model String"));
	printf("\n");
	for (int j = 0; j < 100; j++)
	{
		if (servo[servoIndex[0]]->controlTable[j].Visable) // Use the description from the first servo
		{
			printf("%-8i %-32s|", j, servo[servoIndex[0]]->controlTable[j].Name.c_str());
			for(int i=0; i<nrOfServos; i++)
				printf(" %6i|",servo[servoIndex[i]]->GetValueAtAdress(j));
			printf("\n");
		}
	}
}

static InitClass init("DynamixelConfigure", &DynamixelConfigure::Create, "Source/Modules/RobotModules/Dynamixel/");
