//
//	Dynamixel.cc	This file is a part of the IKAROS project
//
//    Copyright (C) 2018 Birger Johansson
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
// TODO: Code never been used with the new Dynaxmiel Pro servos and only handles 1-2 byte size of paramteres. Also when reading the memory from the servo there is a limimt of 256bytes.

#include "Dynamixel.h"
#include "DynamixelComm.h"

using namespace ikaros;

Dynamixel::Dynamixel(Parameter *p) : Module(p)
{
#ifdef DYNAMIXEL_DEBUG
	printf("Starting Dynamixel\n");
#endif

	device = GetValue("device");
	if (!device)
	{
		Notify(msg_warning, "Dynamixel serial device name is not set.");
		size = 0;
		return;
	}
	com = new DynamixelComm(device, GetIntValue("baud_rate"));
	if (!com)
	{
		Notify(msg_warning, "Dynamixel serial device \"%s\" could not be opened (Check baud rate)", device);
		return;
	}

	std::string csvPath = GetClassPath();
	useFeedback = GetBoolValue("feedback");
	startUpDelay = GetIntValue("start_up_delay");
	torqueUpDelay = GetIntValue("torque_up_delay");
	infoLevel = GetIntValueFromList("print_info");
	indexMode = GetIntValueFromList("index_mode");
	angleUnit = GetIntValueFromList("angle_unit");

	Bind(maxTemperature, "max_temperature");
	Bind(optimizeMode, "optimize");
	Bind(serialLatency, "serial_latency");

	int maxServos = GetIntValue("max_servo_id");
	int servoId_list_size = 0;
	int *servoId_list = GetIntArray("servo_id", servoId_list_size);
	bool strictServoId = GetBoolValue("strict_servo_id");

	servo = new DynamixelServo *[256]; // This array is used to detect ids. Maximum servo id is 253.
	for (int i = 0; i < 256; i++)
		servo[i] = NULL;

#ifdef DYNAMIXEL_DEBUG
	printf("Dynamixel: Init creating a list of Servos\n");
#endif

	// Alt 1: create a list of servos with the IDs read from servoId
	if (servoId_list_size > 0 && indexMode == 0)
	{
		for (int i = 0; i < servoId_list_size; i++)
		{
			if (com->Ping(servoId_list[i]) > 0)
			{
				servo[servoId_list[i]] = new DynamixelServo(com, servoId_list[i], csvPath.c_str(), 0);
			}
			else
			{
				servo[servoId_list[i]] = NULL;
				if (strictServoId)
					Notify(msg_fatal_error, "Dynamixel servo with ID = %d could not be found\n", servoId_list[i]);
				else
					Notify(msg_warning, "Dynamixel servo with ID = %d could not be found\n", servoId_list[i]);
			}
			if (servoId_list[i] > size)
				size = servoId_list[i] + 1;
		}
	}
	else if (servoId_list_size > 0 && indexMode == 1)
	{
		for (int i = 0; i < servoId_list_size; i++)
		{
			if (com->Ping(servoId_list[i]) > 0)
			{
				servo[i] = new DynamixelServo(com, servoId_list[i], csvPath.c_str(), 0);
			}
			else
			{
				servo[i] = NULL;
				if (strictServoId)
					Notify(msg_fatal_error, "Dynamixel servo with ID = %d could not be found\n", servoId_list[i]);
				else
					Notify(msg_warning, "Dynamixel servo with ID = %d could not be found\n", servoId_list[i]);
			}
		}
		size = servoId_list_size;
	}

	// Alt 2: scan for servos
	else if (indexMode == 0)
	{
		size = 0;
		for (int i = 1; i <= maxServos; i++)
		{
			if (com->Ping(i) > 0)
			{
				servo[i] = new DynamixelServo(com, i, csvPath.c_str(), 0);
				size = i + 1;
			}
			else
				servo[i] = NULL;
		}
	}
	else
	{
		size = 0;
		for (int i = 1; i <= maxServos; i++)
		{
			if (com->Ping(i) > 0)
			{
				servo[size] = new DynamixelServo(com, i, csvPath.c_str(), 0);
				size += 1;
			}
		}
	}

	// Adjust size if set as a parameter
	int psize = GetIntValue("size");
	if (psize > 256)
		psize = 256;
	if (psize != 0)
		size = psize;

	// Count servos.
	for (int i = 0; i < size; i++)
		if (servo[i])
			nrOfServos++;

	if (nrOfServos == 0)
	{
		Notify(msg_fatal_error, "Did not find any servos. Check power supply.\n\n");
		return;
	}

	// Allocate servo maps and parameter structures
	servoIndex = new int[nrOfServos];
	servoId = new int[nrOfServos];
	int j = 0;
	for (int i = 0; i < size; i++)
		if (servo[i])
		{
			servoId[j] = servo[i]->extraInfo.GetInt("ID");
			servoIndex[j++] = i;
		}

	// Create memory for binding map. These are specified in the csv file and correspond to where ikaros should look for IO in the dynamixel memory block
	inAdress = new int *[nrOfServos];
	for (int i = 0; i < nrOfServos; i++)
		inAdress[i] = new int[IK_INPUTS];
	inAdressSize = new int *[nrOfServos];
	for (int i = 0; i < nrOfServos; i++)
		inAdressSize[i] = new int[IK_INPUTS];
	outAdress = new int *[nrOfServos];
	for (int i = 0; i < nrOfServos; i++)
		outAdress[i] = new int[IK_OUTPUTS];

	// Reset array
	for (int i = 0; i < nrOfServos; i++)
		for (int j = 0; j < IK_INPUTS; j++)
			inAdress[i][j] = -1;
	for (int i = 0; i < nrOfServos; i++)
		for (int j = 0; j < IK_OUTPUTS; j++)
			outAdress[i][j] = -1;

	// Fill in where to look for data
	for (int i = 0; i < nrOfServos; i++)
		for (int j = 0; j < C_TABLE_SIZE; j++)
		{
			if (servo[servoIndex[i]]->controlTable[j].IkarosInputs >= 0)
				inAdress[i][servo[servoIndex[i]]->controlTable[j].IkarosInputs] = servo[servoIndex[i]]->controlTable[j].Adress;
			if (servo[servoIndex[i]]->controlTable[j].IkarosOutputs >= 0)
				outAdress[i][servo[servoIndex[i]]->controlTable[j].IkarosOutputs] = servo[servoIndex[i]]->controlTable[j].Adress;
		}

	for (int i = 0; i < nrOfServos; i++)
		for (int j = 0; j < IK_INPUTS; j++)
			if (inAdress[i][j] != -1)
				inAdressSize[i][j] = servo[servoIndex[i]]->controlTable[inAdress[i][j]].Size;
			else
				inAdressSize[i][j] = -1;

	// Do not allow mixed protocols
	protocol = -1;
	if (nrOfServos > 0)
		for (int i = 0; i < nrOfServos; i++)
		{
			if (protocol == -1 && servo[servoIndex[i]]->protocol != 0)
				protocol = servo[servoIndex[i]]->protocol; // Set protocol.

			if (protocol != servo[servoIndex[i]]->protocol)
				Notify(msg_fatal_error, "Servos uses different protocols. This is not allowed.\n\n");
		}

	// Create optimize map
	optimize = new bool *[nrOfServos];
	for (int i = 0; i < nrOfServos; i++)
		optimize[i] = new bool[IK_INPUTS];
	for (int i = 0; i < nrOfServos; i++)
		for (int j = 0; j < IK_INPUTS; j++)
			optimize[i][j] = false;

			// Set Goal position to whatever position the servo has now and torque limit to 0
#ifdef DYNAMIXEL_DEBUG
	printf("Setting torque limit to 0 and goalposition to current\n");
#endif
	for (int i = 0; i < nrOfServos; i++)
	{
		servo[servoIndex[i]]->SetValueAtAdress(outAdress[i][IK_OUT_GOAL_POSITION], servo[servoIndex[i]]->GetValueAtAdress(outAdress[i][IK_IN_GOAL_POSITION]));
		com->WriteToServo(servoId[i], protocol, inAdress[i][IK_OUT_GOAL_POSITION], &servo[servoIndex[i]]->dynamixelMemory[inAdress[i][IK_IN_GOAL_POSITION]], inAdressSize[i][IK_IN_GOAL_POSITION]);
		servo[servoIndex[i]]->SetValueAtAdress(outAdress[i][IK_OUT_TORQUE_LIMIT], 0);
		com->WriteToServo(servoId[i], protocol, inAdress[i][IK_OUT_TORQUE_LIMIT], &servo[servoIndex[i]]->dynamixelMemory[inAdress[i][IK_OUT_TORQUE_LIMIT]], inAdressSize[i][IK_OUT_TORQUE_LIMIT]);
	}
}

void Dynamixel::Init()
{
	torqueEnable = GetInputArray("TORQUE_ENABLE");
	LED = GetInputArray("LED");
	dGain = GetInputArray("D_GAIN");
	iGain = GetInputArray("I_GAIN");
	pGain = GetInputArray("P_GAIN");
	goalPosition = GetInputArray("GOAL_POSITION");
	movingSpeed = GetInputArray("MOVING_SPEED");
	torqueLimit = GetInputArray("TORQUE_LIMIT");
	goalTorque = GetInputArray("GOAL_TORQUE");
	goalAcceleration = GetInputArray("GOAL_ACCELERATION");

	// Connected
	connected[IK_IN_TORQUE_ENABLE] = torqueEnable != NULL;
	connected[IK_IN_LED] = LED != NULL;
	connected[IK_IN_D_GAIN] = dGain != NULL;
	connected[IK_IN_I_GAIN] = iGain != NULL;
	connected[IK_IN_P_GAIN] = pGain != NULL;
	connected[IK_IN_GOAL_POSITION] = goalPosition != NULL;
	connected[IK_IN_MOVING_SPEED] = movingSpeed != NULL;
	connected[IK_IN_TORQUE_LIMIT] = torqueLimit != NULL;
	connected[IK_IN_GOAL_TORQUE] = goalTorque != NULL;
	connected[IK_IN_GOAL_ACCELERATION] = goalAcceleration != NULL;

	//	for (int i = 0; i <IK_INPUTS; i++)
	//		connected[i] = false;
	//	if (torqueEnable != NULL)
	//		connected[IK_IN_TORQUE_ENABLE] = true;
	//	if (LED != NULL)
	//		connected[IK_IN_LED] = true;
	//	if (dGain != NULL)
	//		connected[IK_IN_D_GAIN] = true;
	//	if (iGain != NULL)
	//		connected[IK_IN_I_GAIN] = true;
	//	if (pGain != NULL)
	//		connected[IK_IN_P_GAIN] = true;
	//	if (goalPosition != NULL)
	//		connected[IK_IN_GOAL_POSITION] = true;
	//	if (movingSpeed != NULL)
	//		connected[IK_IN_MOVING_SPEED] = true;
	//	if (torqueLimit != NULL)
	//		connected[IK_IN_TORQUE_LIMIT] = true;
	//	if (goalTorque != NULL)
	//		connected[IK_IN_GOAL_TORQUE] = true;
	//	if (goalAcceleration != NULL)
	//		connected[IK_IN_GOAL_ACCELERATION] = true;

	// Create memory active map. Connected and have the parameter means active
	active = new bool *[nrOfServos];
	for (int i = 0; i < nrOfServos; i++)
		active[i] = new bool[IK_INPUTS];

	for (int i = 0; i < nrOfServos; i++)
		for (int j = 0; j < IK_INPUTS; j++)
			active[i][j] = false;

	for (int i = 0; i < nrOfServos; i++)
		for (int j = 0; j < IK_INPUTS; j++)
			if (connected[j] and inAdress[i][j] != -1)
				active[i][j] = true;

	// Check if size is identical to output
	if (connected[IK_IN_TORQUE_ENABLE])
		if (GetInputSize("TORQUE_ENABLE") != size)
			Notify(msg_fatal_error, "Size of input TORQUE_ENABLE is %i must be %i\n", GetInputSize("TORQUE_ENABLE"), size);
	if (connected[IK_IN_LED])
		if (GetInputSize("LED") != size)
			Notify(msg_fatal_error, "Size of input LED is %i must be %i\n", GetInputSize("LED"), size);
	if (connected[IK_IN_D_GAIN])
		if (GetInputSize("D_GAIN") != size)
			Notify(msg_fatal_error, "Size of input G_GAIN is %i must be %i\n", GetInputSize("G_GAIN"), size);
	if (connected[IK_IN_I_GAIN])
		if (GetInputSize("I_GAIN") != size)
			Notify(msg_fatal_error, "Size of input I_GAIN is %i must be %i\n", GetInputSize("I_GAIN"), size);
	if (connected[IK_IN_P_GAIN])
		if (GetInputSize("P_GAIN") != size)
			Notify(msg_fatal_error, "Size of input P_GAIN is %i must be %i\n", GetInputSize("P_GAIN"), size);
	if (connected[IK_IN_GOAL_POSITION])
		if (GetInputSize("GOAL_POSITION") != size)
			Notify(msg_fatal_error, "Size of input GOAL_POSITION is %i must be %i\n", GetInputSize("GOAL_POSITION"), size);
	if (connected[IK_IN_MOVING_SPEED])
		if (GetInputSize("MOVING_SPEED") != size)
			Notify(msg_fatal_error, "Size of input MOVING_SPEED is %i must be %i\n", GetInputSize("MOVING_SPEED"), size);
	if (connected[IK_IN_TORQUE_LIMIT])
		if (GetInputSize("TORQUE_LIMIT") != size)
			Notify(msg_fatal_error, "Size of input TORQUE_LIMIT is %i must be %i\n", GetInputSize("TORQUE_LIMIT"), size);
	if (connected[IK_IN_GOAL_TORQUE])
		if (GetInputSize("GOAL_TORQUE") != size)
			Notify(msg_fatal_error, "Size of input GOAL_TORQUE is %i must be %i\n", GetInputSize("GOAL_TORQUE"), size);
	if (connected[IK_IN_GOAL_ACCELERATION])
		if (GetInputSize("GOAL_ACCELERATION") != size)
			Notify(msg_fatal_error, "Size of input GOAL_ACCELERATION is %i must be %i\n", GetInputSize("GOAL_ACCELERATION"), size);

	// Outputs
	feedbackTorqueEnable = GetOutputArray("FEEDBACK_TORQUE_ENABLE");
	feedbackLED = GetOutputArray("FEEDBACK_LED");
	feedbackDGain = GetOutputArray("FEEDBACK_D_GAIN");
	feedbackIGain = GetOutputArray("FEEDBACK_I_GAIN");
	feedbackPGain = GetOutputArray("FEEDBACK_P_GAIN");
	feedbackGoalPosition = GetOutputArray("FEEDBACK_GOAL_POSITION");
	feedbackMoving = GetOutputArray("FEEDBACK_MOVING_SPEED");
	feedbackTorqueLimit = GetOutputArray("FEEDBACK_TORQUE_LIMIT");
	feedbackPresentPosition = GetOutputArray("FEEDBACK_PRESENT_POSITION");
	feedbackPresentSpeed = GetOutputArray("FEEDBACK_PRESENT_SPEED");
	feedbackPresentLoad = GetOutputArray("FEEDBACK_PRESENT_LOAD");
	feedbackPresentVoltage = GetOutputArray("FEEDBACK_PRESENT_VOLTAGE");
	feedbackPresentTemperature = GetOutputArray("FEEDBACK_PRESENT_TEMPERATURE");
	feedbackPresentCurrent = GetOutputArray("FEEDBACK_PRESENT_CURRENT");
	feedbackGoalTorque = GetOutputArray("FEEDBACK_GOAL_TORQUE");
	feedbackGoalAcceleration = GetOutputArray("FEEDBACK_GOAL_ACCELERATION");
	errors = GetOutputMatrix("ERRORS");
	errorsSizeX = GetOutputSizeX("ERRORS");

	//reset_matrix(errors, nrOfServos, errorsSizeX);

	// Print to console
	if (infoLevel == 1)
		PrintMinimal();
	if (infoLevel == 2)
		PrintAll();

	// Torque limit must be connected
	if (!connected[IK_IN_TORQUE_LIMIT])
		Notify(msg_fatal_error, "Module has no torque limit input. Please connect TORQUE_LIMIT. ");

#ifdef DYNAMIXEL_DEBUG
	PrintMaps();
#endif
}

Dynamixel::~Dynamixel()
{
#ifdef DYNAMIXEL_DEBUG
	printf("Power off servo(s)\n");
#endif
	Timer timer;
	if (infoLevel == 2 || infoLevel == 1)
		printf("\nPower off servo(s)");

	int blink = 0;
	// Turn off torque for all servos. Stepwise to zero
	for (float j = 1.0f; j >= 0; j = j - 0.1)
	{
		for (int i = 0; i < nrOfServos; i++)
		{
			servo[servoIndex[i]]->SetTorqueLimitFormated(outAdress[i][IK_IN_TORQUE_LIMIT], servo[servoIndex[i]]->GetTorqueLimitFormated(outAdress[i][IK_OUT_TORQUE_LIMIT]) * j);
			com->WriteToServo(servoId[i], protocol, inAdress[i][IK_OUT_TORQUE_LIMIT], &servo[servoIndex[i]]->dynamixelMemory[inAdress[i][IK_OUT_TORQUE_LIMIT]], inAdressSize[i][IK_OUT_TORQUE_LIMIT]);
			servo[servoIndex[i]]->SetValueAtAdress(outAdress[i][IK_IN_LED], blink);
			com->WriteToServo(servoId[i], protocol, inAdress[i][IK_IN_LED], &servo[servoIndex[i]]->dynamixelMemory[inAdress[i][IK_IN_LED]], inAdressSize[i][IK_IN_LED]);
		}

		if (infoLevel == 2 || infoLevel == 1)
			printf(".");

		blink = 1 - blink;
		timer.Sleep(100); // Blink
	}
	if (infoLevel == 2 || infoLevel == 1)
		printf("Done\n");

#ifdef DYNAMIXEL_DEBUG
	printf("Turning off LED\n");
#endif
	for (int i = 0; i < nrOfServos; i++)
	{
		servo[servoIndex[i]]->SetValueAtAdress(outAdress[i][IK_IN_LED], 0);
		com->WriteToServo(servoId[i], protocol, inAdress[i][IK_IN_LED], &servo[servoIndex[i]]->dynamixelMemory[inAdress[i][IK_IN_LED]], inAdressSize[i][IK_IN_LED]);
	}
#ifdef DYNAMIXEL_DEBUG
	printf("Setting GoalPosition to current position\n");
#endif
	for (int i = 0; i < nrOfServos; i++)
	{
		com->ReadMemoryRange(servo[servoIndex[i]]->extraInfo.GetInt("ID"), servo[servoIndex[i]]->protocol, servo[servoIndex[i]]->dynamixelMemory, 0, servo[servoIndex[i]]->extraInfo.GetInt("Model Memory"));

		servo[servoIndex[i]]->SetValueAtAdress(outAdress[i][IK_OUT_GOAL_POSITION], servo[servoIndex[i]]->GetValueAtAdress(outAdress[i][IK_OUT_PRESENT_POSITION]));
		com->WriteToServo(servoId[i], protocol, inAdress[i][IK_OUT_GOAL_POSITION], &servo[servoIndex[i]]->dynamixelMemory[inAdress[i][IK_OUT_GOAL_POSITION]], inAdressSize[i][IK_OUT_GOAL_POSITION]);
	}

	timer.Sleep(100); // Sleep to make sure everyting is sent to servo before deleting memory

	// Free memory
	for (int i = 0; i < nrOfServos; i++)
	{
		delete (inAdress[i]);
		delete (inAdressSize[i]);
		delete (outAdress[i]);
		delete (optimize[i]);
		delete (active[i]);
	}
	delete inAdress;
	delete inAdressSize;
	delete outAdress;
	delete optimize;
	delete active;
	delete servoIndex;
	delete servoId;
	delete com;
	delete servo; // Also deletes servo's memorybuffer
}

void Dynamixel::Tick()
{
#ifdef DYNAMIXEL_TIMING
	Timer t;
#endif

	// Update serial latency. Can perhaps be used for tuning.
	com->serialLatency = serialLatency;

	// Reset optimize matrix
	for (int i = 0; i < nrOfServos; i++)
		for (int j = 0; j < IK_INPUTS; j++)
			optimize[i][j] = false;
	if (optimizeMode)
		OptimizeSends();

	// Torque enable feature/bug. If goal position or speed (more?) is sent to the servo, the servo will set tourque enable to 1.
	bool ignore[nrOfServos][IK_INPUTS];
	for (int i = 0; i < nrOfServos; i++)
		for (int j = 0; j < IK_INPUTS; j++)
			ignore[i][j] = false;

	for (int i = 0; i < nrOfServos; i++)
		if (connected[IK_IN_TORQUE_ENABLE])
			if (torqueEnable[servoIndex[i]] == 0)
				ignore[i][IK_IN_GOAL_POSITION] = ignore[i][IK_IN_MOVING_SPEED] = true;

	// Check that the device exists and is open
	if (!device || !com)
		return;

	float torqueMultiplier = 0; // For ramping up torque at start.
#ifdef DYNAMIXEL_DEBUG
	if (GetTick() <= startUpDelay)
		Notify(msg_print, "Startup phase %ld\n", GetTick());
	else if (GetTick() <= startUpDelay + torqueUpDelay)
		Notify(msg_print, "Torque phase %ld\n", GetTick());
	else
		Notify(msg_print, "Normal phase %ld\n", GetTick());
#endif

	if (GetTick() >= startUpDelay) // Do not send any instructions during start_up_delay.
	{
		for (int i = 0; i < nrOfServos; i++)
		{
			if (active[i][IK_IN_TORQUE_ENABLE])
				servo[servoIndex[i]]->SetTorqueEnableFormated(inAdress[i][IK_IN_TORQUE_ENABLE], torqueEnable[servoIndex[i]]);
			if (active[i][IK_IN_LED])
				servo[servoIndex[i]]->SetLEDFormated(inAdress[i][IK_IN_LED], LED[servoIndex[i]]);
			if (active[i][IK_IN_D_GAIN])
				servo[servoIndex[i]]->SetDGainFormated(inAdress[i][IK_IN_D_GAIN], dGain[servoIndex[i]]);
			if (active[i][IK_IN_I_GAIN])
				servo[servoIndex[i]]->SetIGainFormated(inAdress[i][IK_IN_I_GAIN], iGain[servoIndex[i]]);
			if (active[i][IK_IN_P_GAIN])
				servo[servoIndex[i]]->SetPGainFormated(inAdress[i][IK_IN_P_GAIN], pGain[servoIndex[i]]);
			if (active[i][IK_IN_GOAL_POSITION])
				servo[servoIndex[i]]->SetGoalPositionFormated(inAdress[i][IK_IN_GOAL_POSITION], goalPosition[servoIndex[i]], angleUnit);
			if (active[i][IK_IN_MOVING_SPEED])
				servo[servoIndex[i]]->SetMovingSpeedFormated(inAdress[i][IK_IN_MOVING_SPEED], movingSpeed[servoIndex[i]]);

			if (GetTick() <= startUpDelay)
				torqueMultiplier = 0; // No torque during start_up
			else if (GetTick() <= startUpDelay + torqueUpDelay)
				torqueMultiplier = ((GetTick() - startUpDelay) / float(torqueUpDelay)); // Ramping up torque from value servo has now
			else
				torqueMultiplier = 1;

			if (active[i][IK_IN_TORQUE_LIMIT])																								  // Must be connected
				servo[servoIndex[i]]->SetTorqueLimitFormated(inAdress[i][IK_IN_TORQUE_LIMIT], torqueLimit[servoIndex[i]] * torqueMultiplier); // Ramp up torque
			if (active[i][IK_IN_GOAL_TORQUE])
				servo[servoIndex[i]]->SetGoalTorqueFormated(inAdress[i][IK_IN_GOAL_TORQUE], goalTorque[servoIndex[i]]);
			if (active[i][IK_IN_GOAL_ACCELERATION])
				servo[servoIndex[i]]->SetGoalAccelerationFormated(inAdress[i][IK_IN_GOAL_ACCELERATION], goalAcceleration[servoIndex[i]]);
		}

#ifdef DYNAMIXEL_TIMING
		t.Restart();
		com->sendTimer = 0;
#endif
		// MARK: SEND
		if (protocol == 1)
		{
			for (int i = 0; i < nrOfServos; i++)
				for (int j = 0; j < IK_INPUTS; j++)
				{
					// Try to send a block of data to reduce calls
					if (j == IK_IN_GOAL_POSITION and optimizeMode and
						active[i][j] and !optimize[i][j] and !ignore[i][j] and
						active[i][+1] and !optimize[i][j + 1] and !ignore[i][j + 1] and
						active[i][j + 2] and !optimize[i][j + 2] and !ignore[i][j + 2])

					{
						int blockSize = inAdressSize[i][IK_IN_GOAL_POSITION] + inAdressSize[i][IK_IN_MOVING_SPEED] + inAdressSize[i][IK_IN_TORQUE_LIMIT];
						com->AddDataSyncWrite1(servoId[i], inAdress[i][IK_IN_GOAL_POSITION], &servo[servoIndex[i]]->dynamixelMemory[inAdress[i][IK_IN_GOAL_POSITION]], blockSize);
						com->SendSyncWrite1();
						j = j + 3; // Skipping goal position, moving speed and torqueLimit
					}

					if (active[i][j] and !optimize[i][j] and !ignore[i][j])
						com->AddDataSyncWrite1(servoId[i], inAdress[i][j], &servo[servoIndex[i]]->dynamixelMemory[inAdress[i][j]], inAdressSize[i][j]);
					com->SendSyncWrite1();
				}
		}
		if (protocol == 2)
		{
			// Version 2
			// Using bulkwrite in the same way as syncwrite. A limitation of bulkwrite is that a servo id is only allowed once for each call. By using bulkwrite the adress and sizes may change with dynamixel versions. This has not been fully tested.
			// This can be optimzed more.
			for (int i = 0; i < nrOfServos; i++)
				for (int j = 0; j < IK_INPUTS; j++)
				{
					if (active[i][j] and !optimize[i][j] and !ignore[i][j])
						com->AddDataBulkWrite2(servoId[i], inAdress[i][j], &servo[servoIndex[i]]->dynamixelMemory[inAdress[i][j]], inAdressSize[i][j]);
					com->SendBulkWrite2();
				}
		}
	}
#ifdef DYNAMIXEL_TIMING
	t.Restart();
	com->recieveTimer = 0;
#endif

	// Get feedback
	for (int i = 0; i < nrOfServos; i++)
	{
		if (useFeedback)
		{
			com->ReadMemoryRange(servo[servoIndex[i]]->extraInfo.GetInt("ID"), servo[servoIndex[i]]->protocol, servo[servoIndex[i]]->dynamixelMemory, 0, servo[servoIndex[i]]->extraInfo.GetInt("Model Memory"));
			GetErrors(servoIndex[i]);
		}

#ifdef DYNAMIXEL_TIMING
		com->recieveTimer = t.GetTime() - com->recieveTimer;
#endif
		if (outAdress[i][IK_OUT_TORQUE_ENABLE] != -1)
			feedbackTorqueEnable[servoIndex[i]] = servo[servoIndex[i]]->GetTorqueEnableFormated(outAdress[i][IK_OUT_TORQUE_ENABLE]);
		if (outAdress[i][IK_OUT_LED] != -1)
			feedbackLED[servoIndex[i]] = servo[servoIndex[i]]->GetLEDFormated(outAdress[i][IK_OUT_LED]);
		if (outAdress[i][IK_OUT_D_GAIN] != -1)
			feedbackDGain[servoIndex[i]] = servo[servoIndex[i]]->GetDGainFormated(outAdress[i][IK_OUT_D_GAIN]);
		if (outAdress[i][IK_OUT_I_GAIN] != -1)
			feedbackIGain[servoIndex[i]] = servo[servoIndex[i]]->GetIGainFormated(outAdress[i][IK_OUT_I_GAIN]);
		if (outAdress[i][IK_OUT_P_GAIN] != -1)
			feedbackPGain[servoIndex[i]] = servo[servoIndex[i]]->GetPGainFormated(outAdress[i][IK_OUT_P_GAIN]);
		if (outAdress[i][IK_OUT_GOAL_POSITION] != -1)
			feedbackGoalPosition[servoIndex[i]] = servo[servoIndex[i]]->GetGoalPositionFormated(outAdress[i][IK_OUT_GOAL_POSITION], angleUnit);
		if (outAdress[i][IK_OUT_MOVING_SPEED] != -1)
			feedbackMoving[servoIndex[i]] = servo[servoIndex[i]]->GetMovingSpeedFormated(outAdress[i][IK_OUT_MOVING_SPEED]);
		if (outAdress[i][IK_OUT_TORQUE_LIMIT] != -1)
			feedbackTorqueLimit[servoIndex[i]] = servo[servoIndex[i]]->GetTorqueLimitFormated(outAdress[i][IK_OUT_TORQUE_LIMIT]);
		if (outAdress[i][IK_OUT_PRESENT_POSITION] != -1)
			feedbackPresentPosition[servoIndex[i]] = servo[servoIndex[i]]->GetPresentPositionFormated(outAdress[i][IK_OUT_PRESENT_POSITION], angleUnit);
		if (outAdress[i][IK_OUT_PRESENT_SPEED] != -1)
			feedbackPresentSpeed[servoIndex[i]] = servo[servoIndex[i]]->GetPresentSpeedFormated(outAdress[i][IK_OUT_PRESENT_SPEED]);
		if (outAdress[i][IK_OUT_PRESENT_LOAD] != -1)
			feedbackPresentLoad[servoIndex[i]] = servo[servoIndex[i]]->GetPresentLoadFormated(outAdress[i][IK_OUT_PRESENT_LOAD]);
		if (outAdress[i][IK_OUT_PRESENT_VOLTAGE] != -1)
			feedbackPresentVoltage[servoIndex[i]] = servo[servoIndex[i]]->GetPresentVoltageFormated(outAdress[i][IK_OUT_PRESENT_VOLTAGE]);
		if (outAdress[i][IK_OUT_PRESENT_TEMPERATURE] != -1)
			feedbackPresentTemperature[servoIndex[i]] = servo[servoIndex[i]]->GetPresentTemperatureFormated(outAdress[i][IK_OUT_PRESENT_TEMPERATURE]);
		if (outAdress[i][IK_OUT_PRESENT_CURRENT] != -1)
			feedbackPresentCurrent[servoIndex[i]] = servo[servoIndex[i]]->GetCurrentFormated(outAdress[i][IK_OUT_PRESENT_CURRENT]);
		if (outAdress[i][IK_OUT_GOAL_TORQUE] != -1)
			feedbackGoalTorque[servoIndex[i]] = servo[servoIndex[i]]->GetGoalTorqueFormated(outAdress[i][IK_OUT_GOAL_TORQUE]);
		if (outAdress[i][IK_OUT_GOAL_ACCELERATION] != -1)
			feedbackGoalAcceleration[servoIndex[i]] = servo[servoIndex[i]]->GetGoalAccelerationFormated(outAdress[i][IK_OUT_GOAL_ACCELERATION]);
	}
	// Check temp
	for (int i = 0; i < nrOfServos; i++)
		if (servo[servoIndex[i]]->GetPresentTemperatureFormated(outAdress[i][IK_OUT_PRESENT_TEMPERATURE]) > maxTemperature)
		{
			PrintAll();
			Notify(msg_fatal_error, "Servo temperature is over limit. Shuting down ikaros\n");
		}
#ifdef DYNAMIXEL_TIMING
	printf("Timers Send:%f\t Recv:%f\n", com->sendTimer, com->recieveTimer);
#endif
}

void Dynamixel::GetErrors(int index)
{
	// Communication errors
	errors[index][0] += float(com->missingBytesError);
	errors[index][1] += float(com->crcError);
	errors[index][2] += float(com->extendedError);
	errors[index][3] += float(com->notCompleteError);

	// Get servo errors
	if (protocol == 1)
	{
		errors[index][4] += com->errorServoIntruction;
		errors[index][5] += com->errorServoOverload;
		errors[index][6] += com->errorServoChecksum;
		errors[index][7] += com->errorServoRange;
		errors[index][8] += com->errorServoOverHeating;
		errors[index][9] += com->errorServoAngleLimit;
		errors[index][10] += com->errorServoInputVoltage;
	}
	else if (protocol == 2)
	{
		errors[index][11] += com->errorServo2;
		errors[index][12] += com->errorServoResaultFail2;
		errors[index][13] += com->errorServoIntruction2;
		errors[index][14] += com->errorServoCrc2;
		errors[index][15] += com->errorServoRange2;
		errors[index][16] += com->errorServoLength2;
		errors[index][17] += com->errorServoLimit2;
		errors[index][18] += com->errorServoAccess2;
	}
	// printf("%i %i\n",nrOfServos,errorsSizeX);
	ResetComErrors();
	// for (int i = 0; i < errorsSizeX; i++)
	// 		for (int j = 0; j < nrOfServos; j++)
	// 			errors[j][i] = j;

	// print_matrix("E",errors,errorsSizeX,nrOfServos);

}
void Dynamixel::ResetComErrors()
{
		com->missingBytesError 
		= com->crcError 
		= com->extendedError 
		= com->notCompleteError
		= com->errorServoIntruction
		= com->errorServoOverload
		= com->errorServoChecksum
		= com->errorServoRange
		= com->errorServoOverHeating
		= com->errorServoAngleLimit
		=  com->errorServoInputVoltage
		=  com->errorServo2
		=  com->errorServoResaultFail2
		=  com->errorServoIntruction2
		=  com->errorServoCrc2
		=  com->errorServoRange2
		=  com->errorServoLength2
		=  com->errorServoLimit2
		=  com->errorServoAccess2
		= 0;
}

void Dynamixel::PrintMinimal()
{
	printf("\nDYNAMIXEL:\n");
	printf("Number of servos: %d\n\n", nrOfServos);
	printf("%6s %5s\n", "ID", "Name");
	for (int i = 0; i < nrOfServos; i++)
	{
		printf("%-4i", i);
		printf("%-4i", servo[servoIndex[i]]->extraInfo.GetInt("ID"));
		printf("%-6s", servo[servoIndex[i]]->extraInfo.Get("Servo Model String"));
		printf("\n");
	}
}

void Dynamixel::PrintAll()
{
	printf("\nDYNAMIXEL:\n");
	printf("Number of servos: %d\n\n", nrOfServos);
	if (nrOfServos != 0)
	{
		printf("Control table:\n");
		printf("%-8s %-32s|", "Adress", "Name");
		for (int i = 0; i < nrOfServos; i++)
			printf(" %6s|", servo[servoIndex[i]]->extraInfo.Get("Servo Model String"));
		printf("\n");
		for (int j = 0; j < 100; j++)
		{
			if (servo[servoIndex[0]]->controlTable[j].Visable) // Use the description from the first servo
			{
				printf("%-8i %-32s|", j, servo[servoIndex[0]]->controlTable[j].Name.c_str());
				for (int i = 0; i < nrOfServos; i++)
					if (servo[servoIndex[i]]->GetValueAtAdress(j) == -1)
						printf("      -|");
					else
						printf(" %6i|", servo[servoIndex[i]]->GetValueAtAdress(j));
				printf("\n");
			}
		}
	}
}
void Dynamixel::OptimizeSends()
{
	// Skipping sending if the servo has the same value as we trying to
	// The input value is checked with the recived value from previous tick. This could be a problem if the input is switching between two values. Could be fixed by comparing also the previous send message.
	for (int i = 0; i < nrOfServos; i++)
		for (int j = 0; j < IK_INPUTS; j++)
			if (active[i][j])
				switch (j)
				{
				case IK_IN_TORQUE_ENABLE:
					optimize[i][j] = servo[servoIndex[i]]->GetTorqueEnableFormated(inAdress[i][j]) == torqueEnable[servoIndex[i]];
					break;
				case IK_IN_LED:
					optimize[i][j] = servo[servoIndex[i]]->GetLEDFormated(inAdress[i][j]) == LED[servoIndex[i]];
					break;
				case IK_IN_D_GAIN:
					optimize[i][j] = servo[servoIndex[i]]->GetDGainFormated(inAdress[i][j]) == dGain[servoIndex[i]];
					break;
				case IK_IN_I_GAIN:
					optimize[i][j] = servo[servoIndex[i]]->GetIGainFormated(inAdress[i][j]) == iGain[servoIndex[i]];
					break;
				case IK_IN_P_GAIN:
					optimize[i][j] = servo[servoIndex[i]]->GetPGainFormated(inAdress[i][j]) == pGain[servoIndex[i]];
					break;
				case IK_IN_GOAL_POSITION:
					optimize[i][j] = servo[servoIndex[i]]->GetGoalPositionFormated(inAdress[i][j], angleUnit) == goalPosition[servoIndex[i]];
					break;
				case IK_IN_MOVING_SPEED:
					optimize[i][j] = servo[servoIndex[i]]->GetMovingSpeedFormated(inAdress[i][j]) == movingSpeed[servoIndex[i]];
					break;
				case IK_IN_TORQUE_LIMIT:
					optimize[i][j] = servo[servoIndex[i]]->GetTorqueLimitFormated(inAdress[i][j]) == torqueLimit[servoIndex[i]];
					break;
				case IK_IN_GOAL_TORQUE:
					optimize[i][j] = servo[servoIndex[i]]->GetGoalTorqueFormated(inAdress[i][j]) == goalTorque[servoIndex[i]];
					break;
				case IK_IN_GOAL_ACCELERATION:
					optimize[i][j] = servo[servoIndex[i]]->GetGoalAccelerationFormated(inAdress[i][j]) == goalAcceleration[servoIndex[i]];
					break;
				default:
					break;
				}
}

void Dynamixel::PrintMaps()
{
	printf("\nPrintMaps %ld\n", GetTick());
	printf("Map Input\n");
	for (int i = 0; i < nrOfServos; i++)
		for (int j = 0; j < IK_INPUTS; j++)
			printf("%i:%i : %i\t", i, j, inAdress[i][j]);
	printf("\n");
	printf("Input size\n");
	for (int i = 0; i < nrOfServos; i++)
		for (int j = 0; j < IK_INPUTS; j++)
			printf("%i:%i : %i\t\t", i, j, inAdressSize[i][j]);
	printf("\n");
	printf("Active \n");
	for (int i = 0; i < nrOfServos; i++)
		for (int j = 0; j < IK_INPUTS; j++)
			printf("%i:%i : %i\t\t", i, j, active[i][j]);
	printf("\n");
	printf("Map Output\n");
	for (int i = 0; i < nrOfServos; i++)
		for (int j = 0; j < IK_OUTPUTS; j++)
			printf("%i:%i : %i\t", i, j, outAdress[i][j]);
	printf("\n");
	printf("Optimize\n");
	for (int i = 0; i < nrOfServos; i++)
		for (int j = 0; j < IK_INPUTS; j++)
			printf("%i:%i : %i\t", i, j, optimize[i][j]);
	printf("\n");
}
static InitClass init("Dynamixel", &Dynamixel::Create, "Source/Modules/RobotModules/Dynamixel/");
