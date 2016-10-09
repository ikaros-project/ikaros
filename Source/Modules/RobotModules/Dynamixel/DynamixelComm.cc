//
//    DynamixelComm.cc		Class to communicate with Dynamixel servos (version 2)
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
//    Created: March 2016
//

#include "DynamixelComm.h"


DynamixelComm::DynamixelComm(const char * serial_device, unsigned long baud_rate):
Serial(serial_device, baud_rate)
{
    max_failed_reads = 1;
}

DynamixelComm::~DynamixelComm()
{}

// Ping a dynamixel to see if it exists. Return protocol version
int
DynamixelComm::Ping(int id)
{
    if (Ping1(id))
        return(1);
    if (Ping2(id))
        return(2);
    return (-1);
}

void
DynamixelComm::Reset(int id, int protocol)
{
    switch (protocol) {
        case 1:
            return Reset1(id);
            break;
        case 2:
            return Reset2(id);
            break;
            
        default:
            break;
    }
    return;
    
}

void
DynamixelComm::WriteToServo(int * servo_id, int * mask, int protocol, unsigned  char ** buffer, int * ikarosInBind, int * size, int n)
{
    switch (protocol) {
        case 1:
            return SyncWriteWithIdRange1(servo_id, mask, buffer, ikarosInBind, size, n); // Size and adress must be fixed. BUT first servo may not have size of paramter.
            break;
        case 2:
            return BulkWrite2(servo_id, mask, buffer, ikarosInBind, size, n); // Must use bulkwrite as paramters differer in size and adress in dynamixel 2
            break;
            
        default:
            break;
    }
    return;
}

bool
DynamixelComm::ReadMemoryRange(int id, int protocol, unsigned char * buffer, int from, int to)
{
    switch (protocol) {
        case 1:
            return ReadMemoryRange1(id,  buffer, from, to);
            break;
        case 2:
            return ReadMemoryRange2(id,  buffer, from, to);
            
            break;
            
        default:
            break;
    }
    
    return false;
}