//
//    DynamixelComm.cc		Class to communicate with Dynamixel servos (version 2)
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
// This code has only been tested with the XL-320 servo.

#include "DynamixelComm.h"

int DynamixelComm::AddDataBulkWrite2(int ID, int adress, unsigned char * data, int dSize)
{
#ifdef LOG_COMM
	printf("DynamixelComm (AddDataBulkWrite2) Adding data to bulkwrite ID: %i, adress %i, length %i\n", ID, adress, dSize);
#endif

	if (bulkWriteBufferLength + 1 + 2 + 2 + dSize > 1024)  // 1 = id 2 = adress 2 = size
		return false;
	
	bulkWriteBuffer[++bulkWriteBufferLength] = ID; // ID
	bulkWriteBuffer[++bulkWriteBufferLength] = adress&0xff;
	bulkWriteBuffer[++bulkWriteBufferLength] = adress>>8&0xff;
	bulkWriteBuffer[++bulkWriteBufferLength] = dSize&0xff;
	bulkWriteBuffer[++bulkWriteBufferLength] = dSize>>8&0xff;
	memcpy(&bulkWriteBuffer[++bulkWriteBufferLength], data, dSize * sizeof(unsigned char));
	bulkWriteBufferLength+=dSize-1;
	
	//PrintDataBulkWrite2();
	return 0;
}

void DynamixelComm::PrintDataBulkWrite2()
{
	printf("DynamixelComm (PrintDataBulkWrite2)\n");
	for (int i = 0; i <= bulkWriteBufferLength; i++) {
		printf("%3i  \t\tBUFFER: \%#04X\t(%3i)\n", i, bulkWriteBuffer[i], bulkWriteBuffer[i]);
	}
}

int DynamixelComm::SendBulkWrite2()
{
#ifdef LOG_COMM
	printf("DynamixelComm (SendBulkWrite2) Sending BulkWrite command.\n");
#endif
	if (bulkWriteBufferLength == -1)
		return -1;
	
	unsigned int length=1+bulkWriteBufferLength+2; // Length is the packet length AFTER packet length feild. Inst + Parameters + CRC
	unsigned int lengthPackage = length + BULK_WRITE_HEADER_2-1; // Total length of the package with all feilds. BULK_WRITE_HEADER_2-1 as inst feild is included in BULK_WRITE_HEADER_2
	
	if (lengthPackage > DYNAMIXEL_MAX_BUFFER)
	{
		printf("DynamixelCommunication: Message size is over 143 bytes. Please reduce the number of servos of data sent to the servos\n");
		bulkWriteBufferLength = -1;
		return -1;
	}
	
	// Message
	unsigned char outbuf[256] = {0XFF, 0XFF, 0XFD, 0X00, 0XFE, static_cast<unsigned char>((length&0xff)), static_cast<unsigned char>((length>>8)&0xff), INST_BULK_WRITE};
	// Message data
	memcpy(&outbuf[SYNC_WRITE_HEADER_1], &bulkWriteBuffer[0], (bulkWriteBufferLength+1) * sizeof(unsigned char));
	
	//PrintFullInstructionPackage2(outbuf);
	//Send1(outbuf);
	
	// Reset memory
#ifdef LOG_COMM
	printf("DynamixelComm (SendBulkWrite2) Reset BulkWrite memory\n");
#endif
	bulkWriteBufferLength = -1;
	return 0;
}

// MARK: Read memory block
bool DynamixelComm::ReadMemoryRange2(int id, unsigned char * buffer, int from, int to)
{
#ifdef LOG_COMM
	printf("DynamixelComm (ReadMemoryRange2): (id:%i) (%i-%i)\n",id, from, to);
#endif
	
	int bytesToRead = to-from+1;
	unsigned int parameter_length = 4; // Read has 4 byte
	unsigned int length=3+parameter_length;
	
	unsigned char outbuf[256] = {0XFF, 0XFF, 0XFD, 0X00, static_cast<unsigned char>(id), static_cast<unsigned char>(length&0xff), static_cast<unsigned char>((length>>8)&0xff), INST_READ, static_cast<unsigned char>(from&0xff), static_cast<unsigned char>((from>>8)&0xff), static_cast<unsigned char>(bytesToRead&0xff), static_cast<unsigned char>((bytesToRead>>8)&0xff)};
	// Two more bytes for crc
	// Calucate CRC (Function from robotis)
	unsigned short crc = update_crc(0,outbuf,12);
	
	outbuf[12]=crc&0xff;
	outbuf[13]=(crc>>8)&0xff;
	Send2(outbuf);
	unsigned char inbuf[1024];
	int n = Receive2(inbuf);
	
	// Do all message checking here
	if(n==ERROR_NO_HEADER)
	{
#ifdef LOG_COMM_ERROR
		printf("DynamixelComm (ReadMemoryRange2): Did not get the header (4 bytes). Flushing (id:%i)\n",id);
#endif
		FlushIn();
		missingBytesError++;
		return (false);
	}
	if (n == ERROR_CRC)
	{
#ifdef LOG_COMM_ERROR
		printf("DynamixelComm (ReadMemoryRange2): CRC error. Flushing (id:%i)\n",id);
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
	int checkBytesToAdress = 6;
	if (bytesToRead < checkBytesToAdress)
		checkBytesToAdress = bytesToRead;
	
	for (int i = 0; i <checkBytesToAdress; i++)
	{
		if (buffer[i] != inbuf[5+i] && buffer[i] != 0)
		{
#ifdef LOG_COMM_ERROR
			printf("DynamixelComm (ReadMemoryRange2): Extended check byte %i of id %i (%i != %i) does not match\n",i, id, buffer[i], inbuf[i]);
#endif
			extendedError++;
			return (false);
		}
	}
#ifdef LOG_COMM
	printf("DynamixelComm (ReadMemoryRange2): Fill internal buffer from recieved buffer. (id:%i)\n",id);
#endif
	
	// Parse servo error byte
	getServoError2(inbuf[ERROR_BYTE_2]);
	
	memcpy(&buffer[0], &inbuf[RECEIVE_HEADER_2+from+1], bytesToRead * sizeof(unsigned char)-1); // -1 error bit in protocol2
	return true;
}
// MARK: Send/Receive
void DynamixelComm::Send2(unsigned char * b)
{
#ifdef LOG_COMM
	printf("DynamixelComm (Send2)\n");
#endif
	SendBytes((char *)b, (b[LEN_H_BYTE_2]<<8) + b[LEN_L_BYTE_2] + 7);
}

int DynamixelComm::Receive2(unsigned char * b)
{
#ifdef LOG_COMM
	printf("DynamixelComm (Receive2)\n");
#endif
	int c = ReceiveBytes((char *)b, RECEIVE_HEADER_2-1, (this->time_per_byte*RECEIVE_HEADER_2-1) + 8 + serialLatency); // Get header
	
	if(c < RECEIVE_HEADER_2-1)
	{
#ifdef LOG_COMM
		printf("DynamixelComm (Receive2): Did not get header (Timed out. Got %i bytes)\n",c);
#endif
		return ERROR_NO_HEADER;
	}
	int lengthOfPackage = (b[LEN_H_BYTE_2]<<8) + b[LEN_L_BYTE_2];
	c += ReceiveBytes((char *)&b[LEN_H_BYTE_2+1], lengthOfPackage, (this->time_per_byte*lengthOfPackage) + 8 + serialLatency); // Get the rest of the package

	if(c < lengthOfPackage)
	{
#ifdef LOG_COMM
		printf("DynamixelComm (Receive2): Did not get all message (Timed out. Got %i bytes)\n",c);
#endif
		return ERROR_NOT_COMPLETE;
	}
	if (!checkCrc(b))
		return ERROR_CRC;
	
	return c;
}
// MARK: MISC
bool DynamixelComm::Ping2(int id)
{
#ifdef LOG_COMM
	printf("DynamixelComm (Ping2) ID %i\n", id);
#endif
	Flush();
	unsigned int parameter_length = 0; // Ping has no data
	unsigned int length=3+parameter_length;
	unsigned char outbuf[256] = {0XFF,0XFF,0XFD,0X00,static_cast<unsigned char>(id),static_cast<unsigned char>(length&0xff),static_cast<unsigned char>((length>>8)&0xff), INST_PING}; // Two more bytes for crc
	
	// Calucate CRC (Function from robotis)
	unsigned short crc = update_crc(0,outbuf,8);
	
	outbuf[8]=crc&0xff;
	outbuf[9]=(crc>>8)&0xff;
	Send2(outbuf);
	
	unsigned char inbuf[256];
	int n = Receive2(inbuf);
	if (n < 0)
		return (false);
	else
		return (true);
}
void DynamixelComm::Reset2(int id)
{
	printf("Dynamixel: Trying to reset id: %i\n", id);
	unsigned char outbuf[256] = {0XFF,0XFF,0XFD,0X00, static_cast<unsigned char>(id),2,INST_RESET, 0X00};
	Send2(outbuf);
	// Receive status packet but do not do anyting with it.
	unsigned char inbuf[256];
	Receive2(inbuf);
}
// MARK: Print
void DynamixelComm::PrintFullInstructionPackage2(unsigned char * package)
{
	PrintPartInstructionPackage2(package, 1, (package[LEN_H_BYTE_2]<<8) + package[LEN_L_BYTE_2]-2-1); // -2 Crc
}
void DynamixelComm::PrintPartInstructionPackage2(unsigned char * outbuf, int from, int to)
{
	int datalength = to-from+1;                                                         // Only a part of the parameters
	int totalLengthOfPackage = (outbuf[LEN_H_BYTE_2]<<8) + outbuf[LEN_L_BYTE_2] + 7;    // length of package
	
	printf("\n====== Instruction Packet (Send) (%i) ========\n",totalLengthOfPackage);
	for(int j=0; j<RECEIVE_HEADER_2; j++){
		if (j == 0)
			printf("============= Start Bytes =======\n");
		if (j == 4)
			printf("============= ID ================\n");
		if (j == 5)
			printf("============= Length ============\n");
		if (j == 7)
			printf("============= Inst ==============\n");
		printf("%3i  \t\tBUFFER: \%#04X\t(%3i)\n", j, outbuf[j], outbuf[j]);
	}
	
	printf("*********** Parameters (%i) ******\n",datalength);
	
	for(int j=from-1; j<=to-1; j++)
	{
		if (j == 0)
			printf("============= Data bytes ========\n");
		printf("%3i (%2i) \tBUFFER: %#04X \t(%3i)\n", RECEIVE_HEADER_2+j, from+j-1, outbuf[RECEIVE_HEADER_2+j],outbuf[RECEIVE_HEADER_2+j]);
	}
	printf("*********************************\n");
	
	printf("============ CRC ================\n");
	printf("%3i \t\tBUFFER: %#04X \t(%3i)\n", totalLengthOfPackage-2, outbuf[totalLengthOfPackage-2],outbuf[totalLengthOfPackage-2]);
	printf("%3i \t\tBUFFER: %#04X \t(%3i)\n", totalLengthOfPackage-1, outbuf[totalLengthOfPackage-1],outbuf[totalLengthOfPackage-1]);
}
void DynamixelComm::PrintMemory2(unsigned char * m, int from, int to)
{
	printf("\n======= Memory Package  ======\n");
	for(int j=from; j<=to; j++)
		printf("%3i \t\tBUFFER: %#04X \t(%3i)\n", j, m[j],m[j]);
}
void DynamixelComm::PrintFullStatusPackage2(unsigned char * package)
{
	PrintPartStatusPackage2(package, 1, (package[LEN_H_BYTE_2]<<8) + package[LEN_L_BYTE_2]-2-1);
}
void DynamixelComm::PrintPartStatusPackage2(unsigned char * outbuf, int from, int to)
{
	int datalength = to-from+1;                                    // Data range that should be presented
	int totalLengthOfPackage = (outbuf[LEN_H_BYTE_2]<<8) + outbuf[LEN_L_BYTE_2] + 7;    // length of package
	
	printf("\n======= Status Package (%i) ======\n",totalLengthOfPackage);
	for(int j=0; j<RECEIVE_HEADER_2; j++){
		if (j == 0)
			printf("============= Start Bytes =======\n");
		if (j == 4)
			printf("============= ID ================\n");
		if (j == 5)
			printf("============= Length ============\n");
		if (j == 7)
			printf("============= Inst ==============\n");
		printf("%3i  \t\tBUFFER: \%#04X\t(%3i)\n", j, outbuf[j], outbuf[j]);
	}
	
	printf("*********** Parameters (%i) ******\n",datalength);
	
	for(int j=from-1; j<=to-1; j++)
	{
		if (j == 0)
			printf("============= Error byte ========\n");
		if (j == 1)
			printf("============= Data bytes ========\n");
		printf("%3i (%2i) \tBUFFER: %#04X \t(%3i)\n", RECEIVE_HEADER_2+j, j, outbuf[RECEIVE_HEADER_2+j],outbuf[RECEIVE_HEADER_2+j]);
	}
	printf("*********************************\n");
	
	printf("============ CRC ================\n");
	printf("%3i \t\tBUFFER: %#04X \t(%3i)\n", totalLengthOfPackage-2, outbuf[totalLengthOfPackage-2],outbuf[totalLengthOfPackage-2]);
	printf("%3i \t\tBUFFER: %#04X \t(%3i)\n", totalLengthOfPackage-1, outbuf[totalLengthOfPackage-1],outbuf[totalLengthOfPackage-1]);
}

// MARK: CRC
bool DynamixelComm::checkCrc(unsigned char * package)
{
	unsigned short crc = update_crc(0,package, 7 + (package[LEN_H_BYTE_2]<<8) + package[LEN_L_BYTE_2] - 2); // Header + parameteres - crc
	if((crc&0xff) == ((package[LEN_H_BYTE_2]<<8) + package[LEN_L_BYTE_2] -2) &&  ((crc>>8)&0xff) == (package[LEN_H_BYTE_2]<<8) + package[LEN_L_BYTE_2] - 1)
		return 0;
	
	return true;
}

unsigned short DynamixelComm::update_crc(unsigned short crc_accum, unsigned char *data_blk_ptr, unsigned short data_blk_size)
{
	unsigned short i, j;
	unsigned short crc_table[256] = {0x0000,
		0x8005, 0x800F, 0x000A, 0x801B, 0x001E, 0x0014, 0x8011,
		0x8033, 0x0036, 0x003C, 0x8039, 0x0028, 0x802D, 0x8027,
		0x0022, 0x8063, 0x0066, 0x006C, 0x8069, 0x0078, 0x807D,
		0x8077, 0x0072, 0x0050, 0x8055, 0x805F, 0x005A, 0x804B,
		0x004E, 0x0044, 0x8041, 0x80C3, 0x00C6, 0x00CC, 0x80C9,
		0x00D8, 0x80DD, 0x80D7, 0x00D2, 0x00F0, 0x80F5, 0x80FF,
		0x00FA, 0x80EB, 0x00EE, 0x00E4, 0x80E1, 0x00A0, 0x80A5,
		0x80AF, 0x00AA, 0x80BB, 0x00BE, 0x00B4, 0x80B1, 0x8093,
		0x0096, 0x009C, 0x8099, 0x0088, 0x808D, 0x8087, 0x0082,
		0x8183, 0x0186, 0x018C, 0x8189, 0x0198, 0x819D, 0x8197,
		0x0192, 0x01B0, 0x81B5, 0x81BF, 0x01BA, 0x81AB, 0x01AE,
		0x01A4, 0x81A1, 0x01E0, 0x81E5, 0x81EF, 0x01EA, 0x81FB,
		0x01FE, 0x01F4, 0x81F1, 0x81D3, 0x01D6, 0x01DC, 0x81D9,
		0x01C8, 0x81CD, 0x81C7, 0x01C2, 0x0140, 0x8145, 0x814F,
		0x014A, 0x815B, 0x015E, 0x0154, 0x8151, 0x8173, 0x0176,
		0x017C, 0x8179, 0x0168, 0x816D, 0x8167, 0x0162, 0x8123,
		0x0126, 0x012C, 0x8129, 0x0138, 0x813D, 0x8137, 0x0132,
		0x0110, 0x8115, 0x811F, 0x011A, 0x810B, 0x010E, 0x0104,
		0x8101, 0x8303, 0x0306, 0x030C, 0x8309, 0x0318, 0x831D,
		0x8317, 0x0312, 0x0330, 0x8335, 0x833F, 0x033A, 0x832B,
		0x032E, 0x0324, 0x8321, 0x0360, 0x8365, 0x836F, 0x036A,
		0x837B, 0x037E, 0x0374, 0x8371, 0x8353, 0x0356, 0x035C,
		0x8359, 0x0348, 0x834D, 0x8347, 0x0342, 0x03C0, 0x83C5,
		0x83CF, 0x03CA, 0x83DB, 0x03DE, 0x03D4, 0x83D1, 0x83F3,
		0x03F6, 0x03FC, 0x83F9, 0x03E8, 0x83ED, 0x83E7, 0x03E2,
		0x83A3, 0x03A6, 0x03AC, 0x83A9, 0x03B8, 0x83BD, 0x83B7,
		0x03B2, 0x0390, 0x8395, 0x839F, 0x039A, 0x838B, 0x038E,
		0x0384, 0x8381, 0x0280, 0x8285, 0x828F, 0x028A, 0x829B,
		0x029E, 0x0294, 0x8291, 0x82B3, 0x02B6, 0x02BC, 0x82B9,
		0x02A8, 0x82AD, 0x82A7, 0x02A2, 0x82E3, 0x02E6, 0x02EC,
		0x82E9, 0x02F8, 0x82FD, 0x82F7, 0x02F2, 0x02D0, 0x82D5,
		0x82DF, 0x02DA, 0x82CB, 0x02CE, 0x02C4, 0x82C1, 0x8243,
		0x0246, 0x024C, 0x8249, 0x0258, 0x825D, 0x8257, 0x0252,
		0x0270, 0x8275, 0x827F, 0x027A, 0x826B, 0x026E, 0x0264,
		0x8261, 0x0220, 0x8225, 0x822F, 0x022A, 0x823B, 0x023E,
		0x0234, 0x8231, 0x8213, 0x0216, 0x021C, 0x8219, 0x0208,
		0x820D, 0x8207, 0x0202 };
	
	for(j = 0; j < data_blk_size; j++)
	{
		i = ((unsigned short)(crc_accum >> 8) ^ *data_blk_ptr++) & 0xFF;
		crc_accum = (crc_accum << 8) ^ crc_table[i];
	}
	return crc_accum;
}
// MARK: Error
void DynamixelComm::getServoError2(unsigned char errorByte)
{
	// 0x01 Result Fail
	//	- When it has failed to process the sent Instruction Packet
	// 0x02 Instruction Error
	//	- When an undefined Instruction has been used
	//	- When Action has been used without Reg Write
	// 0x03 CRC Error
	//	- When the CRC of the sent Packet does not match
	// 0x04 Data Range Error
	//	- When the Data to be written in the corresponding Address is outside the range of the minimum/maximum value
	// 0x05 Data Length Error
	//	- When you attempt to write Data that is shorter than the data length of the corresponding Address (ex: when you attempt to only use 2 bytes of a item that has been defined as 4 bytes)
	// 0x06 Data Limit Error
	//	- When the Data to be written in the corresponding Address is outside of the Limit value
	// 0x07 Access Error
	//	- When you attempt to write a value in an Address that is Read Only or has not been defined
	//	- When you attempt to read a value in an Address that is Write Only or has not been defined
	//	- When you attempt to write a value in the ROM domain while in a state of Torque Enable(ROM Lock)
	
	
	ErrorServo2 = (errorByte >> 0) & 0x1;
	ErrorServoResaultFail2 = (errorByte >> 1) & 0x1;
	ErrorServoIntruction2 = (errorByte >> 2) & 0x1;
	ErrorServoCrc2 = (errorByte >> 3) & 0x1;
	ErrorServoRange2 = (errorByte >> 4) & 0x1;
	ErrorServoLength2 = (errorByte >> 5) & 0x1;
	ErrorServoLimit2 = (errorByte >> 6) & 0x1;
	ErrorServoAccess2 = (errorByte >> 7) & 0x1;
}
