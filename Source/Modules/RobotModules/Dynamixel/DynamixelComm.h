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

#ifndef DYNAMIXELCOMM
#define DYNAMIXELCOMM

#include "IKAROS.h"


//--- Control Table Address ---

#define P_MODEL_NUMBER_L      0
#define P_MODOEL_NUMBER_H     1
#define P_VERSION             2
#define P_ID                  3
#define P_BAUD_RATE           4
#define P_RETURN_DELAY_TIME   5
#define P_CW_ANGLE_LIMIT_L    6
#define P_CW_ANGLE_LIMIT_H    7
#define P_CCW_ANGLE_LIMIT_L   8
#define P_CCW_ANGLE_LIMIT_H   9
#define P_SYSTEM_DATA2        10
#define P_LIMIT_TEMPERATURE   11
#define P_DOWN_LIMIT_VOLTAGE  12
#define P_UP_LIMIT_VOLTAGE    13
#define P_MAX_TORQUE_L        14
#define P_MAX_TORQUE_H        15
#define P_RETURN_LEVEL        16
#define P_ALARM_LED           17
#define P_ALARM_SHUTDOWN      18
#define P_OPERATING_MODE      19
#define P_KP                  20    // These names seem to suggest some form of PID controller
#define P_KD                  21    // but the AX-12 manual calls address 20-23 up/down calibration
#define P_KI                  22    // to compensate for potentiometer inaccuracies
#define P_IDAMP               23

#define P_TORQUE_ENABLE         24
#define P_LED                   25
#define P_CW_COMPLIANCE_MARGIN  26
#define P_CCW_COMPLIANCE_MARGIN 27
#define P_CW_COMPLIANCE_SLOPE   28
#define P_CCW_COMPLIANCE_SLOPE  29
#define P_GOAL_POSITION_L       30
#define P_GOAL_POSITION_H       31
#define P_GOAL_SPEED_L          32
#define P_GOAL_SPEED_H          33
#define P_TORQUE_LIMIT_L        34
#define P_TORQUE_LIMIT_H        35
#define P_PRESENT_POSITION_L    36
#define P_PRESENT_POSITION_H    37
#define P_PRESENT_SPEED_L       38
#define P_PRESENT_SPEED_H       39
#define P_PRESENT_LOAD_L        40
#define P_PRESENT_LOAD_H        41
#define P_PRESENT_VOLTAGE       42
#define P_PRESENT_TEMPERATURE   43
#define P_REGISTERED_INSTRUCTION 44
#define P_PAUSE_TIME            45
#define P_MOVING                46
#define P_LOCK                  47
#define P_PUNCH_L               48
#define P_PUNCH_H               49

#define INST_PING               1
#define INST_READ               2
#define INST_WRITE              3
#define INST_REG_WRITE          4
#define INST_ACTION             5
#define INST_RESET              6
#define INST_SYNC_WRITE         131



class DynamixelComm : public Serial
{
public:

    DynamixelComm(const char * device_name, unsigned long baud_rate);
    ~DynamixelComm();

    unsigned char   CalculateChecksum(unsigned char * b);
    
    void            Send(unsigned char * b);
    int             Receive(unsigned char * b);
    
    bool            ReadMemoryBlock(int id, unsigned char * buffer, int fromAddress, int toAddress);
    bool            ReadAllData(int id, unsigned char * buffer);
    
    int             Move(int id, int pos, int speed);
    int             SetSpeed(int id, int speed);
    int             SetPosition(int id, int pos);
    int             SetTorque(int id, int value);
    
    void            SyncMoveWithSpeed(int * pos, int * speed, int n);
    void            SyncMoveWithIdAndSpeed(int * servo_id, int * pos, int * speed, int n);
    void            SyncMoveWithIdSpeedAndTorque(int * servo_id, int * pos, int * speed, int * torque, int n);
    
    int             GetPosition(int id);
    bool            Ping(int id);
};

#endif

