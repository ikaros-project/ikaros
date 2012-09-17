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



#include "DynamixelComm.h"


DynamixelComm::DynamixelComm(const char * serial_device, unsigned long baud_rate):
    Serial(serial_device, baud_rate)
{
    max_failed_reads = 1;
}



DynamixelComm::~DynamixelComm()
{
}



unsigned char
DynamixelComm::CalculateChecksum(unsigned char * b)
{
    unsigned char checksum = 0;
    for(int i=0; i<(b[3]+1); i++)
        checksum += b[i+2];
    checksum = ~checksum;
    return checksum;
}



void
DynamixelComm::Send(unsigned char * b)
{
    b[b[3]+3] = CalculateChecksum(b);

    SendBytes((char *)b, b[3]+4);
}



int
DynamixelComm::Receive(unsigned char * b)
{
    int c = ReceiveBytes((char *)b, 4);
    if(c < 4)
    {
//      printf("receive error (data size = %d)\n", c);
        return 0;
    }
    c += ReceiveBytes((char *)&b[4], b[3]);
    
    unsigned char checksum = CalculateChecksum(b);
    if(checksum != b[b[3]+3])
    {
        printf("receive error (data size = %d) incorrect checksum\n", c);
        return -1;
    }
       
    return c;
}



bool
DynamixelComm::ReadMemoryBlock(int id, unsigned char * buffer, int fromAddress, int toAddress)
{
    unsigned char outbuf[256] = {0XFF, 0XFF, id, 4, INST_READ, fromAddress, toAddress, 0X00};
    unsigned char inbuf[256];
    
    Send(outbuf);
    int n = Receive(inbuf);
    
    if(n==0)
        return false;
    
    // TODO: exit if checksum incorrect
    
    // TODO: check ID @ inbuf[2]
    // TODO: check ERROR @ inbuf[4], should be 0
    
    // copy data to buffer
    
    // print all values from servo
    //printDynamixelData(inbuf,0,50);
    
    
    for(int i=0; i<50; i++)
        buffer[i] = inbuf[i+5];
    
    return true;
}



bool
DynamixelComm::ReadAllData(int id, unsigned char * buffer)
{
    unsigned char outbuf[256] = {0XFF, 0XFF, id, 4, INST_READ, 0, 50, 0X00};
    unsigned char inbuf[256];
    
    Send(outbuf);
    int n = Receive(inbuf);
    
    if(n==0)
        return false;

    // TODO: exit if checksum incorrect
    
    // TODO: check ID @ inbuf[2]
    // TODO: check ERROR @ inbuf[4], should be 0
    
    // copy data to buffer
    
    for(int i=0; i<50; i++)
        buffer[i] = inbuf[i+5];
        
    return true;
}



int
DynamixelComm::Move(int id, int pos, int speed)
{
    unsigned char outbuf[256] = {0XFF, 0XFF, id, 7, INST_WRITE, P_GOAL_POSITION_L, pos % 256, pos / 256, speed % 256, speed / 256, 0X00}; // move to position with speed
    unsigned char inbuf[256];
    
    Send(outbuf);
    Receive(inbuf);

    return 1;
}



int
DynamixelComm::SetSpeed(int id, int speed)
{
    unsigned char outbuf[256] = {0XFF, 0XFF, id, 5, INST_WRITE, P_GOAL_SPEED_L, speed % 256, speed / 256, 0X00}; // write two bytes for present position
    unsigned char inbuf[256];
    
    Send(outbuf);
    Receive(inbuf);

    return 1;
}



int
DynamixelComm::SetPosition(int id, int pos)
{
    unsigned char outbuf[256] = {0XFF, 0XFF, id, 5, INST_WRITE, P_GOAL_POSITION_L, pos % 256, pos / 256, 0X00}; // write two bytes for present position
    unsigned char inbuf[256];
    
    Send(outbuf);
    Receive(inbuf);

    return 1;
}



int
DynamixelComm::SetTorque(int id, int value)
{
    unsigned char outbuf[256] = {0XFF, 0XFF, id, 4, INST_WRITE, P_TORQUE_ENABLE, value, 0X00}; // write two bytes for present position
    unsigned char inbuf[256];
    
    Send(outbuf);
    
    if(id != 254)
        Receive(inbuf);
    
    return 1;
}



void
DynamixelComm::SyncMoveWithSpeed(int * pos, int * speed, int n)
{
    unsigned char outbuf[256] =
        {0XFF, 0XFF, 0XFE,
        (4+1)*n+4,  // (datalength+1)*N+4 
        0X83,       // sync_write
        0X1E,       // address
        0x04        // length
        };
        
    for(int i=0; i<n; i++)
    {
        outbuf[7+5*i+0] = i+1;
        outbuf[7+5*i+1] = pos[i] % 256;
        outbuf[7+5*i+2] = pos[i] / 256;
        outbuf[7+5*i+3] = speed[i] % 256;
        outbuf[7+5*i+4] = speed[i] / 256;
    }
    
    Send(outbuf);
}



void
DynamixelComm::SyncMoveWithIdAndSpeed(int * servo_id, int * pos, int * speed, int n)
{
    unsigned char outbuf[256] =
    {0XFF, 0XFF, 0XFE,
        (4+1)*n+4,  // (datalength+1)*N+4 
        0X83,       // sync_write
        0X1E,       // address
        0x04        // length
    };
    
    for(int i=0; i<n; i++)
    {
        outbuf[7+5*i+0] = servo_id[i];
        outbuf[7+5*i+1] = pos[i] % 256;
        outbuf[7+5*i+2] = pos[i] / 256;
        outbuf[7+5*i+3] = speed[i] % 256;
        outbuf[7+5*i+4] = speed[i] / 256;
    }
    
    Send(outbuf);
}



void
DynamixelComm::SyncMoveWithIdSpeedAndTorque(int * servo_id, int * pos, int * speed, int * torque, int n)
{
    unsigned char outbuf[256] =
    {0XFF, 0XFF, 0XFE,
        (6+1)*n+4,  // (datalength+1)*N+4 
        0X83,       // sync_write
        0X1E,       // address
        0x06        // length
    };
    
    for(int i=0; i<n; i++)
    {
        outbuf[7+7*i+0] = servo_id[i];
        outbuf[7+7*i+1] = pos[i] % 256;
        outbuf[7+7*i+2] = pos[i] / 256;
        outbuf[7+7*i+3] = speed[i] % 256;
        outbuf[7+7*i+4] = speed[i] / 256;
        outbuf[7+7*i+5] = torque[i] % 256;
        outbuf[7+7*i+6] = torque[i] / 256;
    }
    
    Send(outbuf);
}



int
DynamixelComm::GetPosition(int id)
{                                                      
    unsigned char outbuf[256] = {0XFF, 0XFF, id, 4, INST_READ, P_PRESENT_POSITION_L, 2, 0X00}; // read two bytes for present position
    unsigned char inbuf[256];
    
    Send(outbuf);
    
    // set a timout first

    Receive(inbuf);
    
    // exit if checksum incorrect
    
    // check ID @ inbuf[2]
    // check ERROR @ inbuf[4], should be 0
    
    return inbuf[5]+256*inbuf[6];
}



// ping a dynamixel to see if it exists

bool
DynamixelComm::Ping(int id)
{
    unsigned char outbuf[256] = {0XFF, 0XFF, id, 2, INST_PING, 0X00};
    unsigned char inbuf[256];
    
    Send(outbuf);
    int n = Receive(inbuf);
    
    return (n != 0);
}


