//
//	DynamixelConfigure.cc	This file is a part of the IKAROS project
//
//    Copyright (C) 2010-2011  Birger Johansson
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
#include "DynamixelConfigure.h"

#include <sstream>

using namespace ikaros;

DynamixelConfigure::DynamixelConfigure(Parameter * p):
Module(p)
{
    
    // Reset mode
    resetMode = GetBoolValue("reset_mode");
    if (resetMode)
    {
        printf("In RESET MODE\n");
        printf("No other parameters will be used. Only reset command to one servo will be sent\n");
    }
    
    device = GetValue("device");
    if(!device)
    {
        Notify(msg_warning, "Dynamixel serial device name is not set.");
        size = 0;
        return;
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
                    baud_rate = 3500000; // guessing
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
                    
                    if(com->Ping(j))
                    {
                        printf(" %i ",j);
                        //servo[size] = new DynamixelServo(com, j);
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
        
        if(com->Ping(i))
        {
            printf(" %i ",i);
            servo[size] = new DynamixelServo(com, i);
            size += 1;
        }
    }
    printf("\n\n");
    
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
DynamixelConfigure::PrintChange(int i)
{
    if(servo[i])
    {
        // Print changes for a single servo.
        printf("\nDYNAMIXEL:");
        printf("\n\nCONTROL TABLE Change (ID:%i): ", servo[i]->GetID());
        
        if (set_id_change)
            printf("%-28s %8.0f ----> %4i","\nID:",servo[i]->GetIDFormated(), set_id);
        
        if (set_baud_rate_change)
            printf("%-28s %8.0f ----> %4i", "\nBaud Rate: ", servo[i]->GetBaudRateFormated(), set_baud_rate);
        
        if (set_return_delay_time_change)
            printf("%-28s %8.0f ----> %4i","\nReturn Delay Time (us): ",servo[i]->GetReturnDelayTimeFormated(), set_return_delay_time);
        
        if (set_cw_angle_limit_change)
            printf("%-28s %8.0f ----> %4.0f","\nCW Angle Limit: ", servo[i]->GetCWAngleLimitFormated(angle_unit), set_cw_angle_limit);
        
        if (set_ccw_angle_limit_change)
            printf("%-28s %8.0f ----> %4.0f","\nCCW Angle Limit: ",servo[i]->GetCCWAngleLimitFormated(angle_unit), set_ccw_angle_limit);
        
        if (set_drive_mode_change)
            printf("%-28s %8.0f ----> %4d","\nDriver Mode: ", servo[i]->GetDriveModeFormated(), set_drive_mode);
        
        if (set_limit_temperature_change)
            printf("%-28s %8.0f ----> %4.0f","\nLimit Temperature (C): ", servo[i]->GetHighestLimitTemperatureFormated(), set_limit_temperature);
        
        if (set_highest_limit_voltage_change)
            printf("%-28s %8.0f ----> %4.0f","\nHighest Limit Voltage (V): ",servo[i]->GetHighestLimitVoltageFormated(), set_highest_limit_voltage);
        
        if (set_lowest_limit_voltage_change)
            printf("%-28s %8.0f ----> %4.0f","\nLowest Limit Voltage (V): ", servo[i]->GetLowestLimitVoltageFormated(), set_lowest_limit_voltage);
        
        if (set_max_torque_change)
            printf("%-28s %8.2f ----> %.2f","\nMax Torque (%): ", servo[i]->GetMaxTorqueFormated(), set_max_torque);
        
        if (set_status_return_level_change)
            printf("%-28s %8.0f ----> %4i","\nStatus Return Level: ",servo[i]->GetStatusReturnLevelFormated(), set_status_return_level);
        
        if (set_alarm_led_change)
            printf("%-28s %8.0f ----> %4i","\nAlarm LED: ", servo[i]->GetAlarmLEDFormated(), set_alarm_led);
        
        if (set_alarm_shutdown_change)
            printf("%-28s %8.0f ----> %4i","\nAlarm Shutdown: ",servo[i]->GetAlarmShutdownFormated(), set_alarm_shutdown);
        
        if (set_torque_enable_change)
            printf("%-28s %8.0f ----> %4i","\nTorque Enable: ",servo[i]->GetTorqueEnableFormated(), set_torque_enable);
        
        if (set_led_change)
            printf("%-28s %8.0f ----> %4i","\nLED: ", servo[i]->GetLEDFormated(), set_led);
        
        if (set_d_gain_change)
        {
            printf("%-28s","\nD Gain:  ");
            if (servo[i]->GetDGainFormated() == -1)
                printf("%8s"		, "NA");
            else
                printf("%8.0f ----> %4i"		, servo[i]->GetDGainFormated(), set_d_gain);
        }
        
        if (set_i_gain_change)
        {
            printf("%-28s","\nI Gain:  ");
            if (servo[i]->GetIGainFormated() == -1)
                printf("%8s"		, "NA");
            else
                printf("%8.0f ----> %4i"		, servo[i]->GetIGainFormated(), set_i_gain);
        }
        
        if (set_p_gain_change)
        {
            printf("%-28s","\nP Gain: ");
            if (servo[i]->GetPGainFormated() == -1)
                printf("%8s"			, "NA");
            else
                printf("%8.0f ----> %4i"		, servo[i]->GetPGainFormated(), set_p_gain);
        }
        
        if (set_goal_position_change)
            printf("%-28s %8.2f ----> %4.2f","\nGoal Position:",servo[i]->GetGoalPositionFormated(angle_unit), set_goal_position);
        
        if (set_moving_speed_change)
            printf("%-28s %8.2f ----> %4.2f","\nMoving Speed:", servo[i]->GetMovingSpeedFormated(), set_moving_speed);
        
        if (set_torque_limit_change)
            printf("%-28s %8.2f ----> %4.2f","\nTorque Limit: ", servo[i]->GetTorqueLimitFormated(), set_torque_limit);
        
        if (set_lock_change)
            printf("%-28s %8.0f ----> %4i","\nLock: ",servo[i]->GetLockFormated(), set_lock);
        
        if (set_punch_change)
            printf("%-28s %8.4f ----> %4.2f","\nPunch: ",servo[i]->GetPunchFormated(), set_punch);
    }
    
}

void
DynamixelConfigure::SetSizes()
{
    SetOutputSize("RESET_MODE", size);
    SetOutputSize("CHANGE_MODE", size);
}

void
DynamixelConfigure::Init()
{
    // Find out which paramters that been set in ikc file.
    
    if (GetValue("set_id"))
        set_id_change = true;
    if (GetValue("set_baud_rate"))
        set_baud_rate_change = true;
    if (GetValue("set_return_delay_time"))
        set_return_delay_time_change = true;
    if (GetValue("set_cw_angle_limit"))
        set_cw_angle_limit_change = true;
    if (GetValue("set_ccw_angle_limit"))
        set_ccw_angle_limit_change = true;
    if (GetValue("set_drive_mode"))
        set_drive_mode_change = true;
    if (GetValue("set_limit_temperature"))
        set_limit_temperature_change = true;
    if (GetValue("set_lowest_limit_voltage"))
        set_lowest_limit_voltage_change = true;
    if (GetValue("set_highest_limit_voltage"))
        set_highest_limit_voltage_change = true;
    if (GetValue("set_max_torque"))
        set_max_torque_change = true;
    if (GetValue("set_status_return_level"))
        set_status_return_level_change = true;
    if (GetValue("set_alarm_led"))
        set_alarm_led_change = true;
    if (GetValue("set_alarm_shutdown"))
        set_alarm_shutdown_change = true;
    if (GetValue("set_torque_enable"))
        set_torque_enable_change = true;
    if (GetValue("set_led"))
        set_led_change = true;
    if (GetValue("set_d_gain"))
        set_d_gain_change = true;
    if (GetValue("set_i_gain"))
        set_i_gain_change = true;
    if (GetValue("set_p_gain"))
        set_p_gain_change = true;
    if (GetValue("set_goal_position"))
        set_goal_position_change = true;
    if (GetValue("set_moving_speed"))
        set_moving_speed_change = true;
    if (GetValue("set_torque_limit"))
        set_torque_limit_change = true;
    if (GetValue("set_lock"))
        set_lock_change = true;
    if (GetValue("set_punch"))
        set_punch_change = true;
    
    
    set_id = GetIntValue("set_id");
    set_baud_rate = GetIntValue("set_baud_rate");
    set_return_delay_time = GetIntValue("set_return_delay_time");
    set_cw_angle_limit = GetFloatValue("set_cw_angle_limit");
    set_ccw_angle_limit = GetFloatValue("set_ccw_angle_limit");
    set_drive_mode = GetIntValue("set_drive_mode");
    set_limit_temperature = GetFloatValue("set_limit_temperature");
    set_lowest_limit_voltage = GetFloatValue("set_lowest_limit_voltage");
    set_highest_limit_voltage = GetFloatValue("set_highest_limit_voltage");
    set_max_torque = GetFloatValue("set_max_torque");
    set_status_return_level = GetIntValue("set_status_return_level");
    set_alarm_led = GetIntValue("set_alarm_led");
    set_alarm_shutdown = GetIntValue("set_alarm_shutdown");
    set_torque_enable = GetBoolValue("set_torque_enable");
    set_led = GetBoolValue("set_led");
    set_d_gain = GetIntValue("set_d_gain");
    set_i_gain = GetIntValue("set_i_gain");
    set_p_gain = GetIntValue("set_p_gain");
    set_goal_position = GetFloatValue("set_goal_position");
    set_moving_speed = GetFloatValue("set_moving_speed");
    set_torque_limit = GetFloatValue("set_torque_limit");
    set_lock = GetBoolValue("set_lock");
    set_punch = GetFloatValue("set_punch");
    
    
    set	= GetInputArray("SET");
    if (!set)
    {
        Notify(msg_fatal_error, "DynamixelConfigure: Input SET must be connected.");
    }
    active	= GetInputArray("ACTIVE");
    if (!active)
    {
        Notify(msg_fatal_error, "DynamixelConfigure: Input ACTIVE must be connected.");
    }
    
    resetModeOut = GetOutputArray("RESET_MODE");
    changeModeOut = GetOutputArray("CHANGE_MODE");
    
}

DynamixelConfigure::~DynamixelConfigure()
{
    
    // Delete dynamixel memory buffert
    delete DynamixelMemoeries;
    
    // Free memory
    delete servo_index;
    delete servo_id;
    delete com;
    delete servo; // Also delete servo's memorybuffer
}

void
DynamixelConfigure::Tick()
{
     if (int(active[0]) < 0)
        active[0] = 0;
    
    if (int(active[0]) >= size)
        active[0] = size-1;
    
    if (resetMode)
    {
        reset_array(resetModeOut, size);
        resetModeOut[int(active[0])] = 1;
    }
    else
    {
        reset_array(changeModeOut, size);
        changeModeOut[int(active[0])] = 1;
    }
    
    // Checking that all inputs are the same.
    if (resetMode && set[0] == 1)
    {
        
        printf("\n\nDynamixel with ID %i will be restored to factory settings\n\n", servo_id[int(active[0])]);
        com->ResetDynamixel(servo_id[int(active[0])]);

        
        // quit ikaros
        Notify(msg_terminate, "DynamixelConfigure: Settings written to dynamixel(s). Quiting Ikaros...");
        return;
    }
    
    PrintChange(int(active[0]));
    
    if(set[0] == 1)
    {

        // Write to a single dynamixel ID
        int i = int(active[0]);
        
        // Adding changes
        if(servo[i])
        {
            if (set_id_change)
                servo[i]->SetID(set_id);
            else
                servo[i]->SetID(servo[i]->GetIDFormated());
            
            if (set_baud_rate_change)
                servo[i]->SetBaudRateFormated(set_baud_rate);
            else
                servo[i]->SetBaudRateFormated(servo[i]->GetBaudRateFormated());
            
            if (set_return_delay_time_change)
                servo[i]->SetReturnDelayTimeFormated(set_return_delay_time);
            else
                servo[i]->SetReturnDelayTimeFormated(servo[i]->GetReturnDelayTimeFormated());
            
            if (set_cw_angle_limit_change)
                servo[i]->SetCWAngleLimitFormated(set_cw_angle_limit, angle_unit);
            else
                servo[i]->SetCWAngleLimitFormated(servo[i]->GetCWAngleLimitFormated(angle_unit),angle_unit);
            
            if (set_ccw_angle_limit_change)
                servo[i]->SetCCWAngleLimitFormated(set_ccw_angle_limit, angle_unit);
            else
                servo[i]->SetCCWAngleLimitFormated(servo[i]->GetCCWAngleLimitFormated(angle_unit),angle_unit);
            
            if (set_drive_mode_change)
                servo[i]->SetDriveModeFormated(set_drive_mode);
            else
                servo[i]->SetDriveModeFormated(servo[i]->GetDriveModeFormated());
            
            if (set_limit_temperature_change)
                servo[i]->SetHighestLimitTemperatureFormated(set_limit_temperature);
            else
                servo[i]->SetHighestLimitTemperatureFormated(servo[i]->GetHighestLimitTemperatureFormated());
            
            if (set_lowest_limit_voltage_change)
                servo[i]->SetLowestLimitVoltageFormated(set_lowest_limit_voltage);
            else
                servo[i]->SetLowestLimitVoltageFormated(servo[i]->GetLowestLimitVoltageFormated());
            
            if (set_highest_limit_voltage_change)
                servo[i]->SetHighestLimitVoltageFormated(set_highest_limit_voltage);
            else
                servo[i]->SetHighestLimitVoltageFormated(servo[i]->GetHighestLimitVoltageFormated());
            
            if (set_max_torque_change)
                servo[i]->SetMaxTorqueFormated(set_max_torque, angle_unit);
            else
                servo[i]->SetMaxTorqueFormated(servo[i]->GetMaxTorqueFormated(),angle_unit);
            
            if (set_status_return_level_change)
                servo[i]->SetStatusReturnLevelFormated(set_status_return_level);
            else
                servo[i]->SetStatusReturnLevelFormated(servo[i]->GetStatusReturnLevelFormated());
            
            if (set_alarm_led_change)
                servo[i]->SetAlarmLEDFormated(set_alarm_led);
            else
                servo[i]->SetAlarmLEDFormated(servo[i]->GetAlarmLEDFormated());
            
            if (set_alarm_shutdown_change)
                servo[i]->SetAlarmShutdownFormated(set_alarm_shutdown);
            else
                servo[i]->SetAlarmShutdownFormated(servo[i]->GetAlarmShutdownFormated());
            
            if (set_torque_enable_change)
                servo[i]->SetTorqueEnableFormated(set_torque_enable);
            else
                servo[i]->SetTorqueEnableFormated(servo[i]->GetTorqueEnableFormated());
            
            if (set_led_change)
                servo[i]->SetLEDFormated(set_led);
            else
                servo[i]->SetLEDFormated(servo[i]->GetLEDFormated());
            
            if (set_d_gain_change)
                servo[i]->SetDGainFormated(set_d_gain);
            else
                servo[i]->SetDGainFormated(servo[i]->GetDGainFormated());
            
            if (set_i_gain_change)
                servo[i]->SetIGainFormated(set_i_gain);
            else
                servo[i]->SetIGainFormated(servo[i]->GetIGainFormated());
            
            if (set_p_gain_change)
                servo[i]->SetPGainFormated(set_p_gain);
            else
                servo[i]->SetPGainFormated(servo[i]->GetPGainFormated());
            
            if (set_goal_position_change)
                servo[i]->SetGoalPositionFormated(set_goal_position, angle_unit);
            else
                servo[i]->SetGoalPositionFormated(servo[i]->GetGoalPositionFormated(angle_unit),angle_unit);
            
            if (set_moving_speed_change)
                servo[i]->SetMovingSpeedFormated(set_moving_speed);
            else
                servo[i]->SetMovingSpeedFormated(servo[i]->GetMovingSpeedFormated());
            
            if (set_torque_limit_change)
                servo[i]->SetTorqueLimitFormated(set_torque_limit);
            else
                servo[i]->SetTorqueLimitFormated(servo[i]->GetTorqueLimitFormated());
            
            if (set_lock_change)
                servo[i]->SetLockFormated(set_lock);
            else
                servo[i]->SetLockFormated(servo[i]->GetLockFormated());
            
            if (set_punch_change)
                servo[i]->SetPunchFormated(set_punch);
            else
                servo[i]->SetPunchFormated(servo[i]->GetPunchFormated());
            
        }
    }
    
    Timer timer;
    
    // Wait for accept on the webUI.
    // Write changes to servos
    if (set[0] == 1)
    {
        // SYNCWRITE
        com->SyncWriteWithIdRange(servo_id, DynamixelMemoeries, 5, 9, size);
        com->SyncWriteWithIdRange(servo_id, DynamixelMemoeries, 11, 18, size);
        com->SyncWriteWithIdRange(servo_id, DynamixelMemoeries, 24, 35, size);
        com->SyncWriteWithIdRange(servo_id, DynamixelMemoeries, 47, 47, size); // This can not be sent using SYNC Write will not let anything else change if this parameter is in the package.
        com->SyncWriteWithIdRange(servo_id, DynamixelMemoeries, 48, 49, size);
        
        // Sending new ID or Baudrate last
        com->SyncWriteWithIdRange(servo_id, DynamixelMemoeries, 3, 4, size);
        
        timer.Sleep(500); // to make sure we have time to write changes before shuting down ikaros.
        
        // Shutdown ikaros.
        Notify(msg_terminate, "DynamixelConfigure: Settings written to dynamixel(s). Quiting Ikaros...");
    }
    
    // Blink active servo
    for(int i=0; i<size; i++)
        if(servo[i])
            servo[i]->SetLED(0);
    if(servo[int(active[0])])
        servo[int(active[0])]->SetLED(blink);
    com->SyncWriteWithIdRange(servo_id, DynamixelMemoeries, 25, 25, size);
    if (blink == 1)
        blink = 0;
    else
        blink = 1;
    timer.Sleep(100);  // Sleep to get the blinking effect
    // Blink active servo end
    
}

int
DynamixelConfigure::checkBaudRate(int br)
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

static InitClass init("DynamixelConfigure", &DynamixelConfigure::Create, "Source/Modules/RobotModules/Dynamixel/");

