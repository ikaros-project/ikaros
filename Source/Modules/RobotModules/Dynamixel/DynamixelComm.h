//
//    DynamixelComm.h
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
//    Created: March, 2016
//


#ifndef DYNAMIXELCOMM
#define DYNAMIXELCOMM

#include "IKAROS.h"

// Protocol version 1 instruction package
#define ID_BYTE_1                   2
#define LEN_BYTE_1                  3
#define INST_BYTE_1                 4

// Protocol version 2 instruction package
#define ID_BYTE_2                   4
#define LEN_L_BYTE_2                5
#define LEN_H_BYTE_2                6
#define INST_BYTE_2                 7
#define ERROR_BYTE_2                8

// Common (Protocol version 2 instructions)
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

#define SYNC_WRITE_HEADER_1 7
#define RECIVE_HEADER_1 5

#define SYNC_WRITE_HEADER_2 8
#define SYNC_WRITE_COMMON_2 4
#define RECIVE_HEADER_2 8

#define BULK_WRITE_HEADER_2 8

#define MAX_SERVOS 256
#define DYNAMIXEL_MAX_BUFFER 143


class DynamixelComm : public Serial
{
public:
    
    DynamixelComm(const char * device_name, unsigned long baud_rate);
    ~DynamixelComm();
    int             Ping(int id);
    void            Reset(int id, int protocol);
    void            WriteToServo(int * servo_id, int * mask, int protocol, unsigned  char ** DynamixelMemoeries, int * ikarosInBind, int * size, int n);
    bool            ReadMemoryRange(int id, int protocol, unsigned char * buffer, int fromAddress, int toAddress);
  
    // Protocol version 1
    void            Send1(unsigned char * b);
    int             Receive1(unsigned char * b);
    void            Reset1(int id);
    bool            Ping1(int id);
    void            PrintFullInstructionPackage1(unsigned char * package);
    void            PrintPartInstructionPackage1(unsigned char * package, int from, int to);
    void            PrintFullStatusPackage1(unsigned char * package);
    void            PrintPartStatusPackage1(unsigned char * package, int from, int to);
    void            PrintMemory1(unsigned char * outbuf, int from, int to);
    //void            ResetDynamixel1(int id);
    bool            ReadMemoryRange1(int id, unsigned char * buffer, int fromAddress, int toAddress);
    void            SyncWriteWithIdRange1(int * servo_id, int * mask, unsigned  char ** DynamixelMemoeries, int * ikarosInBind, int * size, int n);

    unsigned char   CalculateChecksum(unsigned char * b);
    
    // Protocol version 2
    void            Send2(unsigned char * b);
    int             Receive2(unsigned char * b);
    void            Reset2(int id);
    bool            Ping2(int id);
    void            PrintFullInstructionPackage2(unsigned char * package);
    void            PrintPartInstructionPackage2(unsigned char * package, int from, int to);
    void            PrintFullStatusPackage2(unsigned char * package);
    void            PrintPartStatusPackage2(unsigned char * package, int from, int to);
    void            PrintMemory2(unsigned char * outbuf, int from, int to);
    //void            ResetDynamixel2(int id);
    bool            ReadMemoryRange2(int id, unsigned char * buffer, int fromAddress, int toAddress);
    //void            SyncWriteWithIdRange2(int * servo_id, unsigned  char ** DynamixelMemoeries, int * ikarosInBind, int from, int *to, int n);
    void            BulkWrite2(int * servo_id, int * mask, unsigned char ** DynamixelMemoeries, int * ikarosInBind, int * size, int n);
    unsigned short  update_crc(unsigned short crc_accum, unsigned char *data_blk_ptr, unsigned short data_blk_size);
    bool            checkCrc(unsigned char * package);
};

#endif

