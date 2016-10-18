//
//    DynamixelComm.cc
//
//    Copyright (C) 2016  Birger Johansson
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


// When using sync write the maximum size sent total is 143.

#include "DynamixelServo.h"
#include "IKAROS.h"
#include <unistd.h>

using namespace ikaros;

DynamixelServo::DynamixelServo(DynamixelComm *com, int id, const char * csvPath, int forceModel)
{
    protocol = com->Ping(id);
    extraInfo.Set("ID", id);
    
    dynamixelMemory = new unsigned char[DYNAMIXEL_MEM_BUFFER];
    
    // Get a few important features of the servo
    if (!com->ReadMemoryRange(id, protocol, dynamixelMemory, 0, P_CCW_ANGLE_LIMIT+1))
    {
        printf("Dynamixel: Can not read from id %i, Quitting ikaros.... \n", id);
        std::exit(false);
    }
    
    if (forceModel != 0) // Should be used if the memoryblock on the servo is corrupt and servo return a not existing model number.
        model = forceModel;
    else
        model = dynamixelMemory[P_MODEL_NUMBER]+256*dynamixelMemory[P_MODEL_NUMBER+1];
    
    extraInfo.Set("Model", model);

    // Get Additional information about servo
    GetAdditionalInfo(model);
    
    int controlTableSize;
    if (!ReadCSVFileToCtable(controlTable,model,&controlTableSize,csvPath)) // read cvs file with control table and ikaros related IO.
    {
        printf("Dynamixel: Can not read configuration file for dynamixel id %i model %i\n", id, model);
        std::exit(false);
    }

    // Get all memory
    if (!com->ReadMemoryRange(id, protocol, dynamixelMemory, 0, extraInfo.GetInt("Model Memory")))
    {
        printf("Dynamixel: Can not read from id %i, Quitting ikaros.... \n", id);
        std::exit(false);
    }
    
    // Operation mode
    int cw  = dynamixelMemory[P_CW_ANGLE_LIMIT]+256*dynamixelMemory[P_CW_ANGLE_LIMIT+1];
    int ccw = dynamixelMemory[P_CCW_ANGLE_LIMIT]+256*dynamixelMemory[P_CCW_ANGLE_LIMIT+1];
    if (cw == 0 && ccw == 0)
        extraInfo.Set("Operation Type", OP_WHEEL);  // Wheel mode
    else if (cw == extraInfo.GetInt("Model Position Max") && ccw == extraInfo.GetInt("Model Position Max"))
        extraInfo.Set("Operation Type", OP_MULTI_TURN);  // Multi turn mode
    else
        extraInfo.Set("Operation Type", OP_JOINT);  // Joint mode
    return;
}

DynamixelServo::~DynamixelServo()
{
    delete dynamixelMemory;
}

// MARK: Set to memory
bool DynamixelServo::SetValueAtAdress(int adress, int value)
{
    // Check borders.
    if (value < controlTable[adress].Min || value > controlTable[adress].Max)
    {
        printf("DynamixelServo SetValueAtAdress: Value is not OK!\n");
        return false;
    }
    switch (controlTable[adress].Size) {
        case 1:
            dynamixelMemory[adress] = value;
            break;
        case 2:
            dynamixelMemory[adress]   = value % 256;
            dynamixelMemory[adress+1] = value / 256;
            break;
        case 3:
            // Not yet implemented
            break;
        case 4:
            // Not yet implemented
            break;
        default:
            break;
    }
    return (true);
};
// MARK: Get from memory
int DynamixelServo::GetValueAtAdress(int adress)
{
    int value = -1;
    switch (controlTable[adress].Size) {
        case 1:
            value = dynamixelMemory[adress];
            break;
        case 2:
            value = (dynamixelMemory[adress+1]<<8) + dynamixelMemory[adress];
            break;
        case 3:
            // Not yet implemented
            break;
        case 4:
            // Not yet implemented
            break;
        default:
            break;
    }
    return (value);
}
float DynamixelServo::GetValueAtAdressFormated(int adress)
{
    return(float)GetValueAtAdress(adress);
}

// MARK: Get Model specific
int DynamixelServo::GetModelPositionMax()
{
    return (extraInfo.GetInt("Model Position Max"));
}
int DynamixelServo::GetModelSpeedMax()
{
    return (extraInfo.GetInt("Model Speed Max"));
}
int DynamixelServo::GetModelTorqueMax()
{
    return (extraInfo.GetInt("Model Torque Max"));
}
int DynamixelServo::GetModelLoadMax()
{
    return (extraInfo.GetInt("Model Load Max"));
}
int DynamixelServo::GetModelAngleMax()
{
    return (extraInfo.GetInt("Model Angle Max"));
}
int DynamixelServo::GetModelBaudRateMax()
{
    return (extraInfo.GetInt("Model Baud Rate Max"));
}

// MARK: Get formated

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

float DynamixelServo::GetTorqueEnableFormated(int adress)
{
    return (float)GetValueAtAdress(adress);
}
float DynamixelServo::GetLEDFormated(int adress)
{
    return (float)GetValueAtAdress(adress);
}
float DynamixelServo::GetDGainFormated(int adress)
{
    if (adress != -1)
        return (float)GetValueAtAdress(adress);
    else
        return float(-1);
}
float DynamixelServo::GetIGainFormated(int adress)
{
    if (adress != -1)
        return (float)GetValueAtAdress(adress);
    else
        return float(-1);
}
float DynamixelServo::GetPGainFormated(int adress)
{
    if (adress != -1)
        return (float)GetValueAtAdress(adress);
    else
        return float(-1);
}
float DynamixelServo::GetGoalPositionFormated(int adress, int angle_unit)
{
    return float (ConvertToAngle(GetValueAtAdress(adress), angle_unit));
}
float DynamixelServo::GetMovingSpeedFormated(int adress)
{
    if (extraInfo.GetInt("Control Mode") == 1) // Change this?
    {
        return float(GetValueAtAdress(adress))/float(GetModelSpeedMax());
    }
    else
        if (GetValueAtAdress(adress) > 1023) // CCW
            return float(GetModelSpeedMax()-GetValueAtAdress(adress))/float(GetModelSpeedMax());
        else // CW
            return float(GetValueAtAdress(adress))/float(GetModelSpeedMax());
}
float DynamixelServo::GetTorqueLimitFormated(int adress)
{
    return (float(GetValueAtAdress(adress))/float(GetModelTorqueMax()));
}
float DynamixelServo::GetPresentPositionFormated(int adress, int angle_unit)
{
    // Get dynamixel positon in dynamixel angle
    int posD = GetValueAtAdress(adress);
    
    // Convert pos to angle
    float a = ConvertToAngle(posD, angle_unit);
    
    // Convert angle to wanted angle
    float angle = angle_to_angle(a, 0, angle_unit);

    //printf("\n\nGetPresentPositionFormated: posD %i a %f angle %f\n\n", posD, a, angle);
    return angle;
}
float DynamixelServo::GetPresentSpeedFormated(int adress)
{
    // WHEELMODE!
    switch (extraInfo.GetInt("Operation Type")) {
        case OP_WHEEL:
            if (GetValueAtAdress(adress) > 1023) // CCW is this always true? 0-1023 CW 1024-2047 CCW
                return float(GetModelSpeedMax()-GetValueAtAdress(adress))/float(GetModelSpeedMax());
            else // CW
                return float(GetValueAtAdress(adress))/float(GetModelSpeedMax());
        case OP_JOINT:
        case OP_MULTI_TURN:
                return float(GetValueAtAdress(adress))/float(GetModelSpeedMax()); // 0-1023
            break;
        default:
            break;
    }
    return float(GetValueAtAdress(adress))/float(GetModelSpeedMax()); // 0-1023 //
    
    }
float DynamixelServo::GetPresentLoadFormated(int adress)
{
    if (GetValueAtAdress(adress) > 1023) // CCW
        return float(GetModelLoadMax()-GetValueAtAdress(adress))/float(GetModelLoadMax());
    else // CW
        return float(GetValueAtAdress(adress))/float(GetModelLoadMax());
}
float DynamixelServo::GetPresentVoltageFormated(int adress)
{
    return GetValueAtAdress(adress)*controlTable[adress].Convert;
}
float DynamixelServo::GetPresentTemperatureFormated(int adress)
{
    return GetValueAtAdress(adress);
}
float DynamixelServo::GetCurrentFormated(int adress)
{
    return GetValueAtAdress(adress)*controlTable[adress].Convert;
}
float DynamixelServo::GetGoalTorqueFormated(int adress)
{
    return GetValueAtAdress(adress)*controlTable[adress].Convert;
}
float DynamixelServo::GetGoalAccelerationFormated(int adress)
{
    return GetValueAtAdress(adress)*controlTable[adress].Convert;
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

// MARK: Set
bool DynamixelServo::SetTorqueEnableFormated(int adress, float value)
{
    // Do formating stuff...
    return SetValueAtAdress(adress, int(value));
};

bool DynamixelServo::SetLEDFormated(int adress, float value)
{
    return SetValueAtAdress(adress, int(value));
};
bool DynamixelServo::SetDGainFormated(int adress, float value)
{
    return SetValueAtAdress(adress, int(value));
};
bool DynamixelServo::SetIGainFormated(int adress, float value)
{
    return SetValueAtAdress(adress, int(value));
};
bool DynamixelServo::SetPGainFormated(int adress, float value)
{
    return SetValueAtAdress(adress, int(value));
};
bool DynamixelServo::SetGoalPositionFormated(int adress, float value, int angle_unit)
{
    // Convert to internal representation (degrees)
    float valueD = angle_to_angle(value, angle_unit, 0);
    
    if (valueD < GetModelGapAngleFormated(0)/2 || valueD > (360-GetModelGapAngleFormated(0)/2))
    {
        printf("DynamixelServo (SetGoalPositionFormated): %f Value is invalid...\n", value);
        return false;
    }
    
    // Convert real angle to dynamixel position
    int DynPos = ConvertToPosition(valueD, angle_unit);
    return SetValueAtAdress(adress, int(DynPos));
};

// In wheelmode value is -1 to 1. In Control Mode 0 to 1.
bool DynamixelServo::SetMovingSpeedFormated(int adress, float value)
{
    switch (extraInfo.GetInt("Operation Type")) {
        case OP_WHEEL:
            if (value >= 0) // CW
                return SetValueAtAdress(adress, int(value * float(GetModelSpeedMax())));
            else // CCW
                return SetValueAtAdress(adress, int(GetModelSpeedMax() + int(-value * float(GetModelSpeedMax()))));
            break;
        case OP_JOINT:
            return SetValueAtAdress(adress,int(value * float(GetModelSpeedMax())));
            break;
        case OP_MULTI_TURN:
            // Not implemented yet
            break;
            
        default:
            break;
    }
    return false;
};

bool DynamixelServo::SetGoalTorqueFormated(int adress, float value)
{
    return SetValueAtAdress(adress, int(value * float(GetModelTorqueMax())));
};

bool DynamixelServo::SetGoalAccelerationFormated(int adress, float value)
{
    return SetValueAtAdress(adress, int(value * float(GetModelTorqueMax())));
};
bool DynamixelServo::SetTorqueLimitFormated(int adress, float value)
{
    return SetValueAtAdress(adress, int(value * float(GetModelTorqueMax())));
};

void DynamixelServo::GetAdditionalInfo(int model)
{        
    switch (model)
    {
        case 360:
            extraInfo.Set("Servo Model String",    "MX-12W");
            extraInfo.Set("Model Position Max",        4095); // Conntected to Angle Max
            extraInfo.Set("Model Speed Max",           1023);
            extraInfo.Set("Model Torque Max",          1023);
            extraInfo.Set("Model Load Max",            1023);
            extraInfo.Set("Model Angle Max",            360); // degrees
            extraInfo.Set("Model Baud Rate Max",       4.5f); // Mbps
            extraInfo.Set("Model Recomended Votage",  12.0f);
            extraInfo.Set("Model Standby current",       60); // mA
            extraInfo.Set("Model Protocol",               1);
            extraInfo.Set("Model Memory",                73);
            break;
        case 29:
            extraInfo.Set("Servo Model String",       "MX-28");
            extraInfo.Set("Model Position Max",        4095);
            extraInfo.Set("Model Speed Max",           1023);
            extraInfo.Set("Model Torque Max",          1023);
            extraInfo.Set("Model Load Max",            1023);
            extraInfo.Set("Model Angle Max",            360); // degrees
            extraInfo.Set("Model Baud Rate Max",       4.5f); // Mbps
            extraInfo.Set("Model Recomended Votage",  12.0f);
            extraInfo.Set("Model Standby current",      100); // mA
            extraInfo.Set("Model Protocol",               1);
            extraInfo.Set("Model Memory",                73);
            break;
        case 310:
            extraInfo.Set("Servo Model String",     "MX-64");
            extraInfo.Set("Model Position Max",        4095);
            extraInfo.Set("Model Speed Max",           1023);
            extraInfo.Set("Model Torque Max",          1023);
            extraInfo.Set("Model Load Max",            1023);
            extraInfo.Set("Model Angle Max",            360); // degrees
            extraInfo.Set("Model Baud Rate Max",       4.5f); // Mbps
            extraInfo.Set("Model Recomended Votage",  12.0f);
            extraInfo.Set("Model Standby current",      100); // mA
            extraInfo.Set("Model Protocol",               1);
            extraInfo.Set("Model Memory",                73);
            break;
        case 320:
            extraInfo.Set("Servo Model String",      "MX-106");
            extraInfo.Set("Model Position Max",        4095); // Conntected to Angle Max
            extraInfo.Set("Model Speed Max",           1023);
            extraInfo.Set("Model Torque Max",          1023);
            extraInfo.Set("Model Load Max",            1023);
            extraInfo.Set("Model Angle Max",            360); // degrees
            extraInfo.Set("Model Baud Rate Max",       4.5f); // Mbps
            extraInfo.Set("Model Recomended Votage",  12.0f);
            extraInfo.Set("Model Standby current",      100); // mA
            extraInfo.Set("Model Protocol",               1);
            extraInfo.Set("Model Memory",                73);
            break;
            
        case 107:
            extraInfo.Set("Servo Model String",     "EX-106+");
            extraInfo.Set("Model Position Max",          4095);
            extraInfo.Set("Model Speed Max",             1023);
            extraInfo.Set("Model Torque Max",            1023);
            extraInfo.Set("Model Load Max",              1023);
            extraInfo.Set("Model Angle Max",              251); // degrees
            extraInfo.Set("Model Baud Rate Max",         1.0f); // Mbps
            extraInfo.Set("Model Recomended Votage",    14.8f);
            extraInfo.Set("Model Standby current",         55); // mA
            extraInfo.Set("Model Protocol",                 1);
            extraInfo.Set("Model Memory",                  57);
            break;
            
        case 10:
            extraInfo.Set("Servo Model String",  "RX-10");
            extraInfo.Set("Model Position Max",        1023);
            extraInfo.Set("Model Speed Max",           1023);
            extraInfo.Set("Model Torque Max",          1023);
            extraInfo.Set("Model Load Max",            1023);
            extraInfo.Set("Model Angle Max",           300); // degrees
            extraInfo.Set("Model Baud Rate Max",       1.0f); // Mbps
            extraInfo.Set("Model Recomended Votage",   11.1f);
            extraInfo.Set("Model Standby current",     50); // mA
            extraInfo.Set("Model Protocol",           1);
            extraInfo.Set("Model Memory",             49);
            break;
            
        case 24:
            extraInfo.Set("Servo Model String",  "RX-24F");
            extraInfo.Set("Model Position Max",        1023);
            extraInfo.Set("Model Speed Max",           1023);
            extraInfo.Set("Model Torque Max",          1023);
            extraInfo.Set("Model Load Max",            1023);
            extraInfo.Set("Model Angle Max",           300); // degrees
            extraInfo.Set("Model Baud Rate Max",       1.0f); // Mbps
            extraInfo.Set("Model Recomended Votage",   11.1f);
            extraInfo.Set("Model Standby current",     50); // mA
            extraInfo.Set("Model Protocol",           1);
            extraInfo.Set("Model Memory",             49);
            break;
            
        case 28:
            extraInfo.Set("Servo Model String",  "RX-28");
            extraInfo.Set("Model Position Max",        1023);
            extraInfo.Set("Model Speed Max",           1023);
            extraInfo.Set("Model Torque Max",          1023);
            extraInfo.Set("Model Load Max",            1023);
            extraInfo.Set("Model Angle Max",           300); // degrees
            extraInfo.Set("Model Baud Rate Max",       1.0f); // Mbps
            extraInfo.Set("Model Recomended Votage",   14.8f);
            extraInfo.Set("Model Standby current",     50); // mA
            extraInfo.Set("Model Protocol",           1);
            extraInfo.Set("Model Memory",             49);
            break;
            
        case 64:
            extraInfo.Set("Servo Model String",  "RX-64");
            extraInfo.Set("Model Position Max",        1023);
            extraInfo.Set("Model Speed Max",           1023);
            extraInfo.Set("Model Torque Max",          1023);
            extraInfo.Set("Model Load Max",            1023);
            extraInfo.Set("Model Angle Max",           300); // degrees
            extraInfo.Set("Model Baud Rate Max",       1.0f); // Mbps
            extraInfo.Set("Model Recomended Votage",   14.8f);
            extraInfo.Set("Model Standby current",     50); // mA
            extraInfo.Set("Model Protocol",           1);
            extraInfo.Set("Model Memory",             49);
            break;
            
        case 300:
            extraInfo.Set("Servo Model String",  "AX-12W");
            extraInfo.Set("Model Position Max",        1023);
            extraInfo.Set("Model Speed Max",           1023);
            extraInfo.Set("Model Torque Max",          1023);
            extraInfo.Set("Model Load Max",            1023);
            extraInfo.Set("Model Angle Max",           300); // degrees
            extraInfo.Set("Model Baud Rate Max",       1.0f); // Mbps
            extraInfo.Set("Model Recomended Votage",   11.1f);
            extraInfo.Set("Model Standby current",     0); // mA
            extraInfo.Set("Model Protocol",           1);
            extraInfo.Set("Model Memory",             49);
            break;
        case 12:
            extraInfo.Set("Servo Model String",  "AX-12");
            extraInfo.Set("Model Position Max",        1023);
            extraInfo.Set("Model Speed Max",           1023);
            extraInfo.Set("Model Torque Max",          1023);
            extraInfo.Set("Model Load Max",            1023);
            extraInfo.Set("Model Angle Max",           300); // degrees
            extraInfo.Set("Model Baud Rate Max",       1.0f); // Mbps
            extraInfo.Set("Model Recomended Votage",   11.1f);
            extraInfo.Set("Model Standby current",     0); // mA
            extraInfo.Set("Model Protocol",           1);
            extraInfo.Set("Model Memory",             49);
            break;

        case 18:
            extraInfo.Set("Servo Model String",  "AX-18");
            extraInfo.Set("Model Position Max",        1023);
            extraInfo.Set("Model Speed Max",           1023);
            extraInfo.Set("Model Torque Max",          1023);
            extraInfo.Set("Model Load Max",            1023);
            extraInfo.Set("Model Angle Max",           300); // degrees
            extraInfo.Set("Model Baud Rate Max",       1.0f); // Mbps
            extraInfo.Set("Model Recomended Votage",   11.1f);
            extraInfo.Set("Model Standby current",     0); // mA
            extraInfo.Set("Model Protocol",           1);
            extraInfo.Set("Model Memory",             49);
            break;

        case 113:
            extraInfo.Set("Servo Model String",  "DX-113");
            extraInfo.Set("Model Position Max",        1023);
            extraInfo.Set("Model Speed Max",           1023);
            extraInfo.Set("Model Torque Max",          1023);
            extraInfo.Set("Model Load Max",            1023);
            extraInfo.Set("Model Angle Max",           300); // degrees
            extraInfo.Set("Model Baud Rate Max",       1.0f); // Mbps
            extraInfo.Set("Model Recomended Votage",   11.1f);
            extraInfo.Set("Model Standby current",     0); // mA
            extraInfo.Set("Model Protocol",           1);
            extraInfo.Set("Model Memory",             49);
            break;
        case 116:
            extraInfo.Set("Servo Model String",  "DX-116");
            extraInfo.Set("Model Position Max",        1023);
            extraInfo.Set("Model Speed Max",           1023);
            extraInfo.Set("Model Torque Max",          1023);
            extraInfo.Set("Model Load Max",            1023);
            extraInfo.Set("Model Angle Max",           300); // degrees
            extraInfo.Set("Model Baud Rate Max",       1.0f); // Mbps
            extraInfo.Set("Model Recomended Votage",   14.8f);
            extraInfo.Set("Model Standby current",     0); // mA
            extraInfo.Set("Model Protocol",           1);
            extraInfo.Set("Model Memory",             49);
            break;
        case 117:
            extraInfo.Set("Servo Model String",  "DX-117");
            extraInfo.Set("Model Position Max",        1023);
            extraInfo.Set("Model Speed Max",           1023);
            extraInfo.Set("Model Torque Max",          1023);
            extraInfo.Set("Model Load Max",            1023);
            extraInfo.Set("Model Angle Max",           300); // degrees
            extraInfo.Set("Model Baud Rate Max",       1.0f); // Mbps
            extraInfo.Set("Model Recomended Votage",   14.8f);
            extraInfo.Set("Model Standby current",     0); // mA
            extraInfo.Set("Model Protocol",           1);
            extraInfo.Set("Model Memory",             49);
            break;
        case 350:
            extraInfo.Set("Servo Model String",        "XL-320");
            extraInfo.Set("Model Position Max",        1023); // Conntected to Angle Max
            extraInfo.Set("Model Speed Max",           1023);
            extraInfo.Set("Model Torque Max",          1023);
            extraInfo.Set("Model Load Max",            1023);
            extraInfo.Set("Model Angle Max",           300); // degrees
            extraInfo.Set("Model Baud Rate Max",       4.5f); // Mbps
            extraInfo.Set("Model Recomended Votage",   7.6f);
            extraInfo.Set("Model Standby current",     60); // mA
            extraInfo.Set("Model Protocol",           1);
            extraInfo.Set("Model Memory",             52);
            break;
            
        default:
            extraInfo.Set("Servo Model String",  "UNKNOWN");
            extraInfo.Set("Model Position Max",        1023);
            extraInfo.Set("Model Speed Max",           1023);
            extraInfo.Set("Model Torque Max",          1023);
            extraInfo.Set("Model Load Max",            1023);
            extraInfo.Set("Model Angle Max",            360); // degrees
            extraInfo.Set("Model Baud Rate Max",       1.0f); // Mbps
            extraInfo.Set("Model Recomended Votage",  12.0f);
            extraInfo.Set("Model Standby current",        0); // mA
            extraInfo.Set("Model Protocol",               1);
            extraInfo.Set("Model Memory",               100);
            break;
    }
    
    // Position gap variables for servoes without 360 degrees
    int ModelAngleMax = extraInfo.GetInt("Model Angle Max");
    int ModelPositionMax = extraInfo.GetInt("Model Position Max");
    int GapAngle = 360 - ModelAngleMax;
    int GapEstimatedRange = 360/ModelAngleMax*ModelPositionMax;
    extraInfo.Set("Model Angle Gap", GapAngle);
    extraInfo.Set("Gap Estimated Position Range", GapEstimatedRange);
}

// MARK: Read CSV
void readCSV( FILE *fp, std::vector<std::string>& vls )
{
    vls.clear();
    if( ! fp )
        return;
    char buf[10000];
    if( ! fgets( buf,999,fp) )
        return;
    std::string s = buf;
    
    if (s.at(s.length()-1) != '\n') // Numbers do not add a newline char at last line of the csv file.
        s += '\n'; // add a \n if there is none at the end of the line.
    
    long int p,q;
    q = -1;
    // loop over columns
    while( 1 ) {
        p = q;
        q = s.find_first_of(";\n",p+1);
        if( q == -1 )
            break;
        vls.push_back( s.substr(p+1,q-p-1) );
    }
}

bool DynamixelServo::ReadCSVFileToCtable(CT * controlTable,int model, int * size, const char * csvPath)
{
    for (int i =0; i<C_TABLE_SIZE; i++) {
        controlTable[i].Adress = i;
        controlTable[i].Visable = false;
        controlTable[i].IkarosInputs = -1;
        controlTable[i].IkarosOutputs = -1;
        controlTable[i].Convert = -1;
    }
    std::string path = csvPath;;
    std::string filename = path + std::string("DynamixelControlTables/")+std::to_string(model)+std::string("-")+std::to_string(model)+ std::string(".csv");
    //printf("Open file %s\n",filename.c_str());
    std::vector<std::string> vls;
    FILE * fp = fopen(filename.c_str(), "r" );
    if(!fp)
    {
        printf("Can not open file %s",filename.c_str());
        return false;
    }
    
    readCSV(fp, vls); // Read Header
    int i = 0;
    while (1)
    {
        readCSV(fp, vls);
        if (vls.empty())
        {
            *size = i;
            break;
        }
        controlTable[std::stoi(vls[0])].Adress = std::stoi(vls[0]);
        controlTable[std::stoi(vls[0])].Size = std::stoi(vls[1]);
        controlTable[std::stoi(vls[0])].Name = vls[2].c_str();
        controlTable[std::stoi(vls[0])].Description = vls[3].c_str();
        controlTable[std::stoi(vls[0])].Access = vls[4].c_str();
        
        if (!vls[5].empty())
            if (isdigit(vls[5].at(0)) || vls[5].at(0) == '-')
                controlTable[std::stoi(vls[0])].Inital    = std::stoi(vls[5]);
        if (!vls[6].empty())
            if (isdigit(vls[6].at(0)) || vls[6].at(0) == '-')
                controlTable[std::stoi(vls[0])].Min    = std::stoi(vls[6]);
        if (!vls[7].empty())
            if (isdigit(vls[7].at(0)) || vls[7].at(0) == '-')
                controlTable[std::stoi(vls[0])].Max    = std::stoi(vls[7]);
        if (!vls[8].empty())
            if (isdigit(vls[8].at(0)) || vls[8].at(0) == '-')
                controlTable[std::stoi(vls[0])].Convert    = std::stof(vls[8]);
        if (!vls[9].empty())
            if (isdigit(vls[9].at(0)) || vls[9].at(0) == '-')
                controlTable[std::stoi(vls[0])].IkarosInputs    = std::stoi(vls[9]);
        if (!vls[10].empty())
            if (isdigit(vls[10].at(0)) || vls[10].at(0) == '-')
                controlTable[std::stoi(vls[0])].IkarosOutputs    = std::stoi(vls[10]);
        controlTable[std::stoi(vls[0])].Visable = true;   // Used for printing
        i++;
    }
    return true;
}

// MARK: Print
void DynamixelServo::PrintControlTable()
{
    printf("Address\tSize\tName\t\t\t\t\t\t\tAccess\tInital\tMin\t\tMax\t\tConvert\tIN\tOUT\tDescription\n");
    for (int i = 0; i< 128; i++) {
        if (controlTable[i].Visable)
        {
            printf("%-8i",controlTable[i].Adress );
            printf("%-8i",controlTable[i].Size );
            printf("%-32s",controlTable[i].Name.c_str() );
            printf("%-8s",controlTable[i].Access.c_str() );
            printf("%-8i",controlTable[i].Inital );
            printf("%-8i",controlTable[i].Min );
            printf("%-8i",controlTable[i].Max );
            printf("%-8.2f",controlTable[i].Convert );
            printf("%-4i",controlTable[i].IkarosInputs);
            printf("%-4i",controlTable[i].IkarosOutputs);
            printf("%s",controlTable[i].Description.c_str() );
            printf("\n");
        }
    }
}
