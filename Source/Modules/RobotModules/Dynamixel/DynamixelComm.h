//
//    DynamixelComm.h
//
//    Copyright (C) 2018  Birger Johansson
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
//


#ifndef DYNAMIXELCOMM
#define DYNAMIXELCOMM

//#define LOG_COMM_ERROR
//#define LOG_COMM

#include "IKAROS.h"

// Protocol version 1 instruction package
#define ID_BYTE_1                   2
#define LEN_BYTE_1                  3
#define INST_BYTE_1                 4
#define ERROR_BYTE_1                5

// Protocol version 2 instruction package
#define ID_BYTE_2                   4
#define LEN_L_BYTE_2                5
#define LEN_H_BYTE_2                6
#define INST_BYTE_2                 7
#define ERROR_BYTE_2                8

// Protocol version length
#define INSTRUCTION_HEADER_1 5
#define STATUS_HEADER_1 5

// Protocol version 2 length
#define INSTRUCTION_HEADER_2 8		// Header + Reserved + Packet ID + Packet Length + Instruction
#define STATUS_HEADER_2 9			// Header + Reserved + Packet ID + Packet Length + Instruction + Error

// Common (Protocol instructions)
#define INST_PING               1
#define INST_READ               2
#define INST_WRITE              3
#define INST_REG_WRITE          4
#define INST_ACTION             5
#define INST_RESET              6
#define INST_REBOOT             8
#define INST_STATUS_RETURN      85
#define INST_SYNC_READ          130
#define INST_SYNC_WRITE         131
#define INST_BULK_READ          146
#define INST_BULK_WRITE         147


#define MAX_SERVOS 256
#define DYNAMIXEL_MAX_BUFFER 143

#define BUFFER_SIZE 1024


// Errors
#define ERROR_NO_HEADER -1
#define ERROR_CRC -2
#define ERROR_EXT -3
#define ERROR_NOT_COMPLETE -4

/**
 * Class for serial communication with dynamixel servos
 */

class DynamixelComm : public Serial
{
public:
	
	DynamixelComm(const char * device_name, unsigned long baud_rate);
	~DynamixelComm();
	// Can be use for both protocols
	int             Ping(int id);
	void            Reset(int id, int protocol);
	bool            ReadMemoryRange(int id, int protocol, unsigned char * buffer, int fromAddress, int toAddress);
	int				WriteToServo(int id, int protocol, int adress, unsigned char * data, int dSize);
	
	// Protocol version 1
	void            Send1(unsigned char * b);
	int             Receive1(unsigned char * b);
	void            Reset1(int id);
	bool            Ping1(int id);
	void            PrintFullInstructionPackage1(unsigned char * package);
	void            PrintFullStatusPackage1(unsigned char * package);
	void            PrintMemory1(unsigned char * b, int fromAddress, int toAddress);
	bool            ReadMemoryRange1(int id, unsigned char * buffer, int fromAddress, int toAddress);
	bool 			AddDataSyncWrite1(int ID, int adress, unsigned char * data, int dSize);
	void 			PrintDataSyncWrite1();
	bool 			SendSyncWrite1();
	
	unsigned char   syncWriteBuffer[BUFFER_SIZE];
	int				syncWriteBufferLength = -1;
	int				syncWriteAdress = 0;
	int				syncWriteBlockSize = 0;
	
	unsigned char   CalculateChecksum(unsigned char * b);
	void 			GetServoError1(unsigned char errorByte);
	
	bool			errorServoInputVoltage;
	bool			errorServoAngleLimit;
	bool			errorServoOverHeating;
	bool			errorServoRange;
	bool			errorServoChecksum;
	bool			errorServoOverload;
	bool			errorServoIntruction;
	
	// Protocol version 2
	void            Send2(unsigned char * b);
	int             Receive2(unsigned char * b);
	void            Reset2(int id);
	bool            Ping2(int id);
	void            PrintFullInstructionPackage2(unsigned char * package);
	void            PrintFullStatusPackage2(unsigned char * package);
	void            PrintMemory2(unsigned char * b, int fromAddress, int toAddress);
	bool            ReadMemoryRange2(int id, unsigned char * buffer, int fromAddress, int toAddress);
	
	unsigned short  update_crc(unsigned short crc_accum, unsigned char *data_blk_ptr, unsigned short data_blk_size);
	bool            checkCrc(unsigned char * package);
	bool 			AddDataBulkWrite2(int ID, int adress, unsigned char * data, int dSize);
	void 			PrintDataBulkWrite2();
	bool 			SendBulkWrite2();
	
	unsigned char   bulkWriteBuffer[BUFFER_SIZE];
	int				bulkWriteBufferLength  = -1;
	
	int				serialLatency;
	
	int 			crcError;
	int 			missingBytesError;
	int 			notCompleteError;
	int 			extendedError;
	
	float 			sendTimer;
	float 			recieveTimer;
	float 			recieveTimerTotal;
	
	void 			GetServoError2(unsigned char errorByte);
	
	bool			errorServo2;
	bool			errorServoResaultFail2;
	bool			errorServoIntruction2;
	bool			errorServoCrc2;
	bool			errorServoRange2;
	bool			errorServoLength2;
	bool			errorServoLimit2;
	bool			errorServoAccess2;
	
};
#endif
