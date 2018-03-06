//
//    DynamixelComm.cc		Class to communicate with Dynamixel servos (version 1)
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
//    Created: March 2016
//

#include "DynamixelComm.h"

// MARK: SyncWrite
// AddDataSyncWrite1 will add data to a syncwrite package similar to the addDataBulkWrite.
// When calling AddDataSyncWrite1(int ID, int adress, unsigned char * data, int dSize) multiple times int adress and int dSize must not change.
// Ex. AddDataSyncWrite1(servo1, adress, value1, length of value) next call AddDataSyncWrite1(servo2, adress, value2, length of value) etc. adress and length of value must be the same.
bool DynamixelComm::AddDataSyncWrite1(int ID, int adress, unsigned char * data, int dSize)
{
	// Grab the first adress and dSize.
	if (syncWriteBufferLength == -1)
	{
		syncWriteAdress = adress;
		syncWriteBlockSize = dSize;
	}
	
#ifdef LOG_COMM
	printf("DynamixelComm (AddDataSyncWrite1) Adding data to syncwrite ID: %i, adress %i, start adress %i, length %i, total length %i\n", ID, adress, syncWriteAdress, dSize, syncWriteBlockSize);
#endif
	// Check syncWriteBuffer overflow
	if (syncWriteBufferLength + 1 + dSize > 1024)  // 1 = id
		return false;
	
	syncWriteBuffer[++syncWriteBufferLength] = ID;
	memcpy(&syncWriteBuffer[++syncWriteBufferLength], data, dSize);
	syncWriteBufferLength+=dSize-1;
	
	//PrintDataSyncWrite1();
	return true;
}
void DynamixelComm::PrintDataSyncWrite1()
{
	printf("DynamixelComm (PrintDataSyncWrite1) Adress %i, Blocksize %i\n",syncWriteAdress,syncWriteBlockSize);
	for (int i = 0; i <= syncWriteBufferLength; i++)
		printf("%3i  \t\tBUFFER: \%#04X\t(%3i)\n", i, syncWriteBuffer[i], syncWriteBuffer[i]);
}

bool DynamixelComm::SendSyncWrite1()
{
#ifdef LOG_COMM
	printf("DynamixelComm (SendSyncWrite1) Sending SyncWrite command\n");
#endif
	if (syncWriteBufferLength == -1)
		return false;
	
	unsigned int length=1+syncWriteBufferLength+4; // Length is the packet length AFTER packet length feild. Inst + Parameters + CRC  (L+1) X N + 4   (L: Data Length per RX-64, N: the number of RX-64s)
	unsigned int lengthPackage = length + INSTRUCTION_HEADER_1; // Total length of the package with all feilds
	unsigned char id = 0XFE; // Broadcast
	//unsigned char length = static_cast<unsigned char>(syncWriteBufferLength+1+4); 		// Length (L+1) X N + 4   (L: Data Length per RX-64, N: the number of RX-64s)
	unsigned char adress = static_cast<unsigned char>(syncWriteAdress);	// Start address to write Data
	unsigned char paramterLength = static_cast<unsigned char>(syncWriteBlockSize); // Length of data to write
	
	// Check if the length of the packages is vailed. The maximum buffer size for the dynamixel is 147 bytes.
	if (lengthPackage > DYNAMIXEL_MAX_BUFFER)
	{
		printf("DynamixelComm: Message size is over 143 bytes. Please reduce the number of servos of data sent to the servos\n");
		syncWriteBufferLength = -1;
		return false;
	}
	
	// Message
	unsigned char outbuf[256] = {0XFF, 0XFF, id, static_cast<unsigned char>(length), INST_SYNC_WRITE, adress, paramterLength};
	// Message data
	memcpy(&outbuf[INSTRUCTION_HEADER_1 + 2], &syncWriteBuffer[0], (syncWriteBufferLength+1) * sizeof(unsigned char));
	
	//PrintFullInstructionPackage1(outbuf);
	Send1(outbuf);
	
	// Reset memory
	syncWriteBufferLength = -1;
	syncWriteBlockSize = 0;
	syncWriteAdress = 0;
	return true;
}

// MARK: Read memory block
bool DynamixelComm::ReadMemoryRange1(int id, unsigned char * buffer, int fromAdress, int toAdress)
{
#ifdef LOG_COMM
	printf("DynamixelComm (ReadMemoryRange1): (id:%i) (%i-%i)\n",id, fromAdress, toAdress);
#endif
	
	int bytesToRead = toAdress-fromAdress+1;
	unsigned char outbuf[256] = {0XFF,0XFF, static_cast<unsigned char>(id), 4, INST_READ, static_cast<unsigned char>(fromAdress), static_cast<unsigned char>(bytesToRead), 0X00};
	Send1(outbuf);
	//PrintFullInstructionPackage1(outbuf);
	
	unsigned char inbuf[BUFFER_SIZE];
	int n = Receive1(inbuf);
	//PrintFullStatusPackage1(inbuf);
	
	// Do all message checking here
	if (n == ERROR_NO_HEADER)
	{
#ifdef LOG_COMM_ERROR
		printf("DynamixelComm (ReadMemoryRange1): Did not get the header (4 bytes). Flushing (id:%i)\n",id);
#endif
		FlushIn();
		missingBytesError++;
		return (false);
	}
	if (n == ERROR_CRC)
	{
#ifdef LOG_COMM_ERROR
		printf("DynamixelComm (ReadMemoryRange1): CRC error. Flushing (id:%i)\n",id);
#endif
		FlushIn();
		crcError++;
		return (false);
	}
	if (n == ERROR_NOT_COMPLETE)
	{
#ifdef LOG_COMM_ERROR
		printf("DynamixelComm (ReadMemoryRange1): Did not get all bytes expected. Flushing (id:%i)\n",id);
#endif
		FlushIn();
		notCompleteError++;
		return (false);
	}
	
	// Extended check of the message
	// Checking first bytes id, model etc to make sure these are the right ones.
	//	int checkBytesToAdress = 6;
	//	if (bytesToRead < checkBytesToAdress)
	//		checkBytesToAdress = bytesToRead;
	//
	//	for (int i = 0; i <checkBytesToAdress; i++)
	//	{
	//		if (buffer[i] != inbuf[5+i] && buffer[i] != 0)
	//		{
	//#ifdef LOG_COMM_ERROR
	//			printf("DynamixelComm (ReadMemoryRange1): Extended check byte %i of id %i (%i != %i) does not match\n",i, id, buffer[i], inbuf[i]);
	//#endif
	//			extendedError++;
	//			return (false);
	//		}
	//	}
	// Parse servo error byte
	GetServoError2(inbuf[ERROR_BYTE_1]);
	
#ifdef LOG_COMM
	printf("DynamixelComm (ReadMemoryRange1): Fill internal buffer from received buffer. (id:%i)\n",id);
#endif
	
	memcpy(&buffer[0], &inbuf[5], bytesToRead * sizeof(unsigned char));
	return true;
}
// MARK: Send/Receive
void DynamixelComm::Send1(unsigned char * b)
{
#ifdef LOG_COMM
	printf("DynamixelComm (Send1)\n");
#endif
	b[b[LEN_BYTE_1]+LEN_BYTE_1] = CalculateChecksum(b);
	SendBytes((char *)b, b[LEN_BYTE_1]+4);
}

// Calculate timeout
// Timeout = com->time_per_byte*packet_length + (LATENCY_TIMER*2) + 2.0 // Numbers from Robotis sdk. This might need to be tuned for different platform. Will be testing with raspberry pi 3.

int DynamixelComm::Receive1(unsigned char * b)
{	
#ifdef LOG_COMM
	printf("DynamixelComm (Receive1)\n");
#endif
	int c = ReceiveBytes((char *)b, LEN_BYTE_1 + 1, (this->time_per_byte*(LEN_BYTE_1 +1)) + 8 + serialLatency); // Get first part of message to calculate total length of package.
	if(c < LEN_BYTE_1 + 1)
	{
#ifdef LOG_COMM
		printf("DynamixelComm (Receive1): Did not get header (Timed out. Got %i bytes)\n",c);
#endif
		FlushIn();
		return ERROR_NO_HEADER;
	}
	int lengthOfPackage = b[LEN_BYTE_1];
	c += ReceiveBytes((char *)&b[LEN_BYTE_1+1], lengthOfPackage,  (this->time_per_byte*lengthOfPackage) + 8 + serialLatency);
	
	if(c < lengthOfPackage)
	{
#ifdef LOG_COMM
		printf("DynamixelComm (Receive1): Did not get all message (Timed out. Got %i bytes)\n",c);
#endif
		FlushIn();
		return ERROR_NOT_COMPLETE;
	}
	unsigned char checksum = CalculateChecksum(b);
	if(checksum != b[b[3]+3])
	{
		FlushIn();
		return ERROR_CRC;
	}
	return c;
}

// MARK: MISC
bool DynamixelComm::Ping1(int id)
{
#ifdef LOG_COMM
	printf("DynamixelComm (Ping1) ID %i\n", id);
#endif
	Flush();
	unsigned char outbuf[256] = {0XFF, 0XFF, static_cast<unsigned char>(id), 2, INST_PING, 0X00};
	Send1(outbuf);
	//PrintFullInstructionPackage1(outbuf);
	
	unsigned char inbuf[256];
	//int n = Receive1(inbuf);
	return (Receive1(inbuf) > 0);

	//PrintFullStatusPackage1(inbuf);
	//return (Receive1(inbuf) < 0);
//		if (n < 0)
//			return (false);
//		else
//			return (true);
}

void DynamixelComm::Reset1(int id)
{
	printf("Dynamixel: Sending Reset id: %i\n", id);
	unsigned char outbuf[256] = {0XFF, 0XFF, static_cast<unsigned char>(id), 2, INST_RESET, 0X00};
	Send1(outbuf);
	unsigned char inbuf[256];
	Receive1(inbuf); // Ignore status package
}
// MARK: Print
void DynamixelComm::PrintFullInstructionPackage1(unsigned char * package)
{
	int k = 0;
	int l = 0;
	int totalLength = INSTRUCTION_HEADER_1 - 1 + package[LEN_BYTE_1]; // -1 Error byte
	int crcPos = totalLength - 1;
	
	printf("\n== Instruction Packet (Send) (%i) =\n",totalLength);
	while (k < totalLength) {
		if (k == 0)
			printf("============= Start Bytes =======\n");
		if (k == 2)
			printf("============= ID ================\n");
		if (k == 3)
			printf("============= Length ============\n");
		if (k == 4)
			printf("============= Inst ==============\n");
		if (k == 5)
			printf("============= Data bytes ========\n");
		if (k == crcPos)
			printf("============= CRC ===============\n");
		
		if (k > INSTRUCTION_HEADER_1 - 1 and k < crcPos)
			printf("%3i (%2i) BUFFER: %#04X (%3i)\n", k, l++, package[k],package[k]);
		else
			printf("%3i \t\tBUFFER: %#04X \t(%3i)\n", k, package[k], package[k]);
		k++;
	}
}
void DynamixelComm::PrintMemory1(unsigned char * m, int from, int to)
{
	printf("\n======= Memory Package  ======\n");
	for(int j=from; j<to; j++)
		printf("%3i \t\tBUFFER: %#04X \t(%3i)\n", j, m[j],m[j]);
}

void DynamixelComm::PrintFullStatusPackage1(unsigned char * package)
{
	int k = 0;
	int l = 0;
	int totalLength = STATUS_HEADER_1 - 1 + package[LEN_BYTE_1]; // - 1 error byte
	int crcPos = totalLength - 1;
	printf("\n== Status Packet (Send) (%i) =====\n",totalLength);
	while (k < totalLength) {
		if (k == 0)
			printf("============= Start Bytes =======\n");
		if (k == 2)
			printf("============= ID ================\n");
		if (k == 3)
			printf("============= Length ============\n");
		if (k == 4)
			printf("============= Error =============\n");
		if (k == 5)
			printf("============= Data bytes ========\n");
		if (k == crcPos)
			printf("============= CRC ===============\n");
		
		if (k > STATUS_HEADER_1 - 1 and k < crcPos)
			printf("%3i (%2i) BUFFER: %#04X (%3i)\n", k, l++, package[k],package[k]);
		else
			printf("%3i \t\tBUFFER: %#04X \t(%3i)\n", k, package[k], package[k]);
		k++;
	}
}

// MARK: CRC
unsigned char DynamixelComm::CalculateChecksum(unsigned char * b)
{
	unsigned char checksum = 0;
	for(int i=0; i<(b[3]+1); i++)
		checksum += b[i+2];
	checksum = ~checksum;
	return checksum;
}

// MARK: Error
void DynamixelComm::GetServoError1(unsigned char errorByte)
{
	
	// Bit 7 0 -
	// Bit 6 Instruction Error In case of sending an undefined instruction or delivering the         action command without the reg_write command, it is set as 1.
	// Bit 5 Overload Error When the current load cannot be controlled by the set Torque, it is set as 1.
	// Bit 4 Checksum Error When the Checksum of the transmitted Instruction Packet is incorrect, it is set as 1.
	// Bit 3 Range Error When a command is out of the range for use, it is set as 1.
	// Bit 2 Overheating Error When internal temperature of Dynamixel is out of the range of operating temperature set in the Control table, it is set as 1.
	// Bit 1 Angle Limit Error When Goal Position is written out of the range from CW Angle Limit to CCW Angle Limit , it is set as 1.
	// Bit 0 Input Voltage Error When the applied voltage is out of the range of operating voltage set in the Control table, it is as 1.
	
	errorServoInputVoltage = (errorByte >> 0) & 0x1;
	errorServoAngleLimit = (errorByte >> 1) & 0x1;
	errorServoOverHeating = (errorByte >> 2) & 0x1;
	errorServoRange = (errorByte >> 3) & 0x1;
	errorServoChecksum = (errorByte >> 4) & 0x1;
	errorServoOverload = (errorByte >> 5) & 0x1;
	errorServoIntruction = (errorByte >> 6) & 0x1;
	//errorServoInputVoltage = (errorByte >> 7) & 0x1;
	
#ifdef LOG_COMM_ERROR
	if (ErrorServo2 != 0)
	{
		printf("DynamixelComm: Error byte\n");
		printf("%i %i %i %i %i %i %i\n",errorServoInputVoltage,errorServoAngleLimit,errorServoOverHeating,errorServoRange,errorServoChecksum,errorServoOverload,errorServoIntruction);
	}
#endif
	
}
