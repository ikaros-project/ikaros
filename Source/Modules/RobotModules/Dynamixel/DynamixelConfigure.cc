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
    
    // Number of servos
    servos = size;
    
    // Allocate servo maps and parameter structures
    servo_index  = new int [servos];
    servo_id     = new int [servos];
    
    DynamixelMemoeries = new unsigned char * [servos];
    
    // Connecting memory to servo memory
    for(int i=0; i<size; i++)
        if(servo[i])
            DynamixelMemoeries[i] = servo[i]->dynamixelMemory;
    
    int j=0;
    for(int i=0; i<size; i++)
        if(servo[i])
        {
            servo_id[j] = servo[i]->extraInfo.GetInt("ID");
            servo_index[j++] = i;
        }
    
    // If we did not find any servos kill ikaros.
    if (size == 0)
        Notify(msg_fatal_error, "Did not find any Dynamixel servos.\n\n");
    
    changeAdress = GetIntValue("adress");
    newValue = GetIntValue("value");
    
    mask        = new int [servos];
	for (int i = 0; i<servos; i++)
		mask[i] = 1;
}

void
DynamixelConfigure::PrintChange(int i)
{
        // Print changes for a single servo.
        printf("\n\n(ID:%i) %s %i -> %i: ", servo[i]->extraInfo.GetInt("ID"), servo[i]->controlTable[changeAdress].Name.c_str(), servo[i]->GetValueAtAdress(changeAdress), newValue);
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
    
    
    // OBS ORDER OF MATRIXES IS [Paramter][Servo]
    // Create memory for binding map. These are specified in the csv file and correspond to where ikaros should look for IO in the dynamixel memory block
    ikarosInBind = new int * [10];
    for(int i=0; i<10; i++)
        ikarosInBind[i] = new int [servos];
    
    ikarosOutBind = new int * [16];
    for(int i=0; i<16; i++)
        ikarosOutBind[i] = new int [servos];
    
    // Reset array
    for (int i = 0; i < 10; i++)
        for (int j = 0; j < servos; j++)
            ikarosInBind[i][j] = -1;
    for (int i = 0; i < 16; i++)
        for (int j = 0; j < servos; j++)
            ikarosOutBind[i][j] = -1;
    
    // Fill in where to look for data
    for (int i = 0; i < servos; i++)
        for (int j = 0; j < C_TABLE_SIZE; j++)
        {
            if (servo[servo_index[i]]->controlTable[j].IkarosInputs >= 0)
                ikarosInBind[servo[servo_index[i]]->controlTable[j].IkarosInputs][i] = servo[servo_index[i]]->controlTable[j].Adress;
            if (servo[servo_index[i]]->controlTable[j].IkarosOutputs >= 0)
                ikarosOutBind[servo[servo_index[i]]->controlTable[j].IkarosOutputs][i] = servo[servo_index[i]]->controlTable[j].Adress;
        }
    
    // OBS ORDER OF MATRIXES IS [inputs][Servo]
    // Create memory size map
    parameterInSize = new int * [10];
    for(int i=0; i<10; i++)
        parameterInSize[i] = new int [servos];
    
    for(int i=0; i<10; i++)
        for(int j=0; j<servos; j++)
            if(ikarosInBind[i][j]!= -1)
                parameterInSize[i][j] =  servo[servo_index[j]]->controlTable[ikarosInBind[i][j]].Size;
            else
                parameterInSize[i][j] =  -1;
    
    // Do not allow mixed protocols
    protocol = -1;
    if (servos > 0)
        for (int i = 0; i < servos; i++)
        {
            if (protocol == -1 && servo[servo_index[i]]->protocol != 0)
                protocol = servo[servo_index[i]]->protocol; // Set protocol.
            
            if (protocol != servo[servo_index[i]]->protocol)
                Notify(msg_fatal_error, "Dynamixel uses different protocols.\n\n");
        }
    PrintAll();
}

DynamixelConfigure::~DynamixelConfigure()
{
    // Turn off LED
    Timer timer;
    for(int i=0; i<size; i++)
        if(servo[i])
            servo[i]->SetValueAtAdress(ikarosInBind[IK_IN_LED][i], 0);
    com->WriteToServo(servo_id, mask, protocol, DynamixelMemoeries, ikarosInBind[IK_IN_LED], parameterInSize[IK_IN_LED], size);
    timer.Sleep(200);
    
    
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
    delete DynamixelMemoeries;
    delete servo_index;
    delete servo_id;
    delete com;
    delete servo; // Also deletes servo's memorybuffer
}

void
DynamixelConfigure::Tick()
{
    Timer timer;
    int selectedServo = int(active[0]);
    
    if (selectedServo < 0)
        selectedServo = 0;
    
    if (selectedServo >= size)
        selectedServo = size-1;
    
    if (resetMode)
    {
        reset_array(resetModeOut, size);
        resetModeOut[selectedServo] = 1;
        
        if (set[0] == 1)
        {
            printf("\n\nDynamixel with ID %i will be restored to factory settings\n\n", servo_id[int(active[0])]);
            com->Reset(servo_id[int(active[0])],servo[int(active[0])]->protocol);
            
            // quit ikaros
            Notify(msg_terminate, "DynamixelConfigure: Settings written to dynamixel(s). Quiting Ikaros...");
        }
    }
    else
    {
        reset_array(changeModeOut, size);
        changeModeOut[selectedServo] = 1;
        
        PrintChange(selectedServo);
        
        if(set[0] == 1) // Write to a single dynamixel ID
        {
            // Creating a size array
            int * parameterSize = new int [size];
            
            for(int i=0; i<size; i++)
                parameterSize[i] =  servo[servo_index[i]]->controlTable[changeAdress].Size;
            
            // Adding changes
            servo[selectedServo]->SetValueAtAdress(changeAdress, newValue);
            
            // create a temp adress array
            int * adress = new int [size];

            for (int j = 0; j < size; j++) {
                adress[j] = changeAdress;
            }
            
            com->WriteToServo(servo_id, mask, protocol, DynamixelMemoeries, adress, parameterSize, size);
            
            timer.Sleep(500); // to make sure we have time to write changes before shuting down ikaros.
            // Shutdown ikaros.
            Notify(msg_terminate, "DynamixelConfigure: Settings written to dynamixel(s). Quiting Ikaros...");
            
            delete[](adress);
            delete[](parameterSize);
        }
    }
    
    // Blink
    for(int i=0; i<servos; i++)
        servo[i]->SetValueAtAdress(ikarosInBind[IK_IN_LED][i], 0); // Lights off on all servos
    servo[selectedServo]->SetValueAtAdress(ikarosInBind[IK_IN_LED][selectedServo], blink); // Lights on on selected servo
    
    com->WriteToServo(servo_id, mask, protocol, DynamixelMemoeries, ikarosInBind[IK_IN_LED], parameterInSize[IK_IN_LED], size); // write to all servos
    
    if (blink == 1)
        blink = 0;
    else
        blink = 1;
    timer.Sleep(100);
    // Blink end

}

void
DynamixelConfigure::PrintAll()
{
    printf("\nDYNAMIXEL\n");
    printf("Number of servos: %d\n\n", servos);
    printf("Control table:\n");
    printf("%-8s %-32s|", "Adress", "Name");
    for(int i=0; i<servos; i++)
        printf(" %6s|",servo[servo_index[i]]->extraInfo.Get("Servo Model String"));
    printf("\n");
    for (int j = 0; j < 100; j++)
    {
        if (servo[servo_index[0]]->controlTable[j].Visable) // Use the description from the first servo
        {
            printf("%-8i %-32s|", j, servo[servo_index[0]]->controlTable[j].Name.c_str());
            for(int i=0; i<servos; i++)
                printf(" %6i|",servo[servo_index[i]]->GetValueAtAdress(j));
            printf("\n");
        }
    }
}

static InitClass init("DynamixelConfigure", &DynamixelConfigure::Create, "Source/Modules/RobotModules/Dynamixel/");
