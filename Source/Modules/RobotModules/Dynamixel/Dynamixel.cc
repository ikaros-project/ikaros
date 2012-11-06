//
//	Dynamixel.cc	This file is a part of the IKAROS project
//
//    Copyright (C) 2010-2011  Christian Balkenius
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
#include "DynamixelComm.h"

#include <sstream>
#define SLOW_TORQUE

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
    
    baud_rate = checkBaudRate(GetIntValue("baud_rate"));
    
    com = new DynamixelComm(device, baud_rate);
    if(!com)
    {
        Notify(msg_warning, "Dynamixel serial device \"%s\" could not be opened.", device);
        return;
    }
    use_feedback = GetBoolValue("feedback");
    
    start_up_delay = GetIntValue("start_up_delay");
    
    
    init_print = GetIntValueFromList("printf_info");
    
    index_mode = GetIntValueFromList("index_mode");
    angle_unit = GetIntValueFromList("angle_unit");
    int max_servo_id = GetIntValue("max_servo_id");
    int servo_id_list_size = 0;
    int * servo_id_list = GetIntArray("servo_id", servo_id_list_size);
    
    servo = new DynamixelServo * [256]; // maximum number of servos
    for(int i=0; i<256; i++)
        servo[i] = NULL;
    
    // Alt 1: create a list of servos with the IDs read from servo_id
    
    if(servo_id_list_size > 0 && index_mode == 0)
    {
        for(int i=0; i<servo_id_list_size; i++)
        {
            if(com->Ping(servo_id_list[i]))
            {
                servo[servo_id_list[i]] = new DynamixelServo(com, servo_id_list[i]);
            }
            else
            {
                servo[servo_id_list[i]] = NULL;
                Notify(msg_warning, "Dynamixel servo with ID = %d could not be found\n", servo_id_list[i]);
            }
            if(servo_id_list[i] > size)
                size = servo_id_list[i]+1;
        }
    }
    
    else if(servo_id_list_size > 0 && index_mode == 1)
    {
        for(int i=0; i<servo_id_list_size; i++)
        {
            if(com->Ping(servo_id_list[i]))
            {
                servo[i] = new DynamixelServo(com, servo_id_list[i]);
            }
            else
            {
                servo[i] = NULL;
                Notify(msg_warning, "Dynamixel servo with ID = %d could not be found\n", servo_id_list[i]);
            }
        }
        size = servo_id_list_size;
    }
    
    // Alt 2: scan for servos
    
    else if(index_mode == 0)
    {
        size = 0;
        for(int i=1; i<max_servo_id; i++)
        {
            if(com->Ping(i))
            {
                servo[i] = new DynamixelServo(com, i);
                size = i+1;
            }
            else
                servo[i] = NULL;
        }
    }
    else
    {
        size = 0;
        for(int i=1; i<max_servo_id; i++)
        {
            if(com->Ping(i))
            {
                servo[size] = new DynamixelServo(com, i);
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
    
    // Count servos
    servos = 0;
    for(int i=0; i<size; i++)
        if(servo[i])
            servos++;
    
    // Allocate servo maps and parameter structures
    servo_index  = new int [servos];
    servo_id     = new int [servos];
    
    DynamixelMemoeries = new unsigned char * [servos];
    
    // Connecting memory to servo memory
    for(int i=0; i<size; i++)
        if(servo[i])
            DynamixelMemoeries[i] = servo[i]->DynamixelMemory;
    
    // filling servo_index and servo_id. Example: servo_index [1 2 3 4 5] and servo_id[3 2 10 1 3]
    int j=0;
    for(int i=0; i<size; i++)
        if(servo[i])
        {
            servo_index[i] = j++;
            servo_id[i] = atoi(servo[i]->controlTable.Get("ID"));
        }
    
    // If we did not find any servos kill ikaros.
    if (size == 0)
        Notify(msg_fatal_error, "Did not find any Dynamixel servos.\n\n");

}



void
Dynamixel::Print()
{
    printf("Size: %d\n", size);
    printf("Servos: %d\n\n", servos);
    printf("ix  ID  Model     Mode        MaxPos    MaxSpeed   MaxAngle \n");
    printf("------------------------------------------------------------\n");
    for(int i=0; i<size; i++)
        if(servo[i])
        {
            printf("%-2d  ", i);
            printf("%3d ", atoi(servo[i]->controlTable.Get("ID")));
            printf("%-10s", servo[i]->controlTable.Get("Servo Model String"));
            printf("%-6s", atoi(servo[i]->controlTable.Get("Model Position Max")) ? "Joint Mode" : "Wheel Mode" );
            printf("  %4d    ", atoi(servo[i]->controlTable.Get("Model Position Max")));
            printf("  %4d   ", atoi(servo[i]->controlTable.Get("Model Speed Max")));
            printf(" %6.1d   ", atoi(servo[i]->controlTable.Get("Model Angle Max")));
            printf("\n");
        }
    printf("\n");
}

void
Dynamixel::PrintAll()
{
    printf("\nDYNAMIXEL:\n\n");
    
    printf("Size of ikaros input array: %d\n", size);
    printf("Number of servos: %d\n", servos);
    
    printf("\nMODEL SPECIFIC:");
    
    printf("%-28s","\nModel Number:");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%14s"		, servo[i]->controlTable.Get("Servo Model String"));
    
    printf("%-28s","\nModel Position Max:");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%14s"		, servo[i]->controlTable.Get("Model Position Max"));
    
    printf("%-28s","\nModel Speed Max:");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%14s"		, servo[i]->controlTable.Get("Model Speed Max"));
    
    printf("%-28s","\nModel Torque Max:");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%14s"		, servo[i]->controlTable.Get("Model Torque Max"));
    
    printf("%-28s","\nModel Load Max:");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%14s"		, servo[i]->controlTable.Get("Model Load Max"));
    
    printf("%-28s","\nModel Angle Max (deg):");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%14s"		, servo[i]->controlTable.Get("Model Angle Max"));
    
    printf("\n\nCONTROL TABLE (formated | raw ): ");
    
    printf("%-28s","\nModel Number:");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetModelNumberFormated(), servo[i]->GetModelNumber());
    
    printf("%-28s","\nID:");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetIDFormated(), servo[i]->GetID());
    
    printf("%-28s","\nBaud Rate:");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetBaudRateFormated(), servo[i]->GetBaudRate());
    
    
    printf("%-28s","\nReturn Delay Time (us): ");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetReturnDelayTimeFormated(), servo[i]->GetReturnDelayTime());
    
    printf("%-28s","\nCW Angle Limit: ");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetCWAngleLimitFormated(angle_unit), servo[i]->GetCWAngleLimit());
    
    printf("%-28s","\nCCW Angle Limit: ");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetCCWAngleLimitFormated(angle_unit), servo[i]->GetCCWAngleLimit());
    
    printf("%-28s","\nDriver Mode: ");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetDriveModeFormated(), servo[i]->GetDriveMode());
    
    printf("%-28s","\nLimit Temperature (C): ");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetHighestLimitTemperatureFormated(), servo[i]->GetHighestLimitTemperature());
    
    printf("%-28s","\nHighest Limit Voltage (V):");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetHighestLimitVoltageFormated(), servo[i]->GetHighestLimitVoltage());
    
    printf("%-28s","\nLowest Limit Voltage (V):");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetLowestLimitVoltageFormated(), servo[i]->GetLowestLimitVoltage());
    
    printf("%-28s","\nMax Torque (%): ");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetMaxTorqueFormated(), servo[i]->GetMaxTorque());
    
    printf("%-28s","\nStatus Return Level: ");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetStatusReturnLevelFormated(), servo[i]->GetStatusReturnLevel());
    
    printf("%-28s","\nAlarm LED: ");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetAlarmLEDFormated(), servo[i]->GetAlarmLED());
    
    printf("%-28s","\nAlarm Shutdown: ");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetAlarmShutdownFormated(), servo[i]->GetAlarmShutdown());
    
    printf("%-28s","\nTorque Enable: ");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetTorqueEnableFormated(), servo[i]->GetTorqueEnable());
    
    printf("%-28s","\nLED: ");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetLEDFormated(), servo[i]->GetLED());
    
    printf("%-28s","\nD Gain:  ");
    for(int i=0; i<size; i++)
        if(servo[i])
        {
            if (!servo[i]->GetModelDGainEnable())
            {
                printf("%8s |%4i"		, "NA", servo[i]->GetDGain());
            }
            else
            {
                printf("%8.0f |%4i"		, servo[i]->GetDGainFormated(), servo[i]->GetDGain());
            }
        }
    printf("%-28s","\nI Gain:  ");
    for(int i=0; i<size; i++)
        if(servo[i])
        {
            if (!servo[i]->GetModelIGainEnable())
            {
                printf("%8s |%4i"		, "NA", servo[i]->GetIGain());
            }
            else
            {
                printf("%8.0f |%4i"		, servo[i]->GetIGainFormated(), servo[i]->GetIGain());
            }
        }
    printf("%-28s","\nP Gain: ");
    for(int i=0; i<size; i++)
        if(servo[i])
        {
            if (!servo[i]->GetModelPGainEnable())
            {
                printf("%8s |%4i"			, "NA", servo[i]->GetPGain());
            }
            else
            {
                printf("%8.0f |%4i"		, servo[i]->GetPGainFormated(), servo[i]->GetPGain());
            }
        }
    
    printf("%-28s","\nGoal Position:");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetGoalPositionFormated(angle_unit), servo[i]->GetGoalPosition());
    
    printf("%-28s","\nMoving Speed:");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetMovingSpeedFormated(), servo[i]->GetMovingSpeed());
    
    
    printf("%-28s","\nTorque Limit: ");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetTorqueLimitFormated(), servo[i]->GetTorqueLimit());
    
    printf("%-28s","\nPresent Position: ");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetPresentPositionFormated(angle_unit), servo[i]->GetPresentPosition());
    
    printf("%-28s","\nPresent Speed: ");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetPresentSpeedFormated(), servo[i]->GetPresentSpeed());
    
    printf("%-28s","\nPresent Load: ");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetPresentLoadFormated(), servo[i]->GetPresentLoad());
    
    printf("%-28s","\nPresent Voltage: ");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetPresentVoltageFormated(), servo[i]->GetPresentVoltage());
    
    printf("%-28s","\nPresent Temperature: ");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetPresentTemperatureFormated(), servo[i]->GetPresentTemperature());
    
    printf("%-28s","\nRegistered Instruction: ");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetRegisteredFormated(), servo[i]->GetRegistered());
    
    printf("%-28s","\nMoving: ");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetMovingFormated(), servo[i]->GetMoving());
    
    printf("%-28s","\nLock: ");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetLockFormated(), servo[i]->GetLock());
    
    printf("%-28s","\nPunch: ");
    for(int i=0; i<size; i++)
        if(servo[i])
            printf("%8.0f |%4i"		, servo[i]->GetPunchFormated(), servo[i]->GetPunch());
    
    
    printf("%-28s","\nCurrent: ");
    for(int i=0; i<size; i++)
        if(servo[i])
            if(servo[i])
            {
                if (!servo[i]->GetModelCurrentEnable())
                {
                    printf("%8s |%4s"		, "NA", "NA");;
                }
                else
                {
                    printf("%8.0f |%4i"		, servo[i]->GetCurrentFormated(), servo[i]->GetCurrent());
                }
            }
    
    printf("\n\n");
}

void
Dynamixel::SetSizes()
{
    SetOutputSize("FEEDBACK_TORQUE_ENABLE", size);
    SetOutputSize("FEEDBACK_LED", size);
    SetOutputSize("FEEDBACK_D_GAIN", size);
    SetOutputSize("FEEDBACK_I_GAIN", size);
    SetOutputSize("FEEDBACK_P_GAIN", size);
    SetOutputSize("FEEDBACK_GOAL_POSITION", size);
    SetOutputSize("FEEDBACK_MOVING_SPEED", size);
    SetOutputSize("FEEDBACK_TORQUE_LIMIT", size);
    SetOutputSize("FEEDBACK_PRESENT_POSITION", size);
    SetOutputSize("FEEDBACK_PRESENT_SPEED", size);
    SetOutputSize("FEEDBACK_PRESENT_LOAD", size);
    SetOutputSize("FEEDBACK_PRESENT_VOLTAGE", size);
    SetOutputSize("FEEDBACK_PRESENT_TEMPERATURE", size);
    SetOutputSize("FEEDBACK_PRESENT_CURRENT", size);
}

void
Dynamixel::Init()
{
    // Inputs
    
    torqueEnable	= GetInputArray("TORQUE_ENABLE", false);
    if (!torqueEnable)
    {
        //printf("Allocating torque enable (%i)\n",size);
        torqueEnable = create_array(size);
        // Filling it with value fetched from servos.
        for(int i=0; i<size; i++)
            if(servo[i])
                torqueEnable[i] =  servo[i]->GetTorqueEnableFormated();
        allocated_torqueEnable = true;
    }
    
    LED	= GetInputArray("LED", false);
    if (!LED)
    {
        //printf("Allocating LED (%i)\n",size);
        LED = create_array(size);
        // Filling it with value fetched from servos.
        for(int i=0; i<size; i++)
            if(servo[i])
                LED[i] =  servo[i]->GetLEDFormated();
        allocated_LED = true;
    }
    
    dGain	= GetInputArray("D_GAIN", false);
    if (!dGain)
    {
        //printf("Allocating dGain (%i)\n",size);
        dGain = create_array(size);
        // Filling it with value fetched from servos.
        for(int i=0; i<size; i++)
            if(servo[i])
                dGain[i] =  servo[i]->GetDGainFormated();
        allocated_dGain = true;
    }
    iGain	= GetInputArray("I_GAIN", false);
    if (!iGain)
    {
        //printf("Allocating iGain (%i)\n",size);
        iGain = create_array(size);
        // Filling it with value fetched from servos.
        for(int i=0; i<size; i++)
            if(servo[i])
                iGain[i] =  servo[i]->GetIGainFormated();
        allocated_iGain = true;
    }
    pGain	= GetInputArray("P_GAIN", false);
    if (!pGain)
    {
        //printf("Allocating pGain (%i)\n",size);
        pGain = create_array(size);
        // Filling it with value fetched from servos.
        for(int i=0; i<size; i++)
            if(servo[i])
                pGain[i] =  servo[i]->GetPGainFormated();
        allocated_pGain = true;
    }
    goalPosition	= GetInputArray("GOAL_POSITION", false);
    if (!goalPosition)
    {
        //printf("Allocating goalPosition (%i)\n",size);
        goalPosition = create_array(size);
        // Filling it with value fetched from servos.
        for(int i=0; i<size; i++)
            if(servo[i])
                goalPosition[i] =  servo[i]->GetGoalPositionFormated(angle_unit);
        allocated_goalPosition = true;
    }
    movingSpeed	= GetInputArray("MOVING_SPEED", false);
    if (!movingSpeed)
    {
        //printf("Allocating movingSpeed (%i)\n",size);
        movingSpeed = create_array(size);
        // Filling it with value fetched from servos.
        for(int i=0; i<size; i++)
            if(servo[i])
                movingSpeed[i] =  servo[i]->GetMovingSpeedFormated();
        allocated_movingSpeed = true;
    }
    
    torqueLimit	= GetInputArray("TORQUE_LIMIT", false);
    if (!torqueLimit)
    {
        //printf("Allocating torqueLimit (%i)\n",size);
        torqueLimit = create_array(size);
        // Filling it with value fetched from servos.
        for(int i=0; i<size; i++)
            if(servo[i])
                torqueLimit[i] =  servo[i]->GetTorqueLimitFormated();
        allocated_torqueLimit = true;
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
    
    // Print to console
    if (init_print == 1)
        Print();
    if (init_print == 2)
        PrintAll();
    
#ifdef SLOW_TORQUE
    
    // Torqueing up the servos. Using the torque from last time used.
    if (init_print == 2 || init_print == 1)
        printf("\nPower up servo(s) to previous used torque)");
    Timer timer;
    int blink = 0;
    for(float j=0; j<=1; j = j + 0.1)
    {
        for(int i=0; i<size; i++)
            if(servo[i])
            {
                servo[i]->SetTorqueLimit(servo[i]->GetTorqueLimit()*j);
                servo[i]->SetLED(blink);
            }
        com->SyncWriteWithIdRange(servo_id, DynamixelMemoeries, 24, 35, size);
        if (init_print == 2 || init_print == 1)
            printf(".");
        
        if (blink == 1)
            blink = 0;
        else
            blink = 1;
        
        timer.Sleep(100); // Blink
        
    }
    for(int i=0; i<size; i++)
    {    if(servo[i])
        servo[i]->SetLED(0);
        com->SyncWriteWithIdRange(servo_id, DynamixelMemoeries, 25, 25, size);
    }
    if (init_print == 2 || init_print == 1)
        printf("Done\n");
#endif
    
}

Dynamixel::~Dynamixel()
{
#ifdef SLOW_TORQUE
    Timer timer;
    // Torqueing down the servos. Using the torque from last time used.
    if (init_print == 2 || init_print == 1)
        printf("\nPower off servo(s)");
    
    int blink = 0;
    // Turn off torque for all servos. Stepwise to zero
    for(float j=1.0f; j>=0; j = j - 0.1)
    {
        for(int i=0; i<size; i++)
            if(servo[i])
            {
                servo[i]->SetTorqueLimit(servo[i]->GetTorqueLimit()*j);
                servo[i]->SetLED(blink);
            }
        com->SyncWriteWithIdRange(servo_id, DynamixelMemoeries, 24, 35, size);
        if (init_print == 2 || init_print == 1)
            printf(".");
        
        if (blink == 1)
            blink = 0;
        else
            blink = 1;
        
        timer.Sleep(100); // Blink
    }
    for(int i=0; i<size; i++)
    {    if(servo[i])
        servo[i]->SetLED(0);
        com->SyncWriteWithIdRange(servo_id, DynamixelMemoeries, 25, 25, size);
    }
#endif
    
    // Delete dynamixel memory buffert
    //for(int i=0; i<size; i++)
    //    if(servo[i])
    //        delete DynamixelMemoeries[i];
    delete DynamixelMemoeries;
    
    if (allocated_torqueEnable)
        destroy_array(torqueEnable);
    if (allocated_LED)
        destroy_array(LED);
    if (allocated_dGain)
        destroy_array(dGain);
    if (allocated_iGain)
        destroy_array(iGain);
    if (allocated_pGain)
        destroy_array(pGain);
    if (allocated_goalPosition)
        destroy_array(goalPosition);
    if (allocated_movingSpeed)
        destroy_array(movingSpeed);
    if (allocated_torqueLimit)
        destroy_array(torqueLimit);
    
    // Free memory
    delete servo_index;
    delete servo_id;
    delete com;
    delete servo; // Also delete servo's memorybuffer
}

void
Dynamixel::Tick()
{
    // check that the device exists and is open
    if(!device)
        return;
    
    if(!com)
        return;
    
    for(int i=0; i<size; i++)
        if(servo[i])
        {
            // Wheel mode. If we are not in Joint mode and we have a Moving Speed input (we have not allacted our own)
            if(!atoi(servo[i]->controlTable.Get("Joint Mode")) && !allocated_movingSpeed)
            {
                // Set servo in Wheel mode.
                servo[i]->SetMovingSpeedFormated(movingSpeed[i]);
            }
            else // Joint mode
            {
                // Set
                servo[i]->SetTorqueEnableFormated(torqueEnable[i]);
                servo[i]->SetLEDFormated(LED[i]);
                servo[i]->SetDGainFormated(dGain[i]);
                servo[i]->SetIGainFormated(iGain[i]);
                servo[i]->SetPGainFormated(pGain[i]);
                servo[i]->SetGoalPositionFormated(goalPosition[i], angle_unit);
                servo[i]->SetMovingSpeedFormated(movingSpeed[i]);
                servo[i]->SetTorqueLimitFormated(torqueLimit[i]);
            }
        }
    
    if(GetTick() >= start_up_delay) // Do not send commands the first few ticks
    {
        // Write changes to serial
        com->SyncWriteWithIdRange(servo_id, DynamixelMemoeries, 24, 35, size);
    }
    
    // Get feedback
    for(int i=0; i<size; i++)
        if(servo[i])
        {
            unsigned char inbuf[256];
            
            if(use_feedback)
            {
                // Get all data from servo.
                if (com->ReadMemoryRange(atoi(servo[i]->controlTable.Get("ID")), inbuf, 0, DYNAMIXEL_EPROM_SIZE + DYNAMIXEL_RAM_SIZE))
                {
                    //printf("GOT GOOD READINGS\n");
                    // Parse the new data
                    servo[i]->ParseControlTable(inbuf, 0, DYNAMIXEL_EPROM_SIZE + DYNAMIXEL_RAM_SIZE);
                    
                }
                else
                {
                    //printf("GOT BAD READINGS\n");
                    
                }
            }
            
            // Fill the ikaros outputs with formated data.
            feedbackTorqueEnable[i] = servo[i]->GetTorqueEnableFormated();
            feedbackLED[i] = servo[i]->GetLEDFormated();
            feedbackDGain[i] = servo[i]->GetDGainFormated();
            feedbackIGain[i] = servo[i]->GetIGainFormated();
            feedbackPGain[i] = servo[i]->GetPGainFormated();
            feedbackGoalPosition[i] = servo[i]->GetGoalPositionFormated(angle_unit);
            feedbackMoving[i] = servo[i]->GetMovingSpeedFormated();
            feedbackTorqueLimit[i] = servo[i]->GetTorqueLimitFormated();
            feedbackPresentPosition[i] = servo[i]->GetPresentPositionFormated(angle_unit);
            feedbackPresentSpeed[i] = servo[i]->GetPresentSpeedFormated();
            feedbackPresentLoad[i] = servo[i]->GetPresentLoadFormated();
            feedbackPresentVoltage[i] =  servo[i]->GetPresentVoltageFormated();
            feedbackPresentTemperature[i] = servo[i]->GetPresentTemperatureFormated();
            feedbackPresentCurrent[i] = servo[i]->GetCurrentFormated();
            
        }
}


int
Dynamixel::checkBaudRate(int br)
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

static InitClass init("Dynamixel", &Dynamixel::Create, "Source/Modules/RobotModules/Dynamixel/");
