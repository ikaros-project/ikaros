//
//    DynamixelComm.cc		Class to communicate with USB2Dynamixel
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


// When using sync write the maximum size sent total is 143.

#include "DynamixelServo.h"
#include "IKAROS.h"

using namespace ikaros;

DynamixelServo::DynamixelServo(DynamixelComm *com, int id)
{
    unsigned char inbuf[256];
    
    DynamixelMemory = new unsigned char[DYNAMIXEL_MEM_BUFFER];
    
    // Fetch whole control table from servo
    if (com->ReadMemoryRange(id, inbuf, 0, DYNAMIXEL_EPROM_SIZE + DYNAMIXEL_RAM_SIZE))
    {
        ParseControlTable(inbuf, 0, DYNAMIXEL_EPROM_SIZE + DYNAMIXEL_RAM_SIZE);
        // Copy memory block to memory block pointer
        memcpy(DynamixelMemory, inbuf, DYNAMIXEL_MEM_BUFFER);
    }
    else
    {
        printf("COULD NOT READ DYNAMIXEL %i, Quitting ikaros.... \n", id);
        std::exit(false);
    }
    return;
}

DynamixelServo::~DynamixelServo()
{
    delete DynamixelMemory;
}

// MARK: SET

bool DynamixelServo::SetID(int value)
{
    // Value check
    if (value<=0 || value>=253)
    {
        printf("DynamixelServo (setID): %i Value is invalid...\n", value);
        return false;
    }
    
    char tempBuf[64];
    sprintf(tempBuf, "%d", value);
    
    // Update dictionay
    controlTable.Set("ID", tempBuf);
    
    // Update memory
    DynamixelMemory[3] = value;
    return true;
}
bool DynamixelServo::SetBaudRate(int value)
{
    // Value check
    if (value<=0 || value>=253)
    {
        printf("DynamixelServo (SetBaudRate): %i Value is invalid...\n", value);
        return false;
    }
    char tempBuf[64];
    sprintf(tempBuf, "%d", value);
    
    // Update dictionay
    controlTable.Set("Baud Rate", tempBuf);
    
    // Update memory
    DynamixelMemory[4] = value;
    return true;
}
bool DynamixelServo::SetReturnDelayTime(int value)
{
    // Value check
    if (value<=0 || value>=254)
    {
        printf("DynamixelServo (setReturnDelayTime): %i Value is invalid...\n", value);
        return false;
    }
    
    char tempBuf[64];
    sprintf(tempBuf, "%d", value);
    
    // Update dictionay
    controlTable.Set("Return Delay Time", tempBuf);
    
    // Update memory
    DynamixelMemory[5] = value;
    return true;
}
bool DynamixelServo::SetCWAngleLimit(int value)
{
    // Value check
    if (value<0 || value>GetModelPositionMax())
    {
        printf("DynamixelServo (setCWAngleLimit): %i Value is invalid...\n", value);
        return false;
    }
    
    char tempBuf[64];
    sprintf(tempBuf, "%d", value);
    
    // Update dictionay
    controlTable.Set("CW Angle Limit", tempBuf);
    
    // Update memory
    DynamixelMemory[6] = value % 256;
    DynamixelMemory[7] = value / 256;
    
    return true;
}
bool DynamixelServo::SetCCWAngleLimit(int value)
{
    // Value check
    if (value<0 || value>GetModelPositionMax())
    {
        printf("DynamixelServo (setCCWAngleLimit): %i Value is invalid...\n", value);
        return false;
    }
    
    char tempBuf[64];
    sprintf(tempBuf, "%d", value);
    
    // Update dictionay
    controlTable.Set("CCW Angle Limit", tempBuf);
    
    // Update memory
    DynamixelMemory[8] = value % 256;
    DynamixelMemory[9] = value / 256;
    return true;
}
bool DynamixelServo::SetDriveMode(int value)
{
    // Value check
    if (value<0 || value>1)
    {
        printf("DynamixelServo (setDriveMode): %i Value is invalid...\n", value);
        return false;
    }
    
    char tempBuf[64];
    sprintf(tempBuf, "%d", value);
    
    // Update dictionay
    controlTable.Set("Drive Mode", tempBuf);
    
    // Update memory
    DynamixelMemory[10] = value;
    return true;
}
bool DynamixelServo::SetHighestLimitTemperature(int value)
{
    
    // Value check
    if (value<=10 || value>=99)
    {
        printf("DynamixelServo (setHighestLimitTemperature): %i Value is invalid...\n", value);
        return false;
    }
    
    char tempBuf[64];
    sprintf(tempBuf, "%d", value);
    
    // Update dictionay
    controlTable.Set("Highest Limit Temperature", tempBuf);
    
    // Update memory
    DynamixelMemory[11] = value;
    return true;
}


bool DynamixelServo::SetLowestLimitVoltage(int value)
{
    // Value check
    if (value<=50 || value>=250)
    {
        printf("DynamixelServo (setLowestLimitVoltage): %i Value is invalid...\n", value);
        return false;
    }
    
    char tempBuf[64];
    sprintf(tempBuf, "%d", value);
    
    // Update dictionay
    controlTable.Set("Highest Limit Temperature", tempBuf);
    
    // Update memory
    DynamixelMemory[12] = value;
    return true;
}

bool DynamixelServo::SetHighestLimitVoltage(int value)
{
    // Value check
    if (value<=50 || value>=250)
    {
        printf("DynamixelServo (setHighestLimitTemperature): %i Value is invalid...\n", value);
        return false;
    }
    
    char tempBuf[64];
    sprintf(tempBuf, "%d", value);
    
    // Update dictionay
    controlTable.Set("Highest Limit Temperature", tempBuf);
    
    // Update memory
    DynamixelMemory[13] = value;
    return true;
}
bool DynamixelServo::SetMaxTorque(int value)
{
    // Value check
    if (value<=0 || value>GetModelTorqueMax())
    {
        printf("DynamixelServo (setMaxTorque): %i Value is invalid...\n", value);
        return false;
    }
    
    char tempBuf[64];
    sprintf(tempBuf, "%d", value);
    
    // Update dictionay
    controlTable.Set("Max Torque", tempBuf);
    
    // Update memory
    DynamixelMemory[14] = value % 256;
    DynamixelMemory[15] = value / 256;
    return true;
}
bool DynamixelServo::SetStatusReturnLevel(int value)
{
    // Value check
    if (value<=0 || value>=3)
    {
        printf("DynamixelServo (setStatusReturnLevel): %i Value is invalid...\n", value);
        return false;
    }
    
    char tempBuf[64];
    sprintf(tempBuf, "%d", value);
    
    // Update dictionay
    controlTable.Set("Max Torque", tempBuf);
    
    // Update memory
    DynamixelMemory[16] = value;
    return true;
}
bool DynamixelServo::SetAlarmLED(int value)
{
    // Value check
    if (value<0 || value>255)
    {
        printf("DynamixelServo (setAlarmLED): %i Value is invalid...\n", value);
        return false;
    }
    
    char tempBuf[64];
    sprintf(tempBuf, "%d", value);
    
    // Update dictionay
    controlTable.Set("Alarm LED", tempBuf);
    
    // Update memory
    DynamixelMemory[17] = value;
    return true;
}

bool DynamixelServo::SetAlarmShutdown(int value)
{
    // Value check
    if (value<0 || value>255)
    {
        printf("DynamixelServo (setAlarmShutdown): %i Value is invalid...\n", value);
        return false;
    }
    
    char tempBuf[64];
    sprintf(tempBuf, "%d", value);
    
    // Update dictionay
    controlTable.Set("Alarm Shutdown", tempBuf);
    
    // Update memory
    DynamixelMemory[18] = value;
    return true;
}

bool DynamixelServo::SetTorqueEnable(int value)
{
    if (value<0 || value>1)
    {
        printf("DynamixelServo (setTorqueEnable): %i Value is invalid...\n", value);
        return false;
    }
    
    char tempBuf[64];
    sprintf(tempBuf, "%d", value);
    
    // Update dictionay
    controlTable.Set("Torque Enable", tempBuf);
    
    // Update memory
    DynamixelMemory[24] = value;
    return true;
}

bool DynamixelServo::SetLED(int value)
{
    if (value<0 || value>1)
    {
        printf("DynamixelServo (setLED): %i Value is invalid...\n", value);
        return false;
    }
    
    char tempBuf[64];
    sprintf(tempBuf, "%d", value);
    
    // Update dictionay
    controlTable.Set("LED", tempBuf);
    
    // Update memory
    DynamixelMemory[25] = value;
    
    return true;
}

bool DynamixelServo::SetDGain(int value)
{
    if (value<0 || value>254)
    {
        printf("DynamixelServo (setDGain): %i Value is invalid...\n", value);
        return false;
    }
    
    if (GetModelDGainEnable() == 1)
    {
        char tempBuf[64];
        sprintf(tempBuf, "%d", value);
        
        // Update dictionay
        controlTable.Set("D Gain", tempBuf);
        
        // Update memory
        DynamixelMemory[26] = value;
    }
    else
        printf("DynamixelModule: Dynamixel Servo does not support D Gain input (input is ignored)\n");
    
    return true;
}

bool DynamixelServo::SetIGain(int value)
{
    if (value<0 || value>254)
    {
        printf("DynamixelServo (setDGain): %i Value is invalid...\n", value);
        return false;
    }
    
    if (GetModelIGainEnable() == 1)
    {
        char tempBuf[64];
        sprintf(tempBuf, "%d", value);
        
        // Update dictionay
        controlTable.Set("I Gain", tempBuf);
        
        // Update memory
        DynamixelMemory[27] = value;
    }
    else
        printf("DynamixelModule: Dynamixel Servo does not support I Gain input (input is ignored)\n");
    
    return true;
}

bool DynamixelServo::SetPGain(int value)
{
    if (value<0 || value>254)
    {
        printf("DynamixelServo (setDGain): %i Value is invalid...\n", value);
        return false;
    }
    if (GetModelPGainEnable() == 1)
    {
        char tempBuf[64];
        sprintf(tempBuf, "%d", value);
        
        // Update dictionay
        controlTable.Set("P Gain", tempBuf);
        
        // Update memory
        DynamixelMemory[28] = value;
    }
    else
        printf("DynamixelModule: Dynamixel Servo does not support P Gain input (input is ignored)\n");
    return true;
    
}

bool DynamixelServo::SetGoalPosition(int value)
{
    if (value<0 || value>GetModelPositionMax())
    {
        printf("DynamixelServo (setGoalPosition): %i Value is invalid...\n", value);
        return false;
    }
    
    char tempBuf[64];
    sprintf(tempBuf, "%d", value);
    
    // Update dictionay
    controlTable.Set("Goal Position", tempBuf);
    
    // Update memory
    DynamixelMemory[30] = value % 256;
    DynamixelMemory[31] = value / 256;
    return true;
}

bool DynamixelServo::SetMovingSpeed(int value)
{
    // In wheel mode
    if (!atoi(controlTable.Get("Joint Mode")))
    {
        if (value<0 || value>GetModelSpeedMax()*2)
        {
            printf("DynamixelServo (setMovingSpeed): %i Value is invalid...\n", value);
            return false;
        }
    }
    else
    {
        if (value<0 || value>GetModelSpeedMax())
        {
            printf("DynamixelServo (setMovingSpeed): %i Value is invalid...\n", value);
            return false;
        }
    }
        
    char tempBuf[64];
    sprintf(tempBuf, "%d", value);
    
    // Update dictionay
    controlTable.Set("Moving Speed", tempBuf);
    
    // Update memory
    DynamixelMemory[32] = value % 256;
    DynamixelMemory[33] = value / 256;
    return true;
    
}

bool DynamixelServo::SetTorqueLimit(int value)
{
    if (value<0 || value>GetModelTorqueMax())
    {
        printf("DynamixelServo (setTorqueLimit): %i Value is invalid...\n", value);
        return false;
    }
    
    char tempBuf[64];
    sprintf(tempBuf, "%d", value);
    
    // Update dictionay
    controlTable.Set("Torque Limit", tempBuf);
    
    // Update memory
    DynamixelMemory[34] = value % 256;
    DynamixelMemory[35] = value / 256;
    return true;
}
bool DynamixelServo::SetLock(int value)
{
    if (value<0 || value>1)
    {
        printf("DynamixelServo (setLock): %i Value is invalid...\n", value);
        return false;
    }
    
    char tempBuf[64];
    sprintf(tempBuf, "%d", value);
    
    // Update dictionay
    controlTable.Set("Lock", tempBuf);
    
    // Update memory
    DynamixelMemory[47] = value;
    return true;
}
bool DynamixelServo::SetPunch(int value)
{
    if (value<0 || value>1023)
    {
        printf("DynamixelServo (setPunch): %i Value is invalid...\n", value);
        return false;
    }
    
    char tempBuf[64];
    sprintf(tempBuf, "%d", value);
    
    printf("Setting %i\n", value);
    // Update dictionay
    controlTable.Set("Punch", tempBuf);
    
    // Update memory
    DynamixelMemory[48] = value % 256;;
    DynamixelMemory[49] = value / 256;;
    
    return true;
}


// MARK: SET FORMATED

bool DynamixelServo::SetIDFormated(float value)
{
    if (value<0 || value>253)
    {
        printf("DynamixelServo (SetIDFormated): %.0f Value is invalid...\n", value);
        return false;
    }
    return SetID(int(value));
};
bool DynamixelServo::SetBaudRateFormated(float value)
{
    if (value > GetModelBaudRateMaxFormated())
    {
        printf("DynamixelServo (SetBaudRateFormated): %.0f Value is invalid...\n", value);
        return false;
    }
    CheckBaudRate(int(value));
    
    switch (int (value)) {
        case 2250000:
            value = 250;
            break;
        case 2500000:
            value = 251;
            break;
        case 3000000:
            value = 252;
            break;
        case 3500000:
            value = 253; // Guessing
            break;
            
        default:
            value = 2000000/float(value) -1;
            break;
    }
    
    return SetBaudRate(int(value));
};
bool DynamixelServo::SetReturnDelayTimeFormated(float value) //ms
{
    if (value<0 || value>508)
    {
        printf("DynamixelServo (SetReturnDelayTimeFormated): %.0f Value is invalid...\n", value);
        return false;
    }
    return SetReturnDelayTime(int(value/2));
};
bool DynamixelServo::SetCWAngleLimitFormated(float value, int angle_unit)
{
    // Convert to internal representation (degrees)
    float valueD = ConvertAngleToAngle(value, angle_unit, 0);
    
    if (valueD < GetModelGapAngleFormated(0)/2 || valueD > (360-GetModelGapAngleFormated(0)/2))
    {
        printf("DynamixelServo (SetCWAngleLimitFormated): %f Value is invalid...\n", value);
        return false;
    }
    
    // Convert real angle to dynamixel position
    int DynPos = ConvertToPosition(valueD, angle_unit);
    
    return SetCWAngleLimit(DynPos);
    
};
bool DynamixelServo::SetCCWAngleLimitFormated(float value, int angle_unit)
{
    // Convert to internal representation (degrees)
    float valueD = ConvertAngleToAngle(value, angle_unit, 0);
    
    if (valueD < GetModelGapAngleFormated(0)/2 || valueD > (360-GetModelGapAngleFormated(0)/2))
    {
        printf("DynamixelServo (SetCCWAngleLimitFormated): %f Value is invalid...\n", value);
        return false;
    }
    
    // Convert real angle to dynamixel position
    int DynPos = ConvertToPosition(valueD, angle_unit);
    
    return SetCCWAngleLimit(DynPos);
};
bool DynamixelServo::SetDriveModeFormated(float value)
{
    if (value<0 || value>1)
    {
        printf("DynamixelServo (SetDriveModeFormated): %.0f Value is invalid...\n", value);
        return false;
    }
    return SetDriveMode(int(value));
};
bool DynamixelServo::SetHighestLimitTemperatureFormated(float value)
{
    if (value<10 || value>99)
    {
        printf("DynamixelServo (SetHighestLimitTemperatureFormated): %.0f Value is invalid...\n", value);
        return false;
    }
    return SetHighestLimitTemperature(int(value));
};
bool DynamixelServo::SetHighestLimitVoltageFormated(float value)
{
    if (value<5 || value>25)
    {
        printf("DynamixelServo (SetHighestLimitVoltageFormated): %.0f Value is invalid...\n", value);
        return false;
    }
    return SetHighestLimitVoltage(int(value*10));
};
bool DynamixelServo::SetLowestLimitVoltageFormated(float value)
{
    if (value<5 || value>25)
    {
        printf("DynamixelServo (SetLowestLimitVoltageFormated): %.0f Value is invalid...\n", value);
        return false;
    }
    return SetLowestLimitVoltage(int(value*10));
};
bool DynamixelServo::SetMaxTorqueFormated(float value, int angle_unit)
{
    if (value<0 || value>1)
    {
        printf("DynamixelServo (SetMaxTorqueFormated): %.0f Value is invalid...\n", value);
        return false;
    }
    // Convert 0-1 to 0-1024
    return SetMaxTorque(int(value*float(GetModelTorqueMax())));
}
bool DynamixelServo::SetStatusReturnLevelFormated(float value)
{
    if (value<0 || value>3)
    {
        printf("DynamixelServo (SetStatusReturnLevelFormated): %f Value is invalid...\n", value);
        return false;
    }
    return SetStatusReturnLevel(int(value));
};

bool DynamixelServo::SetAlarmLEDFormated(float value)
{
    if (value<0 || value>255)
    {
        printf("DynamixelServo (SetAlarmLEDFormated): %f Value is invalid...\n", value);
        return false;
    }
    return SetAlarmLED(int(value));
};
bool DynamixelServo::SetAlarmShutdownFormated(float value)
{
    if (value<0 || value>255)
    {
        printf("DynamixelServo (SetAlarmShutdownFormated): %f Value is invalid...\n", value);
        return false;
    }
    return SetAlarmShutdown(int(value));
};
bool DynamixelServo::SetTorqueEnableFormated(float value)
{
    if (value<0 || value>1)
    {
        printf("DynamixelServo (SetTorqueEnableFormated): %f Value is invalid...\n", value);
        return false;
    }
    return SetTorqueEnable(int(value));
};
bool DynamixelServo::SetLEDFormated(float value)
{
    if (value<0 || value>1)
    {
        printf("DynamixelServo (SetLEDFormated): %f Value is invalid...\n", value);
        return false;
    }
    return SetLED(int(value));
};
bool DynamixelServo::SetDGainFormated(float value)
{
    if (GetModelDGainEnable() == 0)
    {
        return false;
    }
    if (value<0 || value>254)
    {
        printf("DynamixelServo (SetDGainFormated): %f Value is invalid...\n", value);
        return false;
    }
    
    return SetDGain(int(value));
    
};
bool DynamixelServo::SetIGainFormated(float value)
{
    if (GetModelIGainEnable() == 0)
    {
        return false;
    }
    if (value<0 || value>254)
    {
        printf("DynamixelServo (SetIGainFormated): %f Value is invalid...\n", value);
        return false;
    }
    return SetIGain(int(value));
    
};
bool DynamixelServo::SetPGainFormated(float value)
{
    if (GetModelPGainEnable() == 0)
    {
        return false;
    }
    if (value<0 || value>254)
    {
        printf("DynamixelServo (SetPGainFormated): %f Value is invalid...\n", value);
        return false;
    }
    if (GetModelPGainEnable() == 1)
    {
        return SetPGain(int(value));
    }
    else
        return false;
};
bool DynamixelServo::SetGoalPositionFormated(float value, int angle_unit)
{
    // Convert to internal representation (degrees)
    float valueD = ConvertAngleToAngle(value, angle_unit, 0);
    
    if (valueD < GetModelGapAngleFormated(0)/2 || valueD > (360-GetModelGapAngleFormated(0)/2))
    {
        printf("DynamixelServo (SetGoalPositionFormated): %f Value is invalid...\n", value);
        return false;
    }
    
    // Convert real angle to dynamixel position
    int DynPos = ConvertToPosition(valueD, angle_unit);
    
    //printf("\n\nSetGoalPositionFormated  value %f valueD %f DynPos %i\n\n", value, valueD, DynPos);
    
    return SetGoalPosition(DynPos);
};
// In wheelmode value is -1 to 1. In Joint mode 0 to 1.
bool DynamixelServo::SetMovingSpeedFormated(float value)
{
    // In wheel mode
    if (!atoi(controlTable.Get("Joint Mode")))
    {
        if (value<-1 || value > 1)
        {
            printf("DynamixelServo (SetMovingSpeedFormated): %f Value is invalid...\n", value);
            return false;
        }
        if (value >= 0) // CW
            return SetMovingSpeed(int(value * float(GetModelSpeedMax())));
        else // CCW
            return SetMovingSpeed(GetModelSpeedMax() + int(-value * float(GetModelSpeedMax())));
    }
    else // Joint mode
    {
        if (value<0 || value > 1)
        {
            printf("DynamixelServo (SetMovingSpeedFormated): %f Value is invalid...\n", value);
            return false;
        }
        return SetMovingSpeed(int(value * float(GetModelSpeedMax())));
    }
};
bool DynamixelServo::SetTorqueLimitFormated(float value)
{
    if (value<0 || value > 1)
    {
        printf("DynamixelServo (SetTorqueLimitFormated): %f Value is invalid...\n", value);
        return false;
    }
    return SetTorqueLimit(int(value * float(GetModelTorqueMax())));
};
// ...
bool DynamixelServo::SetLockFormated(float value)
{
    if (value<0 || value > 1)
    {
        printf("DynamixelServo (SetLockFormated): %f Value is invalid...\n", value);
        return false;
    }
    return SetLock(int(value));
};
bool DynamixelServo::SetPunchFormated(float value)
{
    if (value<0 || value > 1)
    {
        printf("DynamixelServo (SetPunchFormated): %f Value is invalid...\n", value);
        return false;
    }
    return SetPunch(int(value*1023.0f));
};


// MARK: GET

// Model specific values
int DynamixelServo::GetModelPositionMax()
{
    return (atoi(controlTable.Get("Model Position Max")));
}
int DynamixelServo::GetModelSpeedMax()
{
    return (atoi(controlTable.Get("Model Speed Max")));
}
int DynamixelServo::GetModelTorqueMax()
{
    return (atoi(controlTable.Get("Model Torque Max")));
}
int DynamixelServo::GetModelLoadMax()
{
    return (atoi(controlTable.Get("Model Load Max")));
}
int DynamixelServo::GetModelAngleMax()
{
    return (atoi(controlTable.Get("Model Angle Max")));
}
int DynamixelServo::GetModelBaudRateMax()
{
    return (atoi(controlTable.Get("Model Baud Rate Max")));
}
int DynamixelServo::GetModelRecomendedVoltage()
{
    return (atoi(controlTable.Get("Model Recomended Voltage")));
}
int DynamixelServo::GetModelStandbyCurrent()
{
    return (atoi(controlTable.Get("Model Standby Current")));
}
int DynamixelServo::GetModelDGainEnable()
{
    return (atoi(controlTable.Get("Model D Gain Enable")));
}
int DynamixelServo::GetModelIGainEnable()
{
    return (atoi(controlTable.Get("Model I Gain Enable")));
}
int DynamixelServo::GetModelPGainEnable()
{
    return (atoi(controlTable.Get("Model P Gain Enable")));
}
int DynamixelServo::GetModelCurrentEnable()
{
    return (atoi(controlTable.Get("Model Current Enable")));
}
int DynamixelServo::GetModelGapPosition()
{
    return (atoi(controlTable.Get("Gap Estimated Position Range")));
}
// Control table
int DynamixelServo::GetModelNumber()
{
    return (atoi(controlTable.Get("Model Number")));
}
int DynamixelServo::GetFirmware()
{
    return (atoi(controlTable.Get("Version Firmware")));
}
int DynamixelServo::GetID()
{
    return (atoi(controlTable.Get("ID")));
}
int DynamixelServo::GetBaudRate()
{
    return (atoi(controlTable.Get("Baud Rate")));
}
int DynamixelServo::GetReturnDelayTime()
{
    return (atoi(controlTable.Get("Return Delay Time")));
}
int DynamixelServo::GetCWAngleLimit()
{
    return (atoi(controlTable.Get("CW Angle Limit")));
}
int DynamixelServo::GetCCWAngleLimit()
{
    return (atoi(controlTable.Get("CCW Angle Limit")));
}
int DynamixelServo::GetDriveMode()
{
    return (atoi(controlTable.Get("Driver Mode")));
}
int DynamixelServo::GetHighestLimitTemperature()
{
    return (atoi(controlTable.Get("Highest Limit Temperature")));
}
int DynamixelServo::GetHighestLimitVoltage()
{
    return (atoi(controlTable.Get("Highest Limit Voltage")));
}
int DynamixelServo::GetLowestLimitVoltage()
{
    return (atoi(controlTable.Get("Lowest Limit Voltage")));
}
int DynamixelServo::GetMaxTorque()
{
    return (atoi(controlTable.Get("Max Torque")));
}
int DynamixelServo::GetStatusReturnLevel()
{
    return (atoi(controlTable.Get("Status Return Level")));
}
int DynamixelServo::GetAlarmLED()
{
    return (atoi(controlTable.Get("Alarm LED")));
}
int DynamixelServo::GetAlarmShutdown()
{
    return (atoi(controlTable.Get("Alarm Shutdown")));
}
int DynamixelServo::GetTorqueEnable()
{
    return atoi(controlTable.Get("Torque Enable"));
}
int DynamixelServo::GetLED()
{
    return atoi(controlTable.Get("LED"));
}
int DynamixelServo::GetDGain()
{
    return atoi(controlTable.Get("D Gain"));
}
int DynamixelServo::GetIGain()
{
    return atoi(controlTable.Get("I Gain"));
}
int DynamixelServo::GetPGain()
{
    return atoi(controlTable.Get("P Gain"));
}
int DynamixelServo::GetGoalPosition()
{
    return atoi(controlTable.Get("Goal Position"));
}
int DynamixelServo::GetMovingSpeed()
{
    return atoi(controlTable.Get("Moving Speed"));
}
int DynamixelServo::GetTorqueLimit()
{
    return atoi(controlTable.Get("Torque Limit"));
}
int DynamixelServo::GetPresentPosition()
{
    return atoi(controlTable.Get("Present Position"));
}
int DynamixelServo::GetPresentSpeed()
{
    return atoi(controlTable.Get("Present Speed"));
}
int DynamixelServo::GetPresentLoad()
{
    return atoi(controlTable.Get("Present Load"));
}
int DynamixelServo::GetPresentVoltage()
{
    return atoi(controlTable.Get("Present Voltage"));
}
int DynamixelServo::GetPresentTemperature()
{
    return atoi(controlTable.Get("Present Temperature"));
}
int DynamixelServo::GetRegistered()
{
    return (atoi(controlTable.Get("Registered")));
}
int DynamixelServo::GetMoving()
{
    return (atoi(controlTable.Get("Moving")));
}
int DynamixelServo::GetLock()
{
    return (atoi(controlTable.Get("Lock")));
}
int DynamixelServo::GetPunch()
{
    return (atoi(controlTable.Get("Punch")));
}
int DynamixelServo::GetCurrent()
{
    return atoi(controlTable.Get("Current"));
}

// MARK: GET FORMATED

float DynamixelServo::GetModelGapAngleFormated(int angle_unit)
{
    float angle = 360 - GetModelAngleMax(); // GetModelAngleMax in degrees
    switch(angle_unit)
    {
        case 0:
        default: return angle;
        case 1: return angle/360*2.0*pi;
        case 2: return angle/360;
    }
    return 0;
}
float DynamixelServo::GetModelBaudRateMaxFormated()
{
    return float(GetModelBaudRateMax()*1000000);
}

// Control Table
float DynamixelServo::GetModelNumberFormated()
{
    return float(GetModelNumber());
}
float DynamixelServo::GetFirmwareFormated()
{
    return float(GetFirmware());
}
float DynamixelServo::GetIDFormated()
{
    return float(GetID());
}
float DynamixelServo::GetBaudRateFormated()
{
    int raw = GetBaudRate();
    
    if (raw < 250)
    {
        return (2000000/(raw+1));
    }
    else
    {
        switch (raw) {
            case 250:
                return (2250000.0f);
                break;
            case 251:
                return (2500000.0f);
                break;
            case 252:
                return (3000000.0f);
                break;
                // MX-series is suppose to handle up to 4.5 Mpbs but there is no documentation on the webpage. Check code later..
                
            default:
                break;
        }
    }
    return float();
}
float DynamixelServo::GetReturnDelayTimeFormated()
{
    return float(GetReturnDelayTime()*2);
}
float DynamixelServo::GetCWAngleLimitFormated(int angle_unit)
{
    return float(ConvertToAngle(GetCWAngleLimit(), angle_unit));
}
float DynamixelServo::GetCCWAngleLimitFormated(int angle_unit)
{
    return float(ConvertToAngle(GetCCWAngleLimit(), angle_unit));
}
float DynamixelServo::GetDriveModeFormated()
{
    return float(GetDriveMode());
}
float DynamixelServo::GetHighestLimitTemperatureFormated()
{
    return float(GetHighestLimitTemperature());
}
float DynamixelServo::GetHighestLimitVoltageFormated()
{
    return float(GetHighestLimitVoltage()*0.1);
}
float DynamixelServo::GetLowestLimitVoltageFormated()
{
    return float(GetLowestLimitVoltage()*0.1);
}
float DynamixelServo::GetMaxTorqueFormated()
{
    return float(GetMaxTorque()/float(atoi((controlTable.Get("Model Torque Max")))));
}
float DynamixelServo::GetStatusReturnLevelFormated()
{
    return float(GetStatusReturnLevel());
}
float DynamixelServo::GetAlarmLEDFormated()
{
    return float(GetAlarmLED());
}
float DynamixelServo::GetAlarmShutdownFormated()
{
    return float(GetAlarmShutdown());
}
float DynamixelServo::GetTorqueEnableFormated()
{
    return float(GetTorqueEnable());
}
float DynamixelServo::GetLEDFormated()
{
    return float(GetLED());
}
float DynamixelServo::GetDGainFormated()
{
    if (atoi(controlTable.Get("Model D Gain Enable")) == 1)
        return float(GetDGain());
    else
        return float(-1);
}
float DynamixelServo::GetIGainFormated()
{
    if (atoi(controlTable.Get("Model I Gain Enable")) == 1)
        return float(GetIGain());
    else
        return float(-1);
}
float DynamixelServo::GetPGainFormated()
{
    if (atoi(controlTable.Get("Model P Gain Enable")) == 1)
        return float(GetPGain());
    else
        return float(-1);
}
float DynamixelServo::GetGoalPositionFormated(int angle_unit)
{
    return float (ConvertToAngle(GetGoalPosition(), angle_unit));
}
float DynamixelServo::GetMovingSpeedFormated()
{
    // If joint mode
    if (atoi(controlTable.Get("Joint Mode")) == 1)
    {
        return float(GetMovingSpeed())/float(GetModelSpeedMax());
    }
    else
        if (GetMovingSpeed() > 1023) // CCW
            return float(GetModelSpeedMax()-GetMovingSpeed())/float(GetModelSpeedMax());
        else // CW
            return float(GetMovingSpeed())/float(GetModelSpeedMax());
}
float DynamixelServo::GetTorqueLimitFormated()
{
    return (float(GetTorqueLimit())/float(GetModelTorqueMax()));
}
float DynamixelServo::GetPresentPositionFormated(int angle_unit)
{
    // Get dynamixel positon in dynamixel angle
    int posD = GetPresentPosition();
    
    // Convert pos to angle
    float a = ConvertToAngle(posD, angle_unit);
    
    // Convert angle to wanted angle
    float angle = ConvertAngleToAngle(a, 0, angle_unit);
    
    //printf("\n\nGetPresentPositionFormated: posD %i a %f angle %f\n\n", posD, a, angle);
    return angle;
}
float DynamixelServo::GetPresentSpeedFormated()
{
    
    if (GetPresentSpeed() > 1023) // CCW
        return float(GetModelSpeedMax()-GetPresentSpeed())/float(GetModelSpeedMax());
    else // CW
        return float(GetPresentSpeed())/float(GetModelSpeedMax());
    
    
}
float DynamixelServo::GetPresentLoadFormated()
{
    if (GetPresentLoad() > 1023) // CCW
        return float(GetModelLoadMax()-GetPresentLoad())/float(GetModelLoadMax());
    else // CW
        return float(GetPresentLoad())/float(GetModelLoadMax());
}
float DynamixelServo::GetPresentVoltageFormated()
{
    return GetPresentVoltage()*0.1;
}
float DynamixelServo::GetPresentTemperatureFormated()
{
    return GetPresentTemperature();
}
float DynamixelServo::GetRegisteredFormated()
{
    return float(GetRegistered());
}
float DynamixelServo::GetMovingFormated()
{
    return float(GetMoving());
}
float DynamixelServo::GetLockFormated()
{
    return float(GetLock());
}
float DynamixelServo::GetPunchFormated()
{
    return float(GetPunch())/1023;
}
float DynamixelServo::GetCurrentFormated()
{
    if (atoi(controlTable.Get("Model Current Enable")) == 1)
        return (4.5*(GetCurrent()-2048));
    else
        return float(-1);
}

// MARK: MISC

float DynamixelServo::ConvertAngleToAngle(float angle, int from_angle_unit, int to_angle_unit)
{
    //printf("\n\nConvertAngleToAngle (%i,%i) angle %f ->", from_angle_unit,to_angle_unit, angle);
    // Convert from to degrees
    switch(from_angle_unit)
    {
        default:
        case 0: angle = angle; break;
        case 1: angle = (angle/(2.0*pi))*360; break;
        case 2: angle = angle*360; break;
    }
    //printf("%f ->", angle);
    
    // Convert from to X
    switch(to_angle_unit)
    {
        default:
        case 0: angle = angle;break;
        case 1: angle = angle/360*(2.0*pi);break;
        case 2: angle = angle/360;break;
    }
    //printf("%f\n\n", angle);
    return angle;
}

float
DynamixelServo::ConvertToAngle(int position, int angle_unit)
{
    float angleD = float(position)/float(GetModelPositionMax()) * float(GetModelAngleMax());
    
    // Convert angle to real angle
    float angle = angleD + GetModelGapAngleFormated(0)/2;
    
    //printf("\n\nConvertToPosition angle: %.2f angleD %.2f posD %.0i\n", angle, angleD, position);
    return angle;
}


// Convert a real angle in angle_units into position in dynamixel angle
int
DynamixelServo::ConvertToPosition(float angle, int angle_unit)
{
    // Convert real angle to dynamixel angle MX 0-360 AX 30- 330 (0-300)
    float angleD = angle - GetModelGapAngleFormated(0)/2;
    
    // Calculate a value between 0 - 1 0 to MaxAngle
    float posD = angleD/float(GetModelAngleMax()); // Might be formated..
    
    // Clip just to be sure nothing strange is sent to servo.
    posD = clip(posD, 0, 1);
    
    //printf("\n\nConvertToPosition angle: %.2f angleD %.2f posD %.0f\n", angle, angleD, posD);
    return int(posD*float(GetModelPositionMax()));
}

int
DynamixelServo::CheckBaudRate(int br)
{
    switch (br) {
        case 9600:
            break;
        case 19200:
            break;
        case 57600:
            break;
        case 115200:
            break;
        case 200000:
            break;
        case 250000:
            break;
        case 400000:
            break;
        case 500000:
            break;
        case 1000000:
            break;
        case 2250000:
            break;
        case 2500000:
            break;
        case 3000000:
            break;
            
            //....
        default:
            printf("Dynamixel: Not a standard baud rate.\n");
            printf("Available baud rates:\n");
            
            printf("%15s","9600bps\n");
            printf("%15s","192000bps\n");
            printf("%15s","576000bps\n");
            printf("%15s","1152000bps\n");
            printf("%15s","2000000bps\n");
            printf("%15s","2500000bps\n");
            printf("%15s","4000000bps\n");
            printf("%15s","5000000bps\n");
            printf("%15s","5000000bps\n");
            printf("%15s","5000000bps\n");
            printf("%15s","10000000bps\n");
            printf("%15s","22500000bps\n");
            printf("%15s","25000000bps\n");
            printf("%15s","30000000bps\n");
            printf("%15s","Setting baud rate to 1 Mbps\n");
            br = 1000000;
            break;
    }
    return br;
}

void DynamixelServo::ParseControlTable(unsigned char* inbuf, int from, int to)
{
    char tempBuf[64];
    
    // Custom parameters
    
    switch (inbuf[0]+256*inbuf[1])
    {
        case 320:
            controlTable.Set("Servo Model String",  "MX-106");
            controlTable.Set("Model Position Max",        "4095"); // Conntected to Angle Max
            controlTable.Set("Model Speed Max",           "1023");
            controlTable.Set("Model Torque Max",          "1023");
            controlTable.Set("Model Load Max",            "1023");
            controlTable.Set("Model Angle Max",           "360"); // degrees
            controlTable.Set("Model Baud Rate Max",       "4.5"); // Mbps
            controlTable.Set("Model Recomended Votage",   "12");
            controlTable.Set("Model Standby current",     "100"); // mA
            controlTable.Set("Model P Gain Enable",     "1");
            controlTable.Set("Model I Gain Enable",     "1");
            controlTable.Set("Model D Gain Enable",     "1");
            controlTable.Set("Model Current Enable",    "1");
            break;
        case 340:
            controlTable.Set("Servo Model String",  "MX-64");
            controlTable.Set("Model Position Max",        "4095");
            controlTable.Set("Model Speed Max",           "1023");
            controlTable.Set("Model Torque Max",          "1023");
            controlTable.Set("Model Load Max",            "1023");
            controlTable.Set("Model Angle Max",           "360"); // degrees
            controlTable.Set("Model Baud Rate Max",       "4.5"); // Mbps
            controlTable.Set("Model Recomended Votage",   "12");
            controlTable.Set("Model Standby current",     "100"); // mA
            controlTable.Set("Model P Gain Enable",     "1");
            controlTable.Set("Model I Gain Enable",     "1");
            controlTable.Set("Model D Gain Enable",     "1");
            controlTable.Set("Model Current Enable",    "0");
            break;
        case 29:
            controlTable.Set("Servo Model String",  "MX-28");
            controlTable.Set("Model Position Max",        "4095");
            controlTable.Set("Model Speed Max",           "1023");
            controlTable.Set("Model Torque Max",          "1023");
            controlTable.Set("Model Load Max",            "1023");
            controlTable.Set("Model Angle Max",           "360"); // degrees
            controlTable.Set("Model Baud Rate Max",       "4.5"); // Mbps
            controlTable.Set("Model Recomended Votage",   "12");
            controlTable.Set("Model Standby current",     "100"); // mA
            controlTable.Set("Model P Gain Enable",     "1");
            controlTable.Set("Model I Gain Enable",     "1");
            controlTable.Set("Model D Gain Enable",     "1");
            controlTable.Set("Model Current Enable",    "0");
            break;
            
        case 12:
            controlTable.Set("Servo Model String",  "AX-12");
            controlTable.Set("Model Position Max",        "1023");
            controlTable.Set("Model Speed Max",           "1023");
            controlTable.Set("Model Torque Max",          "1023");
            controlTable.Set("Model Load Max",            "1023");
            controlTable.Set("Model Angle Max",           "300"); // degrees
            controlTable.Set("Model Baud Rate Max",       "1"); // Mbps
            controlTable.Set("Model Recomended Votage",   "11.1");
            controlTable.Set("Model Standby current",     "0"); // mA
            // Parameters not having comparing to MX-106
            controlTable.Set("Model P Gain Enable",     "0");
            controlTable.Set("Model I Gain Enable",     "0");
            controlTable.Set("Model D Gain Enable",     "0");
            controlTable.Set("Model Current Enable",    "0");
            break;
        case 324:
            controlTable.Set("Servo Model String",  "AX-12W");
            controlTable.Set("Model Position Max",        "1023");
            controlTable.Set("Model Speed Max",           "1023");
            controlTable.Set("Model Torque Max",          "1023");
            controlTable.Set("Model Load Max",            "1023");
            controlTable.Set("Model Angle Max",           "300"); // degrees
            controlTable.Set("Model Baud Rate Max",       "1"); // Mbps
            controlTable.Set("Model Recomended Votage",   "11.1");
            controlTable.Set("Model Standby current",     "0"); // mA
            // Parameters not having comparing to MX-106
            controlTable.Set("Model P Gain Enable",     "0");
            controlTable.Set("Model I Gain Enable",     "0");
            controlTable.Set("Model D Gain Enable",     "0");
            controlTable.Set("Model Current Enable",    "0");
            break;
        case 18:
            controlTable.Set("Servo Model String",  "AX-18F/A");
            controlTable.Set("Model Position Max",        "1023");
            controlTable.Set("Model Speed Max",           "1023");
            controlTable.Set("Model Torque Max",          "1023");
            controlTable.Set("Model Load Max",            "1023");
            controlTable.Set("Model Angle Max",           "300"); // degrees
            controlTable.Set("Model Baud Rate Max",       "1"); // Mbps
            controlTable.Set("Model Recomended Votage",   "11.1");
            controlTable.Set("Model Standby current",     "0"); // mA
            // Parameters not having comparing to MX-106
            controlTable.Set("Model P Gain Enable",     "0");
            controlTable.Set("Model I Gain Enable",     "0");
            controlTable.Set("Model D Gain Enable",     "0");
            controlTable.Set("Model Current Enable",    "0"); // have sensed current adress 56-57
            break;
        case 64:
            controlTable.Set("Servo Model String",  "RX-64");
            controlTable.Set("Model Position Max",        "1023");
            controlTable.Set("Model Speed Max",           "1023");
            controlTable.Set("Model Torque Max",          "1023");
            controlTable.Set("Model Load Max",            "1023");
            controlTable.Set("Model Angle Max",           "300"); // degrees
            controlTable.Set("Model Baud Rate Max",       "1"); // Mbps
            controlTable.Set("Model Recomended Votage",   "14.8");
            controlTable.Set("Model Standby current",     "50"); // mA
            // Parameters not having comparing to MX-106
            controlTable.Set("Model P Gain Enable",     "0");
            controlTable.Set("Model I Gain Enable",     "0");
            controlTable.Set("Model D Gain Enable",     "0");
            controlTable.Set("Model Current Enable",    "0");
            break;
        case 28:
            controlTable.Set("Servo Model String",  "RX-28");
            controlTable.Set("Model Position Max",        "1023");
            controlTable.Set("Model Speed Max",           "1023");
            controlTable.Set("Model Torque Max",          "1023");
            controlTable.Set("Model Load Max",            "1023");
            controlTable.Set("Model Angle Max",           "300"); // degrees
            controlTable.Set("Model Baud Rate Max",       "1"); // Mbps
            controlTable.Set("Model Recomended Votage",   "14.8");
            controlTable.Set("Model Standby current",     "50"); // mA
            // Parameters not having comparing to MX-106
            controlTable.Set("Model P Gain Enable",     "0");
            controlTable.Set("Model I Gain Enable",     "0");
            controlTable.Set("Model D Gain Enable",     "0");
            controlTable.Set("Model Current Enable",    "0");
            break;
        case 24:
            controlTable.Set("Servo Model String",  "RX-24F");
            controlTable.Set("Model Position Max",        "1023");
            controlTable.Set("Model Speed Max",           "1023");
            controlTable.Set("Model Torque Max",          "1023");
            controlTable.Set("Model Load Max",            "1023");
            controlTable.Set("Model Angle Max",           "300"); // degrees
            controlTable.Set("Model Baud Rate Max",       "1"); // Mbps
            controlTable.Set("Model Recomended Votage",   "11.1");
            controlTable.Set("Model Standby current",     "50"); // mA
            // Parameters not having comparing to MX-106
            controlTable.Set("Model P Gain Enable",     "0");
            controlTable.Set("Model I Gain Enable",     "0");
            controlTable.Set("Model D Gain Enable",     "0");
            controlTable.Set("Model Current Enable",    "0");
            break;
        case 10:
            controlTable.Set("Servo Model String",  "RX-10");
            controlTable.Set("Model Position Max",        "1023");
            controlTable.Set("Model Speed Max",           "1023");
            controlTable.Set("Model Torque Max",          "1023");
            controlTable.Set("Model Load Max",            "1023");
            controlTable.Set("Model Angle Max",           "300"); // degrees
            controlTable.Set("Model Baud Rate Max",       "1"); // Mbps
            controlTable.Set("Model Recomended Votage",   "11.1");
            controlTable.Set("Model Standby current",     "50"); // mA
            // Parameters not having comparing to MX-106
            controlTable.Set("Model P Gain Enable",     "0");
            controlTable.Set("Model I Gain Enable",     "0");
            controlTable.Set("Model D Gain Enable",     "0");
            controlTable.Set("Model Current Enable",    "0");
            break;
        case 107:
            controlTable.Set("Servo Model String",  "EX-106+");
            controlTable.Set("Model Position Max",        "4095");
            controlTable.Set("Model Speed Max",           "1023");
            controlTable.Set("Model Torque Max",          "1023");
            controlTable.Set("Model Load Max",            "1023");
            controlTable.Set("Model Angle Max",           "251"); // degrees
            controlTable.Set("Model Baud Rate Max",       "1"); // Mbps
            controlTable.Set("Model Recomended Votage",   "14.8");
            controlTable.Set("Model Standby current",     "55"); // mA
            // Parameters not having comparing to MX-106
            controlTable.Set("Model P Gain Enable",     "0");
            controlTable.Set("Model I Gain Enable",     "0");
            controlTable.Set("Model D Gain Enable",     "0");
            controlTable.Set("Model Current Enable",    "0"); // have sensed current adress 56-57
            break;
        case 113:
            controlTable.Set("Servo Model String",  "DX-113");
            controlTable.Set("Model Position Max",        "1023");
            controlTable.Set("Model Speed Max",           "1023");
            controlTable.Set("Model Torque Max",          "1023");
            controlTable.Set("Model Load Max",            "1023");
            controlTable.Set("Model Angle Max",           "300"); // degrees
            controlTable.Set("Model Baud Rate Max",       "1"); // Mbps
            controlTable.Set("Model Recomended Votage",   "11.1");
            controlTable.Set("Model Standby current",     "0"); // mA
            // Parameters not having comparing to MX-106
            controlTable.Set("Model P Gain Enable",     "0");
            controlTable.Set("Model I Gain Enable",     "0");
            controlTable.Set("Model D Gain Enable",     "0");
            controlTable.Set("Model Current Enable",    "0"); // have sensed current adress 56-57
            break;
        case 116:
            controlTable.Set("Servo Model String",  "DX-116");
            controlTable.Set("Model Position Max",        "1023");
            controlTable.Set("Model Speed Max",           "1023");
            controlTable.Set("Model Torque Max",          "1023");
            controlTable.Set("Model Load Max",            "1023");
            controlTable.Set("Model Angle Max",           "300"); // degrees
            controlTable.Set("Model Baud Rate Max",       "1"); // Mbps
            controlTable.Set("Model Recomended Votage",   "14.8");
            controlTable.Set("Model Standby current",     "0"); // mA
            // Parameters not having comparing to MX-106
            controlTable.Set("Model P Gain Enable",     "0");
            controlTable.Set("Model I Gain Enable",     "0");
            controlTable.Set("Model D Gain Enable",     "0");
            controlTable.Set("Model Current Enable",    "0"); // have sensed current adress 56-57
            break;
        case 117:
            controlTable.Set("Servo Model String",  "DX-117");
            controlTable.Set("Model Position Max",        "1023");
            controlTable.Set("Model Speed Max",           "1023");
            controlTable.Set("Model Torque Max",          "1023");
            controlTable.Set("Model Load Max",            "1023");
            controlTable.Set("Model Angle Max",           "300"); // degrees
            controlTable.Set("Model Baud Rate Max",       "1"); // Mbps
            controlTable.Set("Model Recomended Votage",   "14.8");
            controlTable.Set("Model Standby current",     "0"); // mA
            // Parameters not having comparing to MX-106
            controlTable.Set("Model P Gain Enable",     "0");
            controlTable.Set("Model I Gain Enable",     "0");
            controlTable.Set("Model D Gain Enable",     "0");
            controlTable.Set("Model Current Enable",    "0"); // have sensed current adress 56-57
            break;
        default:
            controlTable.Set("Servo Model String",  "UNKNOWN");
            controlTable.Set("Model Position Max",        "1023");
            controlTable.Set("Model Speed Max",           "1023");
            controlTable.Set("Model Torque Max",          "1023");
            controlTable.Set("Model Load Max",            "1023");
            controlTable.Set("Model Angle Max",           "300"); // degrees
            controlTable.Set("Model Baud Rate Max",       "1"); // Mbps
            controlTable.Set("Model Recomended Votage",   "11.1");
            controlTable.Set("Model Standby current",     "0"); // mA
            // Parameters not having comparing to MX-106
            controlTable.Set("Model P Gain Enable",     "0");
            controlTable.Set("Model I Gain Enable",     "0");
            controlTable.Set("Model D Gain Enable",     "0");
            controlTable.Set("Model Current Enable",    "0");
            break;
    }
    
    // Position Gap variables for servoes without 360 degrees
    int ModelAngleMax = atoi(controlTable.Get("Model Angle Max"));
    int ModelPositionMax = atoi(controlTable.Get("Model Position Max"));
    int GapAngle = 360 - ModelAngleMax;
    int GapEstimatedRange = 360/ModelAngleMax*ModelPositionMax;
    
    sprintf(tempBuf, "%d", GapAngle);
    controlTable.Set("Model Angle Gap", tempBuf);
    
    sprintf(tempBuf, "%d", GapEstimatedRange);
    controlTable.Set("Gap Estimated Position Range", tempBuf);
    
    // Current
    if (68 >= from && 69 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[68]+256*inbuf[69]);
        controlTable.Set("Current", tempBuf);
    }
    
    // Missing ...
    
    // Punch
    if (48 >= from && 49 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[48]+256*inbuf[49]);
        controlTable.Set("Punch", tempBuf);
    }
    
    // Lock
    if (47 >= from && 47 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[47]);
        controlTable.Set("Lock", tempBuf);
    }
    
    // Moving
    if (46 >= from && 46 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[46]);
        controlTable.Set("Moving", tempBuf);
    }
    
    // Missing ...
    
    // Registered
    if (44 >= from && 44 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[44]);
        controlTable.Set("Registered", tempBuf);
    }
    
    // Present Temperature
    if (43 >= from && 43 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[43]);
        controlTable.Set("Present Temperature", tempBuf);
    }
    
    // Present Voltage
    if (42 >= from && 42 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[42]);
        controlTable.Set("Present Voltage", tempBuf);
    }
    
    // Present Load
    if (40 >= from && 41 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[40]+256*inbuf[41]);
        controlTable.Set("Present Load", tempBuf);
    }
    
    // Present Speed
    if (38 >= from && 39 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[38]+256*inbuf[39]);
        controlTable.Set("Present Speed", tempBuf);
    }
    
    // Present Position
    if (36 >= from && 37 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[36]+256*inbuf[37]);
        controlTable.Set("Present Position", tempBuf);
    }
    
    // Torque Limit
    if (34 >= from && 35 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[34]+256*inbuf[35]);
        controlTable.Set("Torque Limit", tempBuf);
    }
    
    // Moving Speed
    if (32 >= from && 33 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[32]+256*inbuf[33]);
        controlTable.Set("Moving Speed", tempBuf);
    }
    
    // Goal Position
    if (30 >= from && 31 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[30]+256*inbuf[31]);
        controlTable.Set("Goal Position", tempBuf);
    }
    
    // P
    if (28 >= from && 28 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[28]);
        controlTable.Set("P Gain", tempBuf);
    }
    
    // I
    if (27 >= from && 27 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[27]);
        controlTable.Set("I Gain", tempBuf);
    }
    
    // D
    if (26 >= from && 26 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[26]);
        controlTable.Set("D Gain", tempBuf);
    }
    
    // LED
    if (25 >= from && 25 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[25]);
        controlTable.Set("LED", tempBuf);
    }
    
    // Torque Enable
    if (24 >= from && 24 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[24]);
        controlTable.Set("Torque Enable", tempBuf);
    }
    
    // Missing ...
    
    
    // Alarm Shotdown
    if (18 >= from && 18 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[18]);
        controlTable.Set("Alarm Shutdown", tempBuf);
    }
    
    // Alarm LED
    if (17 >= from && 17 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[17]);
        controlTable.Set("Alarm LED", tempBuf);
    }
    
    // Status Return Level
    if (16 >= from && 16 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[16]);
        controlTable.Set("Status Return Level", tempBuf);
    }
    
    // Max Torque
    if (14 >= from && 15 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[14]+256*inbuf[15]);
        controlTable.Set("Max Torque", tempBuf);
    }
    
    // Limit Voltage
    if (13 >= from && 13 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[13]);
        controlTable.Set("Highest Limit Voltage", tempBuf);
    }
    
    // Limit Voltage
    if (12 >= from && 12 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[12]);
        controlTable.Set("Lowest Limit Voltage", tempBuf);
    }
    
    // Highest Limit Temperature
    if (11 >= from && 11 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[11]);
        controlTable.Set("Highest Limit Temperature", tempBuf);
    }
    
    // Driver mode
    if (10 >= from && 10 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[10]);
        controlTable.Set("Driver Mode", tempBuf);
    }
    // CCW Angle Limit
    if (8 >= from && 9 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[8]+256*inbuf[9]);
        controlTable.Set("CCW Angle Limit", tempBuf);
    }
    
    // CW Angle Limit
    if (6 >= from && 7 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[6]+256*inbuf[7]);
        controlTable.Set("CW Angle Limit", tempBuf);
    }
    
    // Extra parameter. Wheel mode or joint mode. CW Angle Limit == 0 and CCW Angle Limit == 0 => Wheel mode
    if (atoi(controlTable.Get("CCW Angle Limit")) == 0 && atoi(controlTable.Get("CW Angle Limit")) == 0)
        controlTable.Set("Joint Mode", "0");
    else
        controlTable.Set("Joint Mode", "1");
    
    
    // Return Delay Time
    if (5 >= from && 5 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[5]);
        controlTable.Set("Return Delay Time", tempBuf);
    }
    
    // Baudrate
    if (4 >= from && 4 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[4]);
        controlTable.Set("Baud Rate", tempBuf);
    }
    
    // ID
    if (3 >= from && 3 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[3]);
        controlTable.Set("ID", tempBuf);
    }
    
    // Version Firmware
    if (2 >= from && 2 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[2]);
        controlTable.Set("Version Firmware", tempBuf);
    }
    
    // Model Number
    if (0 >= from && 1 <= to )
    {
        sprintf(tempBuf, "%d", inbuf[0]+256*inbuf[1]);
        controlTable.Set("Model Number", tempBuf);
    }
};



