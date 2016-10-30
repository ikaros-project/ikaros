//
//    DynamixelComm.cc		Class to communicate with Dynamixel servos
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
#include "DynamixelServo.h"


// MARK: Sync write functions (block memory)
void DynamixelComm::SyncWriteWithIdRange1(int * servo_id, int * mask, unsigned char ** DynamixelMemoeries, int * bindedAdress, int * size, int n)
{
    int datalength = -1;
    int nrServoToSendTo = 0;
    
    // Find parameter size. Same in SyncWrite
    for (int i = 0; i < n; i++)
        if (size[i] != -1)
        {
            datalength = size[i]; // Fixed size for all servoes in sync_write mode.
            break;
        }
    
    if (datalength == -1)
        return;
    
    // Find parameter adress.
    int adress = -1;
    for (int i = 0; i < n; i++)
    {
        if (bindedAdress[i] != -1)
        {
            adress = bindedAdress[i]; // Fixed size for all servoes in sync_write mode.
            break;
        }
    }
    for (int i = 0; i < n; i++)
        if (bindedAdress[i] != -1)
            if (mask[i] == 1)
                nrServoToSendTo++;
    
    if (adress == -1)
        return;
    
    //printf("Number of servos to send to %i\n",nrServoToSendTo);
    
    // Check if the length of the packages is vailed. The maximum buffer size for the dynamixel is 147 bytes.
    // If there is a higher value than this we need to split it into smaller pecies.
    if (datalength*n+SYNC_WRITE_HEADER_1 > DYNAMIXEL_MAX_BUFFER)
    {
        printf("DynamixelCommunication: Message size is over 143 bytes. Please reduce the number of servos of data sent to the servos\n");
        
        //        // Finding out how many servoes we can send to each time.
        //        nrServoToSendTo = ((DYNAMIXEL_MAX_BUFFER-SYNC_WRITE_HEADER_1)/datalength)-2;
        //        int nTo = nrServoToSendTo;
        //        int nFrom = 0;
        //        while (nFrom != n)
        //        {
        //            unsigned char outbuf[256] =
        //            {
        //                0XFF,
        //                0XFF,
        //                0XFE,
        //                static_cast<unsigned char>((datalength+1)*(nTo-nFrom+1)+4),     // (datalength+1)*N+4
        //                0X83,                                                           // sync_write
        //                static_cast<unsigned char>(adress),                               // address
        //                static_cast<unsigned char>(datalength)                          // length
        //            };
        //
        //            int k = 0;
        //            for(int i=nFrom; i<=nTo; i++)
        //            {
        //                outbuf[SYNC_WRITE_HEADER_1+k*(datalength+1)] = servo_id[i];
        //                memcpy(&outbuf[SYNC_WRITE_HEADER_1 + (datalength+1)*k+1], &DynamixelMemoeries[i][adress], (datalength) * sizeof(unsigned char) );
        //                k++;
        //            }
        //            Send1(outbuf);
        //
        //            nFrom = nTo+1; // Next send from
        //            nTo = nTo + nrServoToSendTo; // Next send to
        //            if(nTo+1 > n)
        //                nTo = n-1;
        //        }
    }
    else // No splitting needed.
    {
        unsigned char outbuf[256] =
        {
            0XFF,
            0XFF,
            0XFE,
            static_cast<unsigned char>((datalength+1)*nrServoToSendTo+4),   // (datalength+1)*N+4
            0X83,                                                           // sync_write
            static_cast<unsigned char>(adress),                             // address
            static_cast<unsigned char>(datalength)                          // length
        };
        
        for(int i=0; i<n; i++)
            if (bindedAdress[i] != -1 && mask[i] == 1) // Skipp servo if it can not handle parameter (PID parameters etc) and not masked.
            {
                outbuf[SYNC_WRITE_HEADER_1+i*(datalength+1)] = servo_id[i];
                memcpy(&outbuf[SYNC_WRITE_HEADER_1 + i*(datalength+1)+1], &DynamixelMemoeries[i][adress], datalength * sizeof(unsigned char) );
            }
        //PrintFullInstructionPackage1(outbuf);
        Send1(outbuf);
    }
}
// MARK: Read block
bool
DynamixelComm::ReadMemoryRange1(int id, unsigned char * buffer, int from, int to)
{
    
    int datalength = to-from+1;
    unsigned char outbuf[256] = {0XFF,0XFF, static_cast<unsigned char>(id), 4, INST_READ, static_cast<unsigned char>(from), static_cast<unsigned char>(to), 0X00};
    Send1(outbuf);
    //PrintFullInstructionPackage1(outbuf);
    
    unsigned char inbuf[256];
    int n = Receive1(inbuf);
    //PrintFullStatusPackage1(inbuf);
    
    // Nothing in.
    if(n==0)
    {
        FlushIn();
        FlushOut();
        return false;
    }
    
    // Check checksum
    unsigned char CheckSumAnswer = CalculateChecksum(inbuf);
    if (inbuf[inbuf[3]+3] != CheckSumAnswer)
    {
        FlushIn();
        FlushOut();
        return false;
    }
    
    // Copy recived data to servo memory
    memcpy(&buffer[0], &inbuf[5], datalength * sizeof(unsigned char));
    return true;
}
// MARK: Send/Recive
void
DynamixelComm::Send1(unsigned char * b)
{
    b[b[3]+3] = CalculateChecksum(b);
    SendBytes((char *)b, b[3]+4);
}
int
DynamixelComm::Receive1(unsigned char * b)
{
    int c = ReceiveBytes((char *)b, 4);
    if(c < 4)
        return 0;
    
    c += ReceiveBytes((char *)&b[4], b[3]);
    
    unsigned char checksum = CalculateChecksum(b);
    if(checksum != b[b[3]+3])
        return 0;
    
    return c;
}

// MARK: MISC
bool
DynamixelComm::Ping1(int id)
{
    unsigned char outbuf[256] = {0XFF, 0XFF, static_cast<unsigned char>(id), 2, INST_PING, 0X00};
    unsigned char inbuf[256];
    Send1(outbuf);
    int n = Receive1(inbuf);
    return (n != 0);
}

void
DynamixelComm::Reset1(int id)
{
    printf("Dynamixel: Trying to reset id: %i\n", id);
    unsigned char outbuf[256] = {0XFF, 0XFF, static_cast<unsigned char>(id), 2, INST_RESET, 0X00};
    Send1(outbuf);
    // Recive status packet but do not do anyting with it.
    unsigned char inbuf[256];
    Receive1(inbuf);
}
// MARK: Print
void DynamixelComm::PrintFullInstructionPackage1(unsigned char * package)
{
    PrintPartInstructionPackage1(package, 1, package[3]-2);
}
void DynamixelComm::PrintPartInstructionPackage1(unsigned char * outbuf, int from, int to)
{
    int datalength = to-from+1;                                   // Only a part of the parameters
    int totalLengthOfPackage = 3 + outbuf[3] + 1 ;                // length of package
    
    
    printf("\n====== Instruction Packet (Send) (%i) ========\n",totalLengthOfPackage);
    for(int j=0; j<RECIVE_HEADER_1; j++){
        if (j == 0)
            printf("============= Start Bytes =======\n");
        if (j == 2)
            printf("============= ID ================\n");
        if (j == 3)
            printf("============= Length ============\n");
        if (j == 4)
            printf("============= Inst ==============\n");
        printf("%3i  \t\tBUFFER: \%#04X\t(%3i)\n", j, outbuf[j], outbuf[j]);
    }
    
    printf("*********** Parameters (%i) ******\n",datalength);
    
    for(int j=from-1; j<=to-1; j++)
    {
        if (j == 0)
            printf("============= Data bytes ========\n");
        printf("%3i (%2i) \tBUFFER: %#04X \t(%3i)\n", RECIVE_HEADER_1+j, from+j, outbuf[RECIVE_HEADER_1+j],outbuf[RECIVE_HEADER_1+j]);
    }
    printf("*********************************\n");
    
    printf("============ CRC ================\n");
    printf("%3i \t\tBUFFER: %#04X \t(%3i)\n", totalLengthOfPackage-1, outbuf[totalLengthOfPackage-1],outbuf[totalLengthOfPackage-1]);
    
}
void DynamixelComm::PrintMemory1(unsigned char * m, int from, int to)
{
    printf("\n======= Memory Package  ======\n");
    for(int j=from; j<to; j++)
        printf("%3i \t\tBUFFER: %#04X \t(%3i)\n", j, m[j],m[j]);
}
void DynamixelComm::PrintFullStatusPackage1(unsigned char * package)
{
    PrintPartStatusPackage1(package, 1, package[3]-2);
}
void DynamixelComm::PrintPartStatusPackage1(unsigned char * outbuf, int from, int to)
{
    int datalength = to-from+1;                                   // Only a part of the parameters
    int totalLengthOfPackage = 3 + outbuf[3] + 1;    // length of package
    
    
    printf("\n======= Status Package (%i) ======\n",totalLengthOfPackage);
    for(int j=0; j<RECIVE_HEADER_1; j++){
        if (j == 0)
            printf("============= Start Bytes =======\n");
        if (j == 2)
            printf("============= ID ================\n");
        if (j == 3)
            printf("============= Length ============\n");
        if (j == 4)
            printf("============= Error ==============\n");
        printf("%3i  \t\tBUFFER: \%#04X\t(%3i)\n", j, outbuf[j], outbuf[j]);
    }
    
    printf("*********** Parameters (%i) ******\n",datalength);
    
    for(int j=from-1; j<=to-1; j++)
    {
        if (j == 0)
            printf("============= Data bytes ========\n");
        printf("%3i (%2i) \tBUFFER: %#04X \t(%3i)\n", RECIVE_HEADER_1+j, from+j-1, outbuf[RECIVE_HEADER_1+j],outbuf[RECIVE_HEADER_1+j]);
    }
    printf("*********************************\n");
    
    printf("============ CRC ================\n");
    printf("%3i \t\tBUFFER: %#04X \t(%3i)\n", totalLengthOfPackage-1, outbuf[totalLengthOfPackage-1],outbuf[totalLengthOfPackage-1]);
}

// MARK: CRC
unsigned char
DynamixelComm::CalculateChecksum(unsigned char * b)
{
    unsigned char checksum = 0;
    for(int i=0; i<(b[3]+1); i++)
        checksum += b[i+2];
    checksum = ~checksum;
    
    return checksum;
}
