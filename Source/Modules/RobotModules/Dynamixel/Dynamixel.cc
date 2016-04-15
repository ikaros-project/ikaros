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
        Notify(msg_warning, "Dynamixel serial device \"%s\" could not be opened (Check baud rate.", device);
        return;
    }
    
    std::string csvPath;
    
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
            if(com->Ping(servo_id_list[i]) > 0)
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
            if(com->Ping(servo_id_list[i]) > 0)
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
            if(com->Ping(i) > 0)
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
            if(com->Ping(i) > 0)
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
            servo_id[i] = servo[i]->controlTable.GetInt("ID");
        }
    
    
    // If we did not find any servos kill ikaros.
    if (size == 0)
        Notify(msg_fatal_error, "Did not find any Dynamixel servos.\n\n");
    
    
    // Find out where to look to ikaros output data from Dynamixel memory
    for (int i = 0; i < 16; i++)
        ikarosOutBind[i] = -1;
    for (int i = 0; i < size; i++)
        for (int j = 0; j < 128; j++)
            if (servo[i]->ctable[j].IkarosOutputs >= 0)
                ikarosOutBind[servo[i]->ctable[j].IkarosOutputs] = servo[i]->ctable[j].Adress;
    for (int i = 0; i < size; i++)
        for (int j = 0; j < 128; j++)
            if (servo[i]->ctable[j].IkarosInputs >= 0)
                ikarosInBind[servo[i]->ctable[j].IkarosInputs] = servo[i]->ctable[j].Adress;
    //    printf("Bindings OUTPUT\n");
    //    for (int i = 0; i <= 15; i++)
    //        printf("%i : %i\n",i ,ikarosOutBind[i]) ;
    //
    //    printf("Bindings INPUT\n");
    //    for (int i = 0; i <= 9; i++)
    //        printf("%i : %i\n",i ,ikarosInBind[i]) ;
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
    SetOutputSize("FEEDBACK_GOAL_TORQUE", size);
    SetOutputSize("FEEDBACK_GOAL_ACCELERATION", size);
}

void
Dynamixel::Init()
{
    // Inputs
    torqueEnable_connected      = false;
    LED_connected               = false;
    dGain_connected             = false;
    iGain_connected             = false;
    pGain_connected             = false;
    goalPosition_connected      = false;
    movingSpeed_connected       = false;
    torqueLimit_connected       = false;
    goalTorque_connected        = false;
    goalAcceleration_connected  = false;
    
    torqueEnable        = GetInputArray("TORQUE_ENABLE", false);
    LED                 = GetInputArray("LED", false);
    dGain               = GetInputArray("D_GAIN", false);
    iGain               = GetInputArray("I_GAIN", false);
    pGain               = GetInputArray("P_GAIN", false);
    goalPosition        = GetInputArray("GOAL_POSITION", false);
    movingSpeed         = GetInputArray("MOVING_SPEED", false);
    torqueLimit         = GetInputArray("TORQUE_LIMIT", false);
    goalTorque          = GetInputArray("GOAL_TORQUE", false);
    goalAcceleration	= GetInputArray("GOAL_ACCELERATION", false);
    
    // Mark all connected inputs
    if (torqueEnable)
        torqueEnable_connected = true;
    if (LED)
        LED_connected = true;
    if (dGain)
        dGain_connected = true;
    if (iGain)
        iGain_connected = true;
    if (pGain)
        pGain_connected = true;
    if (goalPosition)
        goalPosition_connected = true;
    if (movingSpeed)
        movingSpeed_connected = true;
    if (torqueLimit)
        torqueLimit_connected = true;
    if (goalTorque)
        goalTorque_connected = true;
    if (goalAcceleration)
        goalAcceleration_connected = true;
    
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
    for(int i=0; i<size; i++)
        if(servo[i])
            servo[i]->SetValueAtAdress(ikarosOutBind[IK_IN_GOAL_POSITION], servo[i]->GetValueAtAdress(IK_OUT_PRESENT_SPEED));
    
    com->SyncWriteWithIdRange(servo_id, servo[0]->Protocol, DynamixelMemoeries, ikarosOutBind[IK_OUT_GOAL_POSITION], ikarosOutBind[IK_OUT_GOAL_POSITION]+1, size);
}

Dynamixel::~Dynamixel()
{
    
    // Torqueing down the servos. Using the torque from last time used.
    Timer timer;
    if (init_print == 2 || init_print == 1)
        printf("\nPower off servo(s)");
    
    int blink = 0;
    // Turn off torque for all servos. Stepwise to zero
    for(float j=1.0f; j>=0; j = j - 0.1)
    {
        for(int i=0; i<size; i++)
            if(servo[i])
            {
                servo[i]->SetValueAtAdress(ikarosOutBind[IK_IN_TORQUE_LIMIT], servo[i]->GetValueAtAdress(ikarosOutBind[IK_OUT_TORQUE_LIMIT]*j));
                servo[i]->SetValueAtAdress(ikarosOutBind[IK_IN_LED], blink);
            }
        
        com->SyncWriteWithIdRange(servo_id, servo[0]->Protocol ,DynamixelMemoeries,ikarosInBind[IK_IN_TORQUE_LIMIT], ikarosInBind[IK_IN_TORQUE_LIMIT]+1, size);
        com->SyncWriteWithIdRange(servo_id, servo[0]->Protocol, DynamixelMemoeries,ikarosOutBind[IK_IN_LED], ikarosOutBind[IK_IN_LED], size);
        
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
    
    // Turn off LED
    for(int i=0; i<size; i++)
        if(servo[i])
            servo[i]->SetValueAtAdress(ikarosOutBind[IK_IN_LED], 0);
    
    com->SyncWriteWithIdRange(servo_id, servo[0]->Protocol, DynamixelMemoeries,ikarosOutBind[IK_IN_LED], ikarosOutBind[IK_IN_LED], size);
    
    
    // Set goal position to current position.
    for(int i=0; i<size; i++)
        if(servo[i])
            servo[i]->SetValueAtAdress(ikarosOutBind[IK_IN_GOAL_POSITION], servo[i]->GetValueAtAdress(ikarosOutBind[IK_OUT_GOAL_POSITION]));
    
    com->SyncWriteWithIdRange(servo_id, servo[0]->Protocol, DynamixelMemoeries,ikarosOutBind[IK_OUT_GOAL_POSITION], ikarosOutBind[IK_OUT_GOAL_POSITION], size);
    
    
    timer.Sleep(100); // Sleep to make sure everyting is sent to servo before deleting memory
    
    // Free memory
    delete DynamixelMemoeries;
    delete servo_index;
    delete servo_id;
    delete com;
    delete servo; // Also delete servo's memorybuffer
}

void
Dynamixel::Tick()
{
    // check that the device exists and is open
    if(!device || !com)
        return;
    
    if(GetTick() >= start_up_delay) // Do not send any instructions during start_up_delay
    {
        for(int i=0; i<size; i++)
        {
            if(servo[i])
            {
                
                if (torqueEnable_connected)
                    servo[i]->SetTorqueEnableFormated(ikarosInBind[IK_IN_TORQUE_ENABLE],torqueEnable[i]);
                if (LED_connected)
                    servo[i]->SetLEDFormated(ikarosInBind[IK_IN_LED],LED[i]);
                if (dGain_connected)
                    servo[i]->SetDGainFormated(ikarosInBind[IK_IN_D_GAIN],dGain[i]);
                if (iGain_connected)
                    servo[i]->SetIGainFormated(ikarosInBind[IK_IN_I_GAIN],iGain[i]);
                if (pGain_connected)
                    servo[i]->SetPGainFormated(ikarosInBind[IK_IN_P_GAIN],pGain[i]);
                if (goalPosition_connected)
                    servo[i]->SetGoalPositionFormated(ikarosInBind[IK_IN_GOAL_POSITION],goalPosition[i], angle_unit);
                if (movingSpeed_connected)
                    servo[i]->SetMovingSpeedFormated(ikarosInBind[IK_IN_MOVING_SPEED],movingSpeed[i]);
                if (torqueLimit_connected)
                    servo[i]->SetTorqueLimitFormated(ikarosInBind[IK_IN_TORQUE_LIMIT],torqueLimit[i]);
                if (goalTorque_connected)
                    servo[i]->SetGoalTorqueFormated(ikarosInBind[IK_IN_GOAL_TORQUE],goalTorque[i]);
                if (goalAcceleration_connected)
                    servo[i]->SetGoalAccelerationFormated(ikarosInBind[IK_IN_GOAL_ACCELERATION],goalAcceleration[i]);
            }
        }
        
        // Syncwrite to all servos.
        // Splitting all the package. Test performence of this.
        if (LED_connected)
            com->SyncWriteWithIdRange(servo_id, servo[0]->Protocol, DynamixelMemoeries, ikarosInBind[IK_IN_LED], ikarosInBind[IK_IN_LED], size);
        if (dGain_connected)
            com->SyncWriteWithIdRange(servo_id, servo[0]->Protocol, DynamixelMemoeries,ikarosInBind[IK_IN_D_GAIN], ikarosInBind[IK_IN_D_GAIN], size);
        if (iGain_connected)
            com->SyncWriteWithIdRange(servo_id, servo[0]->Protocol, DynamixelMemoeries, ikarosInBind[IK_IN_I_GAIN], ikarosInBind[IK_IN_I_GAIN], size);
        if (pGain_connected)
            com->SyncWriteWithIdRange(servo_id, servo[0]->Protocol, DynamixelMemoeries, ikarosInBind[IK_IN_P_GAIN], ikarosInBind[IK_IN_P_GAIN]+1, size);
        if (goalPosition_connected && torqueEnable[0] > 0 ) // This Needs to be taken care of.
            com->SyncWriteWithIdRange(servo_id, servo[0]->Protocol, DynamixelMemoeries,ikarosInBind[IK_IN_GOAL_POSITION], ikarosInBind[IK_IN_GOAL_POSITION]+1, size);
        if (movingSpeed_connected)
            com->SyncWriteWithIdRange(servo_id, servo[0]->Protocol, DynamixelMemoeries,ikarosInBind[IK_IN_MOVING_SPEED], ikarosInBind[IK_IN_MOVING_SPEED]+1, size);
        if (torqueLimit_connected)
            com->SyncWriteWithIdRange(servo_id, servo[0]->Protocol, DynamixelMemoeries,ikarosInBind[IK_IN_TORQUE_LIMIT], ikarosInBind[IK_IN_TORQUE_LIMIT]+1, size);
        if (goalTorque_connected)
            com->SyncWriteWithIdRange(servo_id, servo[0]->Protocol, DynamixelMemoeries, ikarosInBind[IK_IN_GOAL_TORQUE], ikarosInBind[IK_IN_GOAL_TORQUE]+1, size);
        if (goalAcceleration_connected)
            com->SyncWriteWithIdRange(servo_id, servo[0]->Protocol, DynamixelMemoeries, ikarosInBind[IK_IN_GOAL_ACCELERATION], ikarosInBind[IK_IN_GOAL_ACCELERATION], size);
        
        // Torque can not be sent in a packet with goal position. See manual.
        if (torqueEnable_connected)
            com->SyncWriteWithIdRange(servo_id, servo[0]->Protocol, DynamixelMemoeries, ikarosInBind[IK_IN_TORQUE_ENABLE], ikarosInBind[IK_IN_TORQUE_ENABLE], size);
    }
    
    
    // Get feedback
    for(int i=0; i<size; i++)
        if(servo[i])
        {
            unsigned char inbuf[256];
            
            if(use_feedback)
                com->ReadMemoryRange(servo[i]->controlTable.GetInt("ID"), servo[i]->Protocol, inbuf, 0, servo[i]->controlTable.GetInt("Model Memory"));
            
            if (ikarosOutBind[IK_OUT_TORQUE_ENABLE] != -1)
                feedbackTorqueEnable[i]  = servo[i]->GetTorqueEnableFormated(ikarosOutBind[IK_OUT_TORQUE_ENABLE]);
            if (ikarosOutBind[IK_OUT_LED] != -1)
                feedbackLED[i] = servo[i]->GetLEDFormated(ikarosOutBind[IK_OUT_LED]);
            if (ikarosOutBind[IK_OUT_D_GAIN] != -1)
                feedbackDGain[i] = servo[i]->GetDGainFormated(ikarosOutBind[IK_OUT_D_GAIN]);
            if (ikarosOutBind[IK_OUT_I_GAIN] != -1)
                feedbackIGain[i] = servo[i]->GetIGainFormated(ikarosOutBind[IK_OUT_I_GAIN]);
            if (ikarosOutBind[IK_OUT_P_GAIN] != -1)
                feedbackPGain[i] = servo[i]->GetPGainFormated(ikarosOutBind[IK_OUT_P_GAIN]);
            if (ikarosOutBind[IK_OUT_GOAL_POSITION] != -1)
                feedbackGoalPosition[i] = servo[i]->GetGoalPositionFormated(ikarosOutBind[IK_OUT_GOAL_POSITION],angle_unit);
            if (ikarosOutBind[IK_OUT_GOAL_POSITION] != -1)
                feedbackMoving[i] = servo[i]->GetMovingSpeedFormated(ikarosOutBind[IK_OUT_MOVING_SPEED]);
            if (ikarosOutBind[IK_OUT_GOAL_POSITION] != -1)
                feedbackTorqueLimit[i] = servo[i]->GetTorqueLimitFormated(ikarosOutBind[IK_OUT_TORQUE_LIMIT]);
            if (ikarosOutBind[IK_OUT_GOAL_POSITION] != -1)
                feedbackPresentPosition[i] = servo[i]->GetPresentPositionFormated(ikarosOutBind[IK_OUT_PRESENT_POSITION],angle_unit);
            if (ikarosOutBind[IK_OUT_PRESENT_SPEED] != -1)
                feedbackPresentSpeed[i] = servo[i]->GetPresentSpeedFormated(ikarosOutBind[IK_OUT_PRESENT_SPEED]);
            if (ikarosOutBind[IK_OUT_PRESENT_LOAD] != -1)
                feedbackPresentLoad[i] = servo[i]->GetPresentLoadFormated(ikarosOutBind[IK_OUT_PRESENT_LOAD]);
            if (ikarosOutBind[IK_OUT_PRESENT_VOLTAGE] != -1)
                feedbackPresentVoltage[i] =  servo[i]->GetPresentVoltageFormated(ikarosOutBind[IK_OUT_PRESENT_VOLTAGE]);
            if (ikarosOutBind[IK_OUT_PRESENT_TEMPERATURE] != -1)
                feedbackPresentTemperature[i] = servo[i]->GetPresentTemperatureFormated(ikarosOutBind[IK_OUT_PRESENT_TEMPERATURE]);
            if (ikarosOutBind[IK_OUT_PRESENT_CURRENT] != -1)
                feedbackPresentCurrent[i] = servo[i]->GetCurrentFormated(ikarosOutBind[IK_OUT_PRESENT_CURRENT]);
            if (ikarosOutBind[IK_OUT_GOAL_TORQUE] != -1)
                feedbackGoalTorque[i] = servo[i]->GetGoalTorqueFormated(ikarosOutBind[IK_OUT_GOAL_TORQUE]);
            if (ikarosOutBind[IK_OUT_GOAL_ACCELERATION] != -1)
                feedbackGoalAcceleration[i] = servo[i]->GetGoalAccelerationFormated(ikarosOutBind[IK_OUT_GOAL_ACCELERATION]);
        }
}

void
Dynamixel::Print()
{
    printf("\nDYNAMIXEL\n");
    printf("Number of servos: %d\n\n", servos);
    printf("%6s %5s\n", "ID", "Name");
    for(int i=0; i<size; i++)
    {
        printf("%-4i",i);
        printf("%-4i",servo[i]->controlTable.GetInt("ID"));
        printf("%6s",servo[i]->controlTable.Get("Servo Model String"));
        printf("\n");
    }
}

void
Dynamixel::PrintAll()
{
    printf("\nDYNAMIXEL\n");
    printf("Number of servos: %d\n\n", servos);
    printf("Control table:\n");
    printf("%-8s %-28s|", "Adress", "Name");
    for(int i=0; i<size; i++)
        printf(" %6s|",servo[i]->controlTable.Get("Servo Model String"));
    printf("\n");
    for (int j = 0; j < 100; j++)
    {
        if (servo[0]->ctable[j].Visable)
        {
            printf("%-8i %-28s|", j, servo[0]->ctable[j].Name.c_str());
            for(int i=0; i<size; i++)
                printf(" %6i|",servo[i]->GetValueAtAdress(j));
            printf("\n");
        }
    }
}

static InitClass init("Dynamixel", &Dynamixel::Create, "Source/Modules/RobotModules/Dynamixel/");
