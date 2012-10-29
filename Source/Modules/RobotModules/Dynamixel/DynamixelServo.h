//
//    DynamixelComm.h		Class to communicate with USB2Dynamixel
//
//    Copyright (C) 2010-2011  Christian Balkenius
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
//    Created: April 4, 2010
//


#ifndef DYNAMIXELSERVO
#define DYNAMIXELSERVO

#define DYNAMIXEL_EPROM_SIZE 18
#define DYNAMIXEL_RAM_SIZE 70-18
#define DYNAMIXEL_MEM_BUFFER 128 // Using 128 instead of 70 as in control table


#include "IKAROS.h"

#include "DynamixelComm.h"
#include "DynamixelServo.h"

class DynamixelServo
{
public:
    
    DynamixelServo(DynamixelComm *com, int id);
    ~DynamixelServo();

    unsigned char * DynamixelMemory;
    Dictionary controlTable;
    
    void ParseControlTable(unsigned char* inbuf, int from, int to);
    float ConvertToAngle(int position, int angle_unit);
    int ConvertToPosition(float angle, int angle_unit);
    const char * ByteToBinary(int x);
    int CheckBaudRate(int value);
    float ConvertAngleToAngle(float angle, int from_angle_unit, int to_angle_unit);
        
    bool SetID(int value);
    bool SetBaudRate(int value);
    bool SetReturnDelayTime(int value);
    bool SetCWAngleLimit(int value);
    bool SetCCWAngleLimit(int value);
    bool SetDriveMode(int value);
    bool SetHighestLimitTemperature(int value);
    bool SetHighestLimitVoltage(int value);
    bool SetLowestLimitVoltage(int value);
    bool SetMaxTorque(int value);
    bool SetStatusReturnLevel(int value);
    bool SetAlarmLED(int value);
    bool SetAlarmLEDBit(int bit, bool value);
    bool SetAlarmShutdown(int value);
    bool SetTorqueEnable(int value);
    bool SetLED(int value);
    bool SetDGain(int value);
    bool SetIGain(int value);
    bool SetPGain(int value);
    bool SetGoalPosition(int value);
    bool SetMovingSpeed(int value);
    bool SetTorqueLimit(int value);
    // ...
    bool SetLock(int value);
    bool SetPunch(int value);
    //bool SetCurrent(int value);
    
    // Formated data
    bool SetIDFormated(float value);
    bool SetBaudRateFormated(float value);
    bool SetReturnDelayTimeFormated(float value);
    bool SetCWAngleLimitFormated(float value, int angle_unit);
    bool SetCCWAngleLimitFormated(float value, int angle_unit);
    bool SetDriveModeFormated(float value);
    bool SetHighestLimitTemperatureFormated(float value);
    bool SetHighestLimitVoltageFormated(float value);
    bool SetLowestLimitVoltageFormated(float value);
    bool SetMaxTorqueFormated(float value, int angle_unit);
    bool SetStatusReturnLevelFormated(float value);
    bool SetAlarmLEDFormated(float value);
    bool SetAlarmShutdownFormated(float value);
    bool SetTorqueEnableFormated(float value);
    bool SetLEDFormated(float value);
    bool SetDGainFormated(float value);
    bool SetIGainFormated(float value);
    bool SetPGainFormated(float value);
    bool SetGoalPositionFormated(float value, int angle_unit);
    bool SetMovingSpeedFormated(float value);
    bool SetTorqueLimitFormated(float value);
    // ...
    bool SetLockFormated(float value);
    bool SetPunchFormated(float value);
    
    // Returns the value grabbed from Dynamixel
    // Model specific values
    int GetModelPositionMax();
    int GetModelSpeedMax();
    int GetModelTorqueMax();
    int GetModelLoadMax();
    int GetModelAngleMax();
    int GetModelBaudRateMax();
    int GetModelRecomendedVoltage();
    int GetModelStandbyCurrent();
    int GetModelDGainEnable();
    int GetModelIGainEnable();
    int GetModelPGainEnable();
    int GetModelCurrentEnable();
    int GetModelGapPosition();

    // Controll Table
    int GetModelNumber();
    int GetFirmware();
    int GetID();
    int GetBaudRate();
    int GetReturnDelayTime();
    int GetCWAngleLimit();
    int GetCCWAngleLimit();
    int GetDriveMode();
    int GetHighestLimitTemperature();
    int GetHighestLimitVoltage();
    int GetLowestLimitVoltage();
    int GetMaxTorque();
    int GetStatusReturnLevel();
    int GetAlarmLED();
    int GetAlarmLEDBit(int bit);
    int GetAlarmShutdown();
    //int GetAlarmShutdownBit(int bit);
    // ...
    int GetTorqueEnable();
    int GetLED();
    int GetDGain();
    int GetIGain();
    int GetPGain();
    int GetGoalPosition();
    int GetMovingSpeed();
    int GetTorqueLimit();
    int GetPresentPosition();
    int GetPresentSpeed();
    int GetPresentLoad();
    int GetPresentVoltage();
    int GetPresentTemperature();
    int GetRegistered();
    int GetMoving();
    int GetLock();
    int GetPunch();
    // ...
    int GetCurrent();
    
    // Formated data, value return is in angle_unit or as specified paramter list.
    // Model specific values
    float GetModelGapAngleFormated(int angle_unit);
    float GetModelBaudRateMaxFormated();
    float GetModelDGainEnableFormated();
    float GetModelIGainEnableFormated();
    float GetModelPGainEnableFormated();
    float GetModelCurrentEnableFormat();

    // Control Table
    float GetModelNumberFormated();
    float GetFirmwareFormated();
    float GetIDFormated();
    float GetBaudRateFormated();
    float GetReturnDelayTimeFormated();
    float GetCWAngleLimitFormated(int angle_unit);
    float GetCCWAngleLimitFormated(int angle_unit);
    float GetDriveModeFormated();
    float GetHighestLimitTemperatureFormated();
    float GetHighestLimitVoltageFormated();
    float GetLowestLimitVoltageFormated();
    float GetMaxTorqueFormated();
    float GetStatusReturnLevelFormated();
    float GetAlarmLEDFormated();
    float GetAlarmLEDBitFormated(int bit);
    float GetAlarmShutdownFormated();
    float GetAlarmShutdownBitFormated(int bit);
    // ...
    float GetTorqueEnableFormated();
    float GetLEDFormated();
    float GetDGainFormated();
    float GetIGainFormated();
    float GetPGainFormated();
    float GetGoalPositionFormated(int angle_unit);
    float GetMovingSpeedFormated();
    float GetTorqueLimitFormated();
    float GetPresentPositionFormated(int angle_unit);
    float GetPresentSpeedFormated();
    float GetPresentLoadFormated();
    float GetPresentVoltageFormated();
    float GetPresentTemperatureFormated();
    float GetRegisteredFormated();
    float GetMovingFormated();
    float GetLockFormated();
    float GetPunchFormated();
    // ...
    float GetCurrentFormated();
};
#endif
    
