//
//    DynamixelComm.h
//
//    Copyright (C) 2018  Birger Johansson
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
//
//    Created: March, 2016
//


#ifndef DYNAMIXELSERVO
#define DYNAMIXELSERVO

//#define DEBUG_SERVO

// Common
#define DYNAMIXEL_MEM_BUFFER 		1024

#define OP_WHEEL                    0
#define OP_JOINT                    1
#define OP_MULTI_TURN               2

// A few Eprom adresses needed (Hopefully the same on all models).
#define P_MODEL_NUMBER              0
#define P_ID                        3
#define P_BAUD_RATE                 4
#define P_RETURN_DELAY_TIME         5
#define P_CW_ANGLE_LIMIT            6
#define P_CCW_ANGLE_LIMIT           8

#define C_TABLE_SIZE                DYNAMIXEL_MEM_BUFFER

#include "IKAROS.h"
#include "DynamixelComm.h"
#include "DynamixelServo.h"

#include <string>
#include <iostream>
#include <vector>

struct CT {
	int  		Adress;
	int  		Size;
	std::string Name;
	std::string Description;
	std::string Access;
	int   		Inital;
	int   		Min;
	int   		Max;
	float		Convert;
	int   		IkarosInputs;
	int   		IkarosOutputs;
	bool  		Visable;
} ;

/**
 Class for dynamixel servo
 */
class DynamixelServo
{
public:

	DynamixelServo(DynamixelComm *com, int id, const char * csvPath, int forceModel);  // forceModel is used by configurate module only
	~DynamixelServo();
	
	// Variables
	unsigned char * dynamixelMemory;    // Memory of the servo is mirrored here
	Dictionary extraInfo;               // Some extra model specific information
	CT controlTable[C_TABLE_SIZE];      // The Control table read from csv file. Here is the mapping to ikaros IO
	
	int protocol;
	int model;
	
	bool ReadCSVFileToCtable(CT * ctable, int model, int * size, const char * csvPath);
	void PrintControlTable();
	void GetAdditionalInfo(int model);
	bool PrintMemory();
	
	// Get/set memory
	bool    SetValueAtAdress(int adress, int value);
	int     GetValueAtAdress(int adress);
	float   GetValueAtAdressFormated(int adress);
	
	// Converters
	float   DynamixelToInternal(int position); 	// Converts dynamixel position to degrees
	int     InternalToDynamixel(float angle); 	// degrees to dynamixel position
	
	// Model specific values
	int GetModelPositionMax();
	int GetModelSpeedMax();
	int GetModelTorqueMax();
	int GetModelLoadMax();
	int GetModelAngleMax();
	int GetModelBaudRateMax();
	
	// Formated data, value return is in angle unit
	// Model specific values
	float GetModelGapAngleFormated(int angleUnit);
	float GetModelBaudRateMaxFormated();
	
	// Get formated
	float GetTorqueEnableFormated(int adress);
	float GetLEDFormated(int adress);
	float GetDGainFormated(int adress);
	float GetIGainFormated(int adress);
	float GetPGainFormated(int adress);
	float GetGoalPositionFormated(int adress,int angleUnit);
	float GetMovingSpeedFormated(int adress);
	float GetTorqueLimitFormated(int adress);
	float GetPresentPositionFormated(int adress,int angleUnit);
	float GetPresentSpeedFormated(int adress);
	float GetPresentLoadFormated(int adress);
	float GetPresentVoltageFormated(int adress);
	float GetPresentTemperatureFormated(int adress);            
	float GetCurrentFormated(int adress);
	float GetGoalTorqueFormated(int adress);
	float GetGoalAccelerationFormated(int adress);
	
	// Set formated
	bool SetTorqueEnableFormated(int adress, float value);
	bool SetLEDFormated(int adress, float value);
	bool SetDGainFormated(int adress, float value);
	bool SetIGainFormated(int adress, float value);
	bool SetPGainFormated(int adress, float value);
	bool SetGoalPositionFormated(int adress, float value, int angleUnit);
	bool SetMovingSpeedFormated(int adress, float value);
	bool SetTorqueLimitFormated(int adress, float value);
	bool SetGoalTorqueFormated(int adress, float value);
	bool SetGoalAccelerationFormated(int adress, float value);
};
#endif
