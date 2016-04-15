//
//	DynamixelConfigure.cc
//
//    Copyright (C) 2016  Birger Johansson
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
    device = GetValue("device");
    if(!device)
    {
        Notify(msg_warning, "Dynamixel serial device name is not set.");
        size = 0;
        return;
    }
    
    
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
                    if(com->Ping(j))
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
    
    //
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
            servo_id[i] = servo[i]->controlTable.GetInt("ID");
        }
    
    // If we did not find any servos kill ikaros.
    if (size == 0)
        Notify(msg_fatal_error, "Did not find any Dynamixel servos.\n\n");
    
    changeAdress = GetIntValue("adress");
    newValue = GetIntValue("value");
}

void
DynamixelConfigure::PrintChange(int i)
{
    if(servo[i])
    {
        // Print changes for a single servo.
        printf("\n\n(ID:%i) %s %i -> %i: ", servo[i]->controlTable.GetInt("ID"), servo[i]->ctable[changeAdress].Name.c_str(), servo[i]->GetValueAtAdress(changeAdress), newValue);
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
    blink = 0;
    
    // Find out where to look to ikaros output data from Dynamixel memory
    for (int i = 0; i < size; i++)
        for (int j = 0; j < 128; j++)
            if (servo[i]->ctable[j].IkarosInputs >= 0)
                ikarosInBind[servo[i]->ctable[j].IkarosInputs] = servo[i]->ctable[j].Adress;
}

DynamixelConfigure::~DynamixelConfigure()
{
    // Turn off LED
    Timer timer;
    for(int i=0; i<size; i++)
        if(servo[i])
            servo[i]->SetValueAtAdress(ikarosInBind[IK_IN_LED], 0);
    com->SyncWriteWithIdRange(servo_id, servo[0]->Protocol, DynamixelMemoeries, ikarosInBind[IK_IN_LED], ikarosInBind[IK_IN_LED], size);
    timer.Sleep(200);
    
    
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
    Timer timer;
    
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
        com->Reset(servo_id[int(active[0])],servo[int(active[0])]->Protocol);
        
        
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
            servo[i]->SetValueAtAdress(changeAdress, newValue);
            com->SyncWriteWithIdRange(servo_id, servo[0]->Protocol, DynamixelMemoeries, changeAdress, changeAdress+ servo[i]->ctable[changeAdress].Size-1, size);
            
            timer.Sleep(500); // to make sure we have time to write changes before shuting down ikaros.
            // Shutdown ikaros.
            Notify(msg_terminate, "DynamixelConfigure: Settings written to dynamixel(s). Quiting Ikaros...");
            
        }
    }
    
    // Blink active servo
    for(int i=0; i<size; i++)
        if(servo[i])
            servo[i]->SetValueAtAdress(ikarosInBind[IK_IN_LED], 0);
    if(servo[int(active[0])])
        servo[int(active[0])]->SetValueAtAdress(ikarosInBind[IK_IN_LED], blink);
    
    com->SyncWriteWithIdRange(servo_id, servo[0]->Protocol, DynamixelMemoeries, ikarosInBind[IK_IN_LED], ikarosInBind[IK_IN_LED], size);
    if (blink == 1)
        blink = 0;
    else
        blink = 1;
    timer.Sleep(100);  // Sleep to get the blinking effect
    // Blink active servo end
    
}

static InitClass init("DynamixelConfigure", &DynamixelConfigure::Create, "Source/Modules/RobotModules/Dynamixel/");

