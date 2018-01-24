//
//	Dynamixel.cc	This file is a part of the IKAROS project
//
//    Copyright (C) 2016 Birger Johansson
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

Dynamixel::Dynamixel(Parameter * p):
Module(p)
{
	device = GetValue("device");
	if(!device)
	{
		Notify(msg_warning, "Dynamixel serial device name is not set.");
		size = 0;
		return;
	}
	com = new DynamixelComm(device, GetIntValue("baud_rate"));
	if(!com)
	{
		Notify(msg_warning, "Dynamixel serial device \"%s\" could not be opened (Check baud rate)", device);
		return;
	}

	
	std::string csvPath     = GetClassPath();
	use_feedback            = GetBoolValue("feedback");
	start_up_delay          = GetIntValue("start_up_delay");
	torque_up_delay         = GetIntValue("torque_up_delay");
	init_print              = GetIntValueFromList("print_info");
	index_mode              = GetIntValueFromList("index_mode");
	angle_unit              = GetIntValueFromList("angle_unit");
	max_temperature         = GetIntValue("max_temperature");
	Bind(optimize_mode, "optimize");
	
	int maxServos           = GetIntValue("max_servo_id");
	int servoId_list_size  	= 0;
	int * servoId_list     	= GetIntArray("servo_id", servoId_list_size);
	bool strictServoId 		= GetBoolValue("strict_servo_id");
	
	servo = new DynamixelServo * [256]; // This array is used to detect ids. Maximum servo id is 253.
	for(int i=0; i<256; i++)
		servo[i] = NULL;
	
#ifdef DYNAMIXEL_DEBUG
	printf("Dynamixel: Init creating a list of Servos\n");
#endif
	
	// Alt 1: create a list of servos with the IDs read from servoId
	if(servoId_list_size > 0 && index_mode == 0)
	{
		for(int i=0; i<servoId_list_size; i++)
		{
			if(com->Ping(servoId_list[i]) > 0)
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
			if(servoId_list[i] > size)
				size = servoId_list[i]+1;
		}
	}
	else if(servoId_list_size > 0 && index_mode == 1)
	{
		for(int i=0; i<servoId_list_size; i++)
		{
			if(com->Ping(servoId_list[i]) > 0)
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
	else if(index_mode == 0)
	{
		size = 0;
		for(int i=1; i<=maxServos; i++)
		{
			if(com->Ping(i) > 0)
			{
				servo[i] = new DynamixelServo(com, i, csvPath.c_str(),0);
				size = i+1;
			}
			else
				servo[i] = NULL;
		}
	}
	else
	{
		size = 0;
		for(int i=1; i<=maxServos; i++)
		{
			if(com->Ping(i) > 0)
			{
				servo[size] = new DynamixelServo(com, i, csvPath.c_str(),0);
				size += 1;
			}
		}
	}
	
	// Adjust size if set as a parameter
	int psize = GetIntValue("size");
	if(psize > 256)
		psize = 256;
	if(psize != 0)
		size = psize;
	
	// Count servos.
	for(int i=0; i<size; i++)
		if(servo[i])
			nrOfServos++;
	
	if (nrOfServos == 0){
		Notify(msg_fatal_error, "Did not find any servoes.\n\n");
		return;
	}
	
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
	
	
	
	
	// Creating pointers to the servos memory block
	
	// The Module has direct referens to each servoes memory. This is because we send package with multiple ids and grab info from many servoes at ones
	//DynamixelMemoeries = new unsigned char * [nrOfServos];
	//for(int i=0; i<nrOfServos; i++)
	//	DynamixelMemoeries[i] = servo[servoIndex[i]]->dynamixelMemory;
	
	// OBS ORDER OF MATRIXES IS [Paramter][ServoID]
	// Create memory for binding map. These are specified in the csv file and correspond to where ikaros should look for IO in the dynamixel memory block
	ikarosInBind = new int * [IK_INPUTS];
	for(int i=0; i<IK_INPUTS; i++)
		ikarosInBind[i] = new int [nrOfServos];
	
	ikarosOutBind = new int * [IK_OUTPUTS];
	for(int i=0; i<IK_OUTPUTS; i++)
		ikarosOutBind[i] = new int [nrOfServos];
	
	// Reset array
	for (int i = 0; i < IK_INPUTS; i++)
		for (int j = 0; j < nrOfServos; j++)
			ikarosInBind[i][j] = -1;
	for (int i = 0; i < IK_OUTPUTS; i++)
		for (int j = 0; j < nrOfServos; j++)
			ikarosOutBind[i][j] = -1;
	
	// Fill in where to look for data
	for (int i = 0; i < nrOfServos; i++)
		for (int j = 0; j < C_TABLE_SIZE; j++)
		{
			if (servo[servoIndex[i]]->controlTable[j].IkarosInputs >= 0)
				ikarosInBind[servo[servoIndex[i]]->controlTable[j].IkarosInputs][i] = servo[servoIndex[i]]->controlTable[j].Adress;
			if (servo[servoIndex[i]]->controlTable[j].IkarosOutputs >= 0)
				ikarosOutBind[servo[servoIndex[i]]->controlTable[j].IkarosOutputs][i] = servo[servoIndex[i]]->controlTable[j].Adress;
		}
//	printf("Bindings IN\n");
//	for (int j = 0; j < IK_INPUTS; j++)
//	{
//		for (int k = 0; k < nrOfServos; k++)
//			printf("%i:%i : %i\t",k ,j, ikarosInBind[j][k]) ;
//		printf("\n");
//	}
	//	printf("Bindings OUT\n");
	//	for (int j = 0; j < IK_OUTPUTS; j++)
	//	{
	//		for (int k = 0; k < nrOfServos; k++)
	//			printf("%i:%i : %i\t",k ,j, ikarosOutBind[j][k]) ;
	//		printf("\n");
	//	}
	
	// OBS ORDER OF MATRIXES IS [inputs][Servo]
	// Create memory size map
	parameterInSize = new int * [IK_INPUTS];
	for(int i=0; i<IK_INPUTS; i++)
		parameterInSize[i] = new int [nrOfServos];
	
	for(int i=0; i<IK_INPUTS; i++)
		for(int j=0; j<nrOfServos; j++)
			if(ikarosInBind[i][j]!= -1)
				parameterInSize[i][j] =  servo[servoIndex[j]]->controlTable[ikarosInBind[i][j]].Size;
			else
				parameterInSize[i][j] =  -1;
	
	//	printf("Parameter size\n");
	//	for(int j=0; j<IK_INPUTS; j++)
	//	{
	//		for(int i=0; i<nrOfServos; i++)
	//			printf("%i:%i=%i\t",i ,j, parameterInSize[j][i]) ;
	//		printf("\n");
	//	}
	
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
	
	mask        = new int [nrOfServos];
	for (int i = 0; i<nrOfServos; i++)
		mask[i] = 1;
	
	// Create optimize map
	optimize = new bool * [nrOfServos];
	for(int i=0; i<nrOfServos; i++)
		optimize[i] = new bool [IK_INPUTS];
	
	for(int i=0; i<nrOfServos; i++)
		for(int j=0; j<IK_INPUTS; j++)
			optimize[i][j] = false;
}
//void
//Dynamixel::SetSizes()
//{
//	SetOutputSize("FEEDBACK_TORQUE_ENABLE", size);
//	SetOutputSize("FEEDBACK_LED", size);
//	SetOutputSize("FEEDBACK_D_GAIN", size);
//	SetOutputSize("FEEDBACK_I_GAIN", size);
//	SetOutputSize("FEEDBACK_P_GAIN", size);
//	SetOutputSize("FEEDBACK_GOAL_POSITION", size);
//	SetOutputSize("FEEDBACK_MOVING_SPEED", size);
//	SetOutputSize("FEEDBACK_TORQUE_LIMIT", size);
//	SetOutputSize("FEEDBACK_PRESENT_POSITION", size);
//	SetOutputSize("FEEDBACK_PRESENT_SPEED", size);
//	SetOutputSize("FEEDBACK_PRESENT_LOAD", size);
//	SetOutputSize("FEEDBACK_PRESENT_VOLTAGE", size);
//	SetOutputSize("FEEDBACK_PRESENT_TEMPERATURE", size);
//	SetOutputSize("FEEDBACK_PRESENT_CURRENT", size);
//	SetOutputSize("FEEDBACK_GOAL_TORQUE", size);
//	SetOutputSize("FEEDBACK_GOAL_ACCELERATION", size);
//	
//}

void
Dynamixel::Init()
{
	
	torqueEnable        = GetInputArray("TORQUE_ENABLE");
	LED                 = GetInputArray("LED");
	dGain               = GetInputArray("D_GAIN");
	iGain               = GetInputArray("I_GAIN");
	pGain               = GetInputArray("P_GAIN");
	goalPosition        = GetInputArray("GOAL_POSITION");
	movingSpeed         = GetInputArray("MOVING_SPEED");
	torqueLimit         = GetInputArray("TORQUE_LIMIT");
	goalTorque          = GetInputArray("GOAL_TORQUE");
	goalAcceleration	= GetInputArray("GOAL_ACCELERATION");
	
	// Connected
	for (int i = 0; i <IK_INPUTS; i++)
		connected[i] = false;
	
	if (torqueEnable != NULL)
		connected[IK_IN_TORQUE_ENABLE] = true;
	if (LED != NULL)
		connected[IK_IN_LED] = true;
	if (dGain != NULL)
		connected[IK_IN_D_GAIN] = true;
	if (iGain != NULL)
		connected[IK_IN_I_GAIN] = true;
	if (pGain != NULL)
		connected[IK_IN_P_GAIN] = true;
	if (goalPosition != NULL)
		connected[IK_IN_GOAL_POSITION] = true;
	if (movingSpeed != NULL)
		connected[IK_IN_MOVING_SPEED] = true;
	if (torqueLimit != NULL)
		connected[IK_IN_TORQUE_LIMIT] = true;
	if (goalTorque != NULL)
		connected[IK_IN_GOAL_TORQUE] = true;
	if (goalAcceleration != NULL)
		connected[IK_IN_GOAL_ACCELERATION] = true;
	
//	for (int i = 0; i <IK_INPUTS; i++)
//		printf("%i = %i\n",i, connected[i]);
	
	// Create memory active map. Connected and have the parameter gives active
	active = new bool * [nrOfServos];
	for(int i=0; i<nrOfServos; i++)
		active[i] = new bool [IK_INPUTS];
	
	for(int i=0; i<nrOfServos; i++)
		for(int j=0; j<IK_INPUTS; j++)
			if(connected[j] and ikarosInBind[j][i] != -1)
				active[i][j] = true;
			else
				active[i][j] = false;
	
	//	printf("Active\n");
	//	for(int i=0; i<nrOfServos; i++)
	//		for(int j=0; j<IK_INPUTS; j++)
	//				printf("%i:%i=%i\n",i ,j, active[i][j]) ;
	//			printf("\n");
	
	
	
	
	// Inputs
	torqueEnableConnected      = torqueEnable;
	LEDConnected               = LED;
	dGainConnected             = dGain;
	iGainConnected             = iGain;
	pGainConnected             = pGain;
	goalPositionConnected      = goalPosition;
	movingSpeedConnected       = movingSpeed;
	torqueLimitConnected       = torqueLimit;
	goalTorqueConnected        = goalTorque;
	goalAccelerationConnected  = goalAcceleration;
	
	// Mark all connected inputs and check if size is identical to output
	if (torqueEnable)
	{
		torqueEnableConnected = true;
		if (GetInputSize("TORQUE_ENABLE") < size)
			Notify(msg_fatal_error, "Size of input TORQUE_ENABLE is %i must be %i\n",GetInputSize("TORQUE_ENABLE"), size);
	}
	if (LED)
	{
		LEDConnected = true;
		if (GetInputSize("LED") < size)
			Notify(msg_fatal_error, "Size of input LED is %i must be %i\n",GetInputSize("LED"), size);
	}
	if (dGain)
	{
		dGainConnected = true;
		if (GetInputSize("D_GAIN") < size)
			Notify(msg_fatal_error, "Size of input G_GAIN is %i must be %i\n",GetInputSize("G_GAIN"), size);
	}
	if (iGain)
	{
		iGainConnected = true;
		if (GetInputSize("I_GAIN") < size)
			Notify(msg_fatal_error, "Size of input I_GAIN is %i must be %i\n",GetInputSize("I_GAIN"), size);
	}
	if (pGain)
	{
		pGainConnected = true;
		if (GetInputSize("P_GAIN") < size)
			Notify(msg_fatal_error, "Size of input P_GAIN is %i must be %i\n",GetInputSize("P_GAIN"), size);
	}
	if (goalPosition)
	{
		goalPositionConnected = true;
		if (GetInputSize("GOAL_POSITION") < size)
			Notify(msg_fatal_error, "Size of input GOAL_POSITION is %i must be %i\n",GetInputSize("GOAL_POSITION"), size);
	}
	if (movingSpeed)
	{
		movingSpeedConnected = true;
		if (GetInputSize("MOVING_SPEED") < size)
			Notify(msg_fatal_error, "Size of input MOVING_SPEED is %i must be %i\n",GetInputSize("MOVING_SPEED"), size);
	}
	if (torqueLimit)
	{
		torqueLimitConnected = true;
		if (GetInputSize("TORQUE_LIMIT") < size)
			Notify(msg_fatal_error, "Size of input TORQUE_LIMIT is %i must be %i\n",GetInputSize("TORQUE_LIMIT"), size);
	}
	if (goalTorque)
	{
		goalTorqueConnected = true;
		if (GetInputSize("GOAL_TORQUE") < size)
			Notify(msg_fatal_error, "Size of input GOAL_TORQUE is %i must be %i\n",GetInputSize("GOAL_TORQUE"), size);
	}
	if (goalAcceleration)
	{
		goalAccelerationConnected = true;
		if (GetInputSize("GOAL_ACCELERATION") < size)
			Notify(msg_fatal_error, "Size of input GOAL_ACCELERATION is %i must be %i\n",GetInputSize("GOAL_ACCELERATION"), size);
	}
	
	
	
	// Outputs
	feedbackTorqueEnable    	= GetOutputArray("FEEDBACK_TORQUE_ENABLE");
	feedbackLED    				= GetOutputArray("FEEDBACK_LED");
	feedbackDGain				= GetOutputArray("FEEDBACK_D_GAIN");
	feedbackIGain    			= GetOutputArray("FEEDBACK_I_GAIN");
	feedbackPGain  				= GetOutputArray("FEEDBACK_P_GAIN");
	feedbackGoalPosition    	= GetOutputArray("FEEDBACK_GOAL_POSITION");
	feedbackMoving              = GetOutputArray("FEEDBACK_MOVING_SPEED");
	feedbackTorqueLimit    		= GetOutputArray("FEEDBACK_TORQUE_LIMIT");
	feedbackPresentPosition		= GetOutputArray("FEEDBACK_PRESENT_POSITION");
	feedbackPresentSpeed		= GetOutputArray("FEEDBACK_PRESENT_SPEED");
	feedbackPresentLoad			= GetOutputArray("FEEDBACK_PRESENT_LOAD");
	feedbackPresentVoltage		= GetOutputArray("FEEDBACK_PRESENT_VOLTAGE");
	feedbackPresentTemperature	= GetOutputArray("FEEDBACK_PRESENT_TEMPERATURE");
	feedbackPresentCurrent		= GetOutputArray("FEEDBACK_PRESENT_CURRENT");
	feedbackGoalTorque          = GetOutputArray("FEEDBACK_GOAL_TORQUE");
	feedbackGoalAcceleration	= GetOutputArray("FEEDBACK_GOAL_ACCELERATION");
	
	// Print to console
	if (init_print == 1)
		Print();
	if (init_print == 2)
		PrintAll();
	
	// Set Goal position to whatever position the servo has now.
	// TODO: Check what happens if a servo is out of the angles it can move. EX106 at 0 degrees.
	for(int i=0; i<nrOfServos; i++)
	{
		servo[servoIndex[i]]->SetValueAtAdress(ikarosOutBind[IK_OUT_GOAL_POSITION][i], servo[servoIndex[i]]->GetValueAtAdress(ikarosOutBind[IK_IN_GOAL_POSITION][i]));
		com->AddDataSyncWrite1(servoId[i], ikarosInBind[IK_OUT_GOAL_POSITION][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_GOAL_POSITION][i]], parameterInSize[IK_IN_GOAL_POSITION][i]);
	}
	com->SendSyncWrite1();
	
	
	// Warning if there will be no torque
	if (!torqueLimitConnected)
		Notify(msg_fatal_error, "Module has no torque limit input. Please connect TORQUE_LIMIT. ");
	
}

Dynamixel::~Dynamixel()
{
#ifdef DYNAMIXEL_DEBUG
	printf("Shuting down\n");
#endif
	
	if(torqueLimitConnected)
	{
#ifdef DYNAMIXEL_DEBUG
		printf("Power off servo\n");
#endif
		Timer timer;
		if (init_print == 2 || init_print == 1)
			printf("\nPower off servo(s)");
		
		int blink = 0;
		// Turn off torque for all servos. Stepwise to zero
		for(float j=1.0f; j>=0; j = j - 0.1)
		{
			
			
			//			for(int i=0; i<nrOfServos; i++)
			//			{
			//				servo[servoIndex[i]]->SetTorqueLimitFormated(ikarosOutBind[IK_IN_TORQUE_LIMIT][i], servo[servoIndex[i]]->GetTorqueLimitFormated(ikarosOutBind[IK_OUT_TORQUE_LIMIT][i])*j);
			//			}
			//			com->WriteToServo(servoId, mask, protocol ,DynamixelMemoeries, ikarosInBind[IK_IN_TORQUE_LIMIT], parameterInSize[IK_IN_TORQUE_LIMIT], nrOfServos);
			//			com->WriteToServo(servoId, mask, protocol, DynamixelMemoeries, ikarosOutBind[IK_IN_LED], parameterInSize[IK_IN_LED], nrOfServos);
			//
			for(int i=0; i<nrOfServos; i++)
			{
				servo[servoIndex[i]]->SetTorqueLimitFormated(ikarosOutBind[IK_IN_TORQUE_LIMIT][i], servo[servoIndex[i]]->GetTorqueLimitFormated(ikarosOutBind[IK_OUT_TORQUE_LIMIT][i])*j);
				com->AddDataSyncWrite1(servoId[i], ikarosInBind[IK_OUT_TORQUE_LIMIT][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_TORQUE_LIMIT][i]], parameterInSize[IK_IN_TORQUE_LIMIT][i]);
			}
			com->SendSyncWrite1();
			
			for(int i=0; i<nrOfServos; i++)
			{
				servo[servoIndex[i]]->SetValueAtAdress(ikarosOutBind[IK_IN_LED][i], blink);
				com->AddDataSyncWrite1(servoId[i], ikarosInBind[IK_IN_LED][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_LED][i]], parameterInSize[IK_IN_LED][i]);
			}
			com->SendSyncWrite1();
			
			if (init_print == 2 || init_print == 1)
				printf(".");
			
			if (blink == 1)
				blink = 0;
			else
				blink = 1;
			
			timer.Sleep(100); // Blink
		}
		if (init_print == 2 || init_print == 1)
			printf("Done\n");
		
#ifdef DYNAMIXEL_DEBUG
		printf("Turning off LED\n");
#endif
		// Turn off LED
		for(int i=0; i<nrOfServos; i++)
		{
			servo[servoIndex[i]]->SetValueAtAdress(ikarosOutBind[IK_IN_LED][i], 0);
			com->AddDataSyncWrite1(servoId[i], ikarosInBind[IK_IN_LED][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_LED][i]], parameterInSize[IK_IN_LED][i]);
		}
		com->SendSyncWrite1();
		
		// Set goal position to current position.
		//		for(int i=0; i<nrOfServos; i++)
		//			servo[servoIndex[i]]->SetValueAtAdress(ikarosOutBind[IK_IN_GOAL_POSITION][i], servo[servoIndex[i]]->GetValueAtAdress(ikarosOutBind[IK_OUT_GOAL_POSITION][i]));
		//		com->WriteToServo(servoId, mask, protocol, DynamixelMemoeries,ikarosOutBind[IK_OUT_GOAL_POSITION], parameterInSize[IK_OUT_GOAL_POSITION], nrOfServos);
		
		// Get feedback
		for(int i=0; i<nrOfServos; i++)
		{
#ifdef DYNAMIXEL_DEBUG
			printf("Getting current position\n");
#endif
			com->ReadMemoryRange(servo[servoIndex[i]]->extraInfo.GetInt("ID"), servo[servoIndex[i]]->protocol, servo[servoIndex[i]]->dynamixelMemory, 0, servo[servoIndex[i]]->extraInfo.GetInt("Model Memory"));
			servo[servoIndex[i]]->SetValueAtAdress(ikarosOutBind[IK_OUT_GOAL_POSITION][i], servo[servoIndex[i]]->GetValueAtAdress(ikarosOutBind[IK_OUT_PRESENT_POSITION][i]));
			com->AddDataSyncWrite1(servoId[i], ikarosInBind[IK_OUT_GOAL_POSITION][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_GOAL_POSITION][i]], parameterInSize[IK_IN_GOAL_POSITION][i]);
		}
#ifdef DYNAMIXEL_DEBUG
		printf("Setting GoalPosition to current position\n");
#endif
		com->SendSyncWrite1();
		timer.Sleep(100); // Sleep to make sure everyting is sent to servo before deleting memory
	}
	
	
	// Comminication Error report
	printf("\nError Report\n");
	printf("Crc Errors:\t\t%i\n",com->crcError);
	printf("Missing bytes:\t%i\n",com->missingBytesError);
	printf("Not complete:\t%i\n",com->notCompleteError);
	printf("Extended check:\t%i\n",com->extendedError);
	printf("Ticks:\t\t\t%li\n",GetTick());
	
	
	
	// Free memory
	delete mask;
	for(int i=0; i<10; i++)
		delete(ikarosInBind[i]);
	delete(ikarosInBind);
	for(int i=0; i<10; i++)
		delete(parameterInSize[i]);
	delete(parameterInSize);
	for(int i=0; i<16; i++)
		delete(ikarosOutBind[i]);
	delete(ikarosOutBind);
	//delete DynamixelMemoeries;
	delete servoIndex;
	delete servoId;
	delete com;
	delete servo; // Also deletes servo's memorybuffer
}

void
Dynamixel::Tick()
{
#ifdef DYNAMIXEL_TIMING
	Timer t;
#endif
	
	// Reset optimize matrix
	for(int i=0; i<nrOfServos; i++)
		for(int j=0; j<IK_INPUTS; j++)
			optimize[i][j] = false;
	if (optimize_mode)
		OptimizeSendCalls();
	
	// Torque enable feature/bug. If goal position is sent to the servo, the servo will set tourque enable to 1.
	bool torqueON[nrOfServos];
	for(int i=0; i<nrOfServos; i++)
		torqueON[i] = true;
	
	// Check that the device exists and is open
	if(!device || !com)
		return;
	
	float torqueMultiplier = 0; // For ramping up torque at start.
	
	if(GetTick() >= start_up_delay) // Do not send any instructions during start_up_delay.
	{
		for(int i=0; i<nrOfServos; i++)
		{
			// Bug/feature sending torqueEnable is not good.
			if (torqueEnableConnected && ikarosInBind[IK_IN_TORQUE_ENABLE][i] != -1)
			{
				servo[servoIndex[i]]->SetTorqueEnableFormated(ikarosInBind[IK_IN_TORQUE_ENABLE][i],torqueEnable[servoIndex[i]]);
				
				// Torque enable feature/bug. Could perhaps be moved to active.
				if (torqueEnable[servoIndex[i]] == 0)
					torqueON[i]  = false;
			}
			
			if (LEDConnected && ikarosInBind[IK_IN_LED][i] != -1)
				servo[servoIndex[i]]->SetLEDFormated(ikarosInBind[IK_IN_LED][i],LED[servoIndex[i]]);
			if (dGainConnected && ikarosInBind[IK_IN_D_GAIN][i] != -1)
				servo[servoIndex[i]]->SetDGainFormated(ikarosInBind[IK_IN_D_GAIN][i],dGain[servoIndex[i]]);
			if (iGainConnected && ikarosInBind[IK_IN_I_GAIN][i] != -1)
				servo[servoIndex[i]]->SetIGainFormated(ikarosInBind[IK_IN_I_GAIN][i],iGain[servoIndex[i]]);
			if (pGainConnected && ikarosInBind[IK_IN_P_GAIN][i] != -1)
				servo[servoIndex[i]]->SetPGainFormated(ikarosInBind[IK_IN_P_GAIN][i],pGain[servoIndex[i]]);
			if (goalPositionConnected && ikarosInBind[IK_IN_GOAL_POSITION][i] != -1)
				servo[servoIndex[i]]->SetGoalPositionFormated(ikarosInBind[IK_IN_GOAL_POSITION][i],goalPosition[servoIndex[i]], angle_unit);
			if (movingSpeedConnected && ikarosInBind[IK_IN_MOVING_SPEED][i] != -1)
				servo[servoIndex[i]]->SetMovingSpeedFormated(ikarosInBind[IK_IN_MOVING_SPEED][i],movingSpeed[servoIndex[i]]);
			
			if (GetTick() <= start_up_delay)
				torqueMultiplier = 0; // No torque during start_up
			else if (GetTick() <= start_up_delay + torque_up_delay)
				torqueMultiplier = ((GetTick() - start_up_delay) / float(torque_up_delay)); // Ramping up torque from value servo has now
			else
				torqueMultiplier = 1;
			
			if (torqueLimitConnected && ikarosInBind[IK_IN_TORQUE_LIMIT][i] != -1) // TorqueLimitConnected must be connected
				servo[servoIndex[i]]->SetTorqueLimitFormated(ikarosInBind[IK_IN_TORQUE_LIMIT][i], torqueLimit[servoIndex[i]]*torqueMultiplier); // Ramp up torque
			
			if (goalTorqueConnected && ikarosInBind[IK_IN_GOAL_TORQUE][i] != -1)
				servo[servoIndex[i]]->SetGoalTorqueFormated(ikarosInBind[IK_IN_GOAL_TORQUE][i],goalTorque[servoIndex[i]]);
			if (goalAccelerationConnected && ikarosInBind[IK_IN_GOAL_ACCELERATION][i] != -1)
				servo[servoIndex[i]]->SetGoalAccelerationFormated(ikarosInBind[IK_IN_GOAL_ACCELERATION][i],goalAcceleration[servoIndex[i]]);
		}
#ifdef DYNAMIXEL_TIMING
		t.Restart();
		com->sendTimer = 0;
#endif
		// Check protocol
		if (protocol == 1)
		{
	
			for(int i=0; i<nrOfServos; i++)
				if (active[i][IK_IN_TORQUE_ENABLE] and !optimize[i][IK_IN_TORQUE_ENABLE])
					com->AddDataSyncWrite1(servoId[i], ikarosInBind[IK_IN_TORQUE_ENABLE][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_TORQUE_ENABLE][i]], parameterInSize[IK_IN_TORQUE_ENABLE][i]);
			com->SendSyncWrite1();
			
			for(int i=0; i<nrOfServos; i++)
				if (active[i][IK_IN_LED] and !optimize[i][IK_IN_LED])
					com->AddDataSyncWrite1(servoId[i], ikarosInBind[IK_IN_LED][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_LED][i]], parameterInSize[IK_IN_LED][i]);
			com->SendSyncWrite1();
			
			for(int i=0; i<nrOfServos; i++)
				if (active[i][IK_IN_D_GAIN] and !optimize[i][IK_IN_D_GAIN])
					com->AddDataSyncWrite1(servoId[i], ikarosInBind[IK_IN_D_GAIN][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_D_GAIN][i]], parameterInSize[IK_IN_D_GAIN][i]);
			com->SendSyncWrite1();
			
			for(int i=0; i<nrOfServos; i++)
				if (active[i][IK_IN_I_GAIN] and !optimize[i][IK_IN_I_GAIN])
					com->AddDataSyncWrite1(servoId[i], ikarosInBind[IK_IN_I_GAIN][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_I_GAIN][i]], parameterInSize[IK_IN_I_GAIN][i]);
			com->SendSyncWrite1();
			
			for(int i=0; i<nrOfServos; i++)
				if (active[i][IK_IN_P_GAIN] and !optimize[i][IK_IN_P_GAIN])
					com->AddDataSyncWrite1(servoId[i], ikarosInBind[IK_IN_P_GAIN][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_P_GAIN][i]], parameterInSize[IK_IN_P_GAIN][i]);
			com->SendSyncWrite1();
			
			// Sending block with most common paramters to reduce serial calls on common paramters
			// Mem adr 30-35 (Goal Position(L) Goal Position(H) Moving Speed(L) Moving Speed(H) Torque Limit(L) Torque Limit(H). Always send these. If they are not connected send the recived from memory. Use syncwrite to write if input is connected for the rest of the inputs.
			for(int i=0; i<nrOfServos; i++)
			{
				int blockSize = parameterInSize[IK_IN_GOAL_POSITION][i] + parameterInSize[IK_IN_MOVING_SPEED][i] + parameterInSize[IK_IN_TORQUE_LIMIT][i];
				if (torqueON[i])
					com->AddDataSyncWrite1(servoId[i], ikarosInBind[IK_IN_GOAL_POSITION][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_GOAL_POSITION][i]], blockSize);
			}
			com->SendSyncWrite1();
			
			//			for(int i=0; i<nrOfServos; i++)
			//				if (goalPositionConnected && ikarosInBind[IK_IN_GOAL_POSITION][i] != -1)
			//					com->AddDataSyncWrite1(servoId[i], ikarosInBind[IK_IN_GOAL_POSITION][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_GOAL_POSITION][i]], parameterInSize[IK_IN_GOAL_POSITION][i]);
			//			com->SendSyncWrite1();
			//			for(int i=0; i<nrOfServos; i++)
			//				if (movingSpeedConnected && ikarosInBind[IK_IN_MOVING_SPEED][i] != -1)
			//					com->AddDataSyncWrite1(servoId[i], ikarosInBind[IK_IN_MOVING_SPEED][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_MOVING_SPEED][i]], parameterInSize[IK_IN_MOVING_SPEED][i]);
			//			com->SendSyncWrite1();
			//			for(int i=0; i<nrOfServos; i++)
			//				if (torqueLimitConnected && ikarosInBind[IK_IN_TORQUE_LIMIT][i] != -1)
			//					com->AddDataSyncWrite1(servoId[i], ikarosInBind[IK_IN_TORQUE_LIMIT][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_TORQUE_LIMIT][i]], parameterInSize[IK_IN_TORQUE_LIMIT][i]);
			//			com->SendSyncWrite1();
			
			for(int i=0; i<nrOfServos; i++)
				if (active[i][IK_IN_GOAL_TORQUE] and !optimize[i][IK_IN_GOAL_TORQUE])
					com->AddDataSyncWrite1(servoId[i], ikarosInBind[IK_IN_GOAL_TORQUE][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_GOAL_TORQUE][i]], parameterInSize[IK_IN_GOAL_TORQUE][i]);
			com->SendSyncWrite1();
			
			for(int i=0; i<nrOfServos; i++)
				if (active[i][IK_IN_GOAL_ACCELERATION] and !optimize[i][IK_IN_GOAL_ACCELERATION])
					com->AddDataSyncWrite1(servoId[i], ikarosInBind[IK_IN_GOAL_ACCELERATION][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_GOAL_ACCELERATION][i]], parameterInSize[IK_IN_GOAL_ACCELERATION][i]);
			com->SendSyncWrite1();
		}
		if (protocol == 2)
		{
			// Version 2
			// Using bulkwrite in the same way as syncwrite. A limitation of bulkwrite is that a servo id is only allowed once for each call. By using bulkwrite the adress and sizes may change with dynamixel versions. This has not been fully tested.
			for(int i=0; i<nrOfServos; i++)
				if (torqueEnableConnected && ikarosInBind[IK_IN_TORQUE_ENABLE][i] != -1)
					com->AddDataBulkWrite2(servoId[i], ikarosInBind[IK_IN_TORQUE_ENABLE][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_TORQUE_ENABLE][i]], parameterInSize[IK_IN_TORQUE_ENABLE][i]);
			com->SendBulkWrite2();
			
			for(int i=0; i<nrOfServos; i++)
				if (LEDConnected && ikarosInBind[IK_IN_LED][i] != -1)
					com->AddDataBulkWrite2(servoId[i], ikarosInBind[IK_IN_LED][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_LED][i]], parameterInSize[IK_IN_LED][i]);
			com->SendBulkWrite2();
			
			for(int i=0; i<nrOfServos; i++)
				if (dGainConnected && ikarosInBind[IK_IN_D_GAIN][i] != -1)
					com->AddDataBulkWrite2(servoId[i], ikarosInBind[IK_IN_D_GAIN][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_D_GAIN][i]], parameterInSize[IK_IN_D_GAIN][i]);
			com->SendBulkWrite2();
			
			for(int i=0; i<nrOfServos; i++)
				if (iGainConnected && ikarosInBind[IK_IN_I_GAIN][i] != -1)
					com->AddDataBulkWrite2(servoId[i], ikarosInBind[IK_IN_I_GAIN][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_D_GAIN][i]], parameterInSize[IK_IN_D_GAIN][i]);
			com->SendBulkWrite2();
			
			for(int i=0; i<nrOfServos; i++)
				if (pGainConnected && ikarosInBind[IK_IN_P_GAIN][i] != -1)
					com->AddDataBulkWrite2(servoId[i], ikarosInBind[IK_IN_P_GAIN][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_D_GAIN][i]], parameterInSize[IK_IN_D_GAIN][i]);
			com->SendBulkWrite2();
			
			for(int i=0; i<nrOfServos; i++)
				if (goalPositionConnected && ikarosInBind[IK_IN_GOAL_POSITION][i] != -1)
					com->AddDataBulkWrite2(servoId[i], ikarosInBind[IK_IN_GOAL_POSITION][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_GOAL_POSITION][i]], parameterInSize[IK_IN_GOAL_POSITION][i]);
			com->SendBulkWrite2();
			for(int i=0; i<nrOfServos; i++)
				if (movingSpeedConnected && ikarosInBind[IK_IN_MOVING_SPEED][i] != -1)
					com->AddDataBulkWrite2(servoId[i], ikarosInBind[IK_IN_MOVING_SPEED][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_MOVING_SPEED][i]], parameterInSize[IK_IN_MOVING_SPEED][i]);
			com->SendBulkWrite2();
			for(int i=0; i<nrOfServos; i++)
				if (torqueLimitConnected && ikarosInBind[IK_IN_TORQUE_LIMIT][i] != -1)
					com->AddDataBulkWrite2(servoId[i], ikarosInBind[IK_IN_TORQUE_LIMIT][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_TORQUE_LIMIT][i]], parameterInSize[IK_IN_TORQUE_LIMIT][i]);
			com->SendBulkWrite2();
			
			for(int i=0; i<nrOfServos; i++)
				if (goalTorqueConnected && ikarosInBind[IK_IN_GOAL_TORQUE][i] != -1)
					com->AddDataBulkWrite2(servoId[i], ikarosInBind[IK_IN_GOAL_TORQUE][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_GOAL_TORQUE][i]], parameterInSize[IK_IN_GOAL_TORQUE][i]);
			com->SendBulkWrite2();
			
			for(int i=0; i<nrOfServos; i++)
				if (goalAccelerationConnected && ikarosInBind[IK_IN_GOAL_ACCELERATION][i] != -1)
					com->AddDataBulkWrite2(servoId[i], ikarosInBind[IK_IN_GOAL_ACCELERATION][i], &servo[servoIndex[i]]->dynamixelMemory[ikarosInBind[IK_IN_GOAL_ACCELERATION][i]], parameterInSize[IK_IN_GOAL_ACCELERATION][i]);
			com->SendBulkWrite2();
		}
	}
#ifdef DYNAMIXEL_TIMING
	t.Restart();
	com->ReciveTimer = 0;
#endif
	
	// Get feedback
	for(int i=0; i<nrOfServos; i++)
	{
		if(use_feedback)
			com->ReadMemoryRange(servo[servoIndex[i]]->extraInfo.GetInt("ID"), servo[servoIndex[i]]->protocol, servo[servoIndex[i]]->dynamixelMemory, 0, servo[servoIndex[i]]->extraInfo.GetInt("Model Memory"));
#ifdef DYNAMIXEL_TIMING
		com->ReciveTimer = t.GetTime()-com->ReciveTimer;
#endif
		if (ikarosOutBind[IK_OUT_TORQUE_ENABLE][i] != -1)
			feedbackTorqueEnable[servoIndex[i]]  = servo[servoIndex[i]]->GetTorqueEnableFormated(ikarosOutBind[IK_OUT_TORQUE_ENABLE][i]);
		if (ikarosOutBind[IK_OUT_LED][i] != -1)
			feedbackLED[servoIndex[i]] = servo[servoIndex[i]]->GetLEDFormated(ikarosOutBind[IK_OUT_LED][i]);
		if (ikarosOutBind[IK_OUT_D_GAIN][i] != -1)
			feedbackDGain[servoIndex[i]] = servo[servoIndex[i]]->GetDGainFormated(ikarosOutBind[IK_OUT_D_GAIN][i]);
		if (ikarosOutBind[IK_OUT_I_GAIN][i] != -1)
			feedbackIGain[servoIndex[i]] = servo[servoIndex[i]]->GetIGainFormated(ikarosOutBind[IK_OUT_I_GAIN][i]);
		if (ikarosOutBind[IK_OUT_P_GAIN][i] != -1)
			feedbackPGain[servoIndex[i]] = servo[servoIndex[i]]->GetPGainFormated(ikarosOutBind[IK_OUT_P_GAIN][i]);
		if (ikarosOutBind[IK_OUT_GOAL_POSITION][i] != -1)
			feedbackGoalPosition[servoIndex[i]] = servo[servoIndex[i]]->GetGoalPositionFormated(ikarosOutBind[IK_OUT_GOAL_POSITION][i],angle_unit);
		if (ikarosOutBind[IK_OUT_MOVING_SPEED][i] != -1)
			feedbackMoving[servoIndex[i]] = servo[servoIndex[i]]->GetMovingSpeedFormated(ikarosOutBind[IK_OUT_MOVING_SPEED][i]);
		if (ikarosOutBind[IK_OUT_TORQUE_LIMIT][i] != -1)
			feedbackTorqueLimit[servoIndex[i]] = servo[servoIndex[i]]->GetTorqueLimitFormated(ikarosOutBind[IK_OUT_TORQUE_LIMIT][i]);
		if (ikarosOutBind[IK_OUT_PRESENT_POSITION][i] != -1)
			feedbackPresentPosition[servoIndex[i]] = servo[servoIndex[i]]->GetPresentPositionFormated(ikarosOutBind[IK_OUT_PRESENT_POSITION][i],angle_unit);
		if (ikarosOutBind[IK_OUT_PRESENT_SPEED][i] != -1)
			feedbackPresentSpeed[servoIndex[i]] = servo[servoIndex[i]]->GetPresentSpeedFormated(ikarosOutBind[IK_OUT_PRESENT_SPEED][i]);
		if (ikarosOutBind[IK_OUT_PRESENT_LOAD][i] != -1)
			feedbackPresentLoad[servoIndex[i]] = servo[servoIndex[i]]->GetPresentLoadFormated(ikarosOutBind[IK_OUT_PRESENT_LOAD][i]);
		if (ikarosOutBind[IK_OUT_PRESENT_VOLTAGE][i] != -1)
			feedbackPresentVoltage[servoIndex[i]] =  servo[servoIndex[i]]->GetPresentVoltageFormated(ikarosOutBind[IK_OUT_PRESENT_VOLTAGE][i]);
		if (ikarosOutBind[IK_OUT_PRESENT_TEMPERATURE][i] != -1)
			feedbackPresentTemperature[servoIndex[i]] = servo[servoIndex[i]]->GetPresentTemperatureFormated(ikarosOutBind[IK_OUT_PRESENT_TEMPERATURE][i]);
		if (ikarosOutBind[IK_OUT_PRESENT_CURRENT][i] != -1)
			feedbackPresentCurrent[servoIndex[i]] = servo[servoIndex[i]]->GetCurrentFormated(ikarosOutBind[IK_OUT_PRESENT_CURRENT][i]);
		if (ikarosOutBind[IK_OUT_GOAL_TORQUE][i] != -1)
			feedbackGoalTorque[servoIndex[i]] = servo[servoIndex[i]]->GetGoalTorqueFormated(ikarosOutBind[IK_OUT_GOAL_TORQUE][i]);
		if (ikarosOutBind[IK_OUT_GOAL_ACCELERATION][i] != -1)
			feedbackGoalAcceleration[servoIndex[i]] = servo[servoIndex[i]]->GetGoalAccelerationFormated(ikarosOutBind[IK_OUT_GOAL_ACCELERATION][i]);
	}
	// Check temp
	for(int i=0; i<nrOfServos; i++)
		if (servo[servoIndex[i]]->GetPresentTemperatureFormated(ikarosOutBind[IK_OUT_PRESENT_TEMPERATURE][i]) > max_temperature)
		{
			PrintAll();
			Notify(msg_fatal_error, "Servo temperature is over limit. Shuting down ikaros\n");
		}
#ifdef DYNAMIXEL_TIMING
	printf("Timers Send:%f\t Recv:%f\n", com->sendTimer, com->ReciveTimer);
#endif
}

void
Dynamixel::Print()
{
	printf("\nDYNAMIXEL\n");
	printf("Number of servos: %d\n\n", nrOfServos);
	printf("%6s %5s\n", "ID", "Name");
	for(int i=0; i<nrOfServos; i++)
	{
		printf("%-4i",i);
		printf("%-4i",servo[servoIndex[i]]->extraInfo.GetInt("ID"));
		printf("%-6s",servo[servoIndex[i]]->extraInfo.Get("Servo Model String"));
		printf("\n");
	}
}

void
Dynamixel::PrintAll()
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
				if (servo[servoIndex[i]]->GetValueAtAdress(j) == -1)
					printf("      -|");
				else
					printf(" %6i|",servo[servoIndex[i]]->GetValueAtAdress(j));
			printf("\n");
		}
	}
}
void Dynamixel::OptimizeSendCalls()
{
	// Skipping sending if the servo has the same value as we trying to send.
	// Check performence
	for(int i=0; i<nrOfServos; i++)
		for(int j=0; j<IK_INPUTS; j++)
			if (active[i][j])
				
				switch (j) {
					case IK_IN_TORQUE_ENABLE:
						if (servo[servoIndex[i]]->GetTorqueEnableFormated(ikarosInBind[j][i]) == torqueEnable[servoIndex[i]])
							optimize[i][j] = true;
						break;
					case IK_IN_LED:
						if (servo[servoIndex[i]]->GetLEDFormated(ikarosInBind[j][i]) == LED[servoIndex[i]])
							optimize[i][j] = true;
						break;
					case IK_IN_D_GAIN:
						if (servo[servoIndex[i]]->GetDGainFormated(ikarosInBind[j][i]) == dGain[servoIndex[i]])
							optimize[i][j] = true;
						break;
					case IK_IN_I_GAIN:
						if (servo[servoIndex[i]]->GetIGainFormated(ikarosInBind[j][i]) == iGain[servoIndex[i]])
							optimize[i][j] = true;
						break;
					case IK_IN_P_GAIN:
						if (servo[servoIndex[i]]->GetPGainFormated(ikarosInBind[j][i]) == pGain[servoIndex[i]])
							optimize[i][j] = true;
						break;
					case IK_IN_GOAL_POSITION:
						if (servo[servoIndex[i]]->GetGoalPositionFormated(ikarosInBind[j][i], angle_unit) == goalPosition[servoIndex[i]])
							optimize[i][j] = true;
						break;
					case IK_IN_MOVING_SPEED:
						if (servo[servoIndex[i]]->GetMovingSpeedFormated(ikarosInBind[j][i]) == movingSpeed[servoIndex[i]])
							optimize[i][j] = true;
						break;
					case IK_IN_TORQUE_LIMIT:
						if (servo[servoIndex[i]]->GetTorqueLimitFormated(ikarosInBind[j][i]) == torqueLimit[servoIndex[i]])
							optimize[i][j] = true;
						break;
					case IK_IN_GOAL_TORQUE:
						if (servo[servoIndex[i]]->GetGoalTorqueFormated(ikarosInBind[j][i]) == goalTorque[servoIndex[i]])
							optimize[i][j] = true;
						break;
					case IK_IN_GOAL_ACCELERATION:
						if (servo[servoIndex[i]]->GetGoalAccelerationFormated(ikarosInBind[j][i]) == goalAcceleration[servoIndex[i]])
							optimize[i][j] = true;
						break;
					default:
						break;
				}
	
	//	printf("Optimize\n");
	//	for(int i=0; i<nrOfServos; i++)
	//	{
	//		for(int j=0; j<IK_INPUTS; j++)
	//			printf("%i:%i=%i\t",i ,j, optimize[i][j]) ;
	//		printf("\n");
	//	}
}
static InitClass init("Dynamixel", &Dynamixel::Create, "Source/Modules/RobotModules/Dynamixel/");
