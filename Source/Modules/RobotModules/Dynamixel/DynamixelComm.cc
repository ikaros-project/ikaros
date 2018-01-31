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
	// Errors
	crcError = missingBytesError = notCompleteError = extendedError = 0;
}

DynamixelComm::~DynamixelComm()
{
	Flush();
}

// Ping a dynamixel to see if it exists. Return protocol version
int DynamixelComm::Ping(int id)
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

int DynamixelComm::WriteToServo(int id, int protocol, int adress, unsigned char * data, int dSize)
{
	switch (protocol) {
		case 1:
			AddDataSyncWrite1(id, adress, data, dSize);
			return SendSyncWrite1();
			break;
		case 2:
			AddDataBulkWrite2(id, adress, data, dSize);
			return SendBulkWrite2();
			break;
			
		default:
			break;
	}
	
	return false;
	
}
