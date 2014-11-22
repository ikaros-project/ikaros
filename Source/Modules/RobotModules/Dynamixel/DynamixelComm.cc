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

// MINIMIZED_SEND: Store a copy of the data sent to servo. This is later used to reduce the amout of sent data. When a new command is recived it is compared with the previous command. if these are identical we asume that the servo is up to date. If we send a block of data the block is compared to the stored data to allow minimal send to be made. If we transimitt a [10 10 10 10 10] and we have stored [10 10 3 10 4] from previous sends this send will only be the tree last elements of the data [3 10 4]. This is useful when communicating a lot with the servos but only with none or a few changes. It might also produce a smoother behaviour as the servo seem to be more jerky in the movement (my opinien) when frequently sending new position. I might have something to do with the way dynamixel is using the position regulation.
//#define MINIMIZED_SEND


#define SYNC_WRITE_HEADER 7
#define RECIVE_HEADER 5
#define MAX_SERVOS 256
#define DYNAMIXEL_MAX_BUFFER 143

DynamixelComm::DynamixelComm(const char * serial_device, unsigned long baud_rate):
Serial(serial_device, baud_rate)
{
    max_failed_reads = 1;
    
#ifdef MINIMIZED_SEND
    sentStore = new unsigned char * [MAX_SERVOS];
#endif
    
}

DynamixelComm::~DynamixelComm()
{
#ifdef MINIMIZED_SEND
    // free memory
    for(int i=0; i<MAX_SERVOS; i++)
    {
        if (sentStore[i] != NULL)
            delete sentStore[i];
    }
    delete sentStore;
#endif
    
}

// MARK: Sync write functions (block memory)

void DynamixelComm::SyncWriteWithIdRange(int * servo_id,  unsigned char ** DynamixelMemoeries, int from, int to, int n)
{
    
    //printf("\nSyncWriteWithIdRange what to send (%i) %i - %i\n", n, from,to);
    
#ifdef MINIMIZED_SEND
    
    bool sentStoreComplete = true;
    
    // first we need to check if we have any previous send stored for the IDs about to be sent.
    for(int i=0; i<n; i++)
    {
        if (sentStore[i] == NULL)
            sentStoreComplete = false;
    }
    
    
    if (sentStoreComplete)
    {
        int newFrom = 9999;
        int newTo = -10;
        
        // Loop to find first byte that differ
        for(int i=0; i<n; i++)
        {
            int j = from;
            while(j != to+1 && j < newFrom)
                if (DynamixelMemoeries[i][j] == sentStore[i][j])
                    j++;
                else
                    newFrom = j;
        }
        // The memoryblocks are identical. No need to send it again or to store it in sentStore
        if (newFrom == 9999)
        {
            //printf("\n * Identical *\n");
            return;
        }
        else
        {
            // Loop to find last byte that differ
            for(int i=0; i<n; i++)
            {
                int j = to;
                while(j != from-1 && j > newTo)
                    if (DynamixelMemoeries[i][j] == sentStore[i][j])
                        j--;
                    else
                        newTo = j;
                
            }
        }
        
        //printf("%i -> %i and %i -> %i\n", from, newFrom,  to, newTo);
        to = newTo;
        from = newFrom;
    }
    
    // Store sent memory to sentStore
    for(int i=0; i<n; i++)
    {
        if (sentStore[i] == NULL)
            sentStore[i] = new unsigned char[128];
        
        //PrintMemory(DynamixelMemoeries[i], 24, 35);
        //printf("\nStoring last sent (%i) %i - %i (%i)\n", i, from,to,to-from+1);
        
        // Copy the chnges values
        std::memcpy(&sentStore[i][from], &DynamixelMemoeries[i][from], (to-from+1)* sizeof(unsigned char));
        
        //PrintMemory(sentStore[i], 24, 35);
    }
    
#endif
    
    int datalength = to-from+1;
    //printf("Total Length = %i\n", datalength*n);
    
    // Check if the length of the packages is vailed. The maximum buffer size for the dynamixel is 147 bytes. If there is a higher value than this we need to split it into smaller pecies.
    if (datalength*n+SYNC_WRITE_HEADER > DYNAMIXEL_MAX_BUFFER)
    {
        //printf("The message is too long and needs to be splitted up.\n");
        
        // Finding out how many servoes we can send to each time.
        int nrServoToSendTo = ((DYNAMIXEL_MAX_BUFFER-SYNC_WRITE_HEADER)/datalength)-2;
        int nTo = nrServoToSendTo;
        int nFrom = 0;
        while (nFrom != n)
        {
            //printf("Sending %i to %i\n", nFrom+1, nTo+1);
            
            unsigned char outbuf[256] =
            {
                0XFF,
                0XFF,
                0XFE,
                static_cast<unsigned char>((datalength+1)*(nTo-nFrom+1)+4),     // (datalength+1)*N+4
                0X83,                                               // sync_write
                0X18,                                               // address
                static_cast<unsigned char>(datalength)              // length
            };
            
            int k = 0;
            for(int i=nFrom; i<=nTo; i++)
            {
                outbuf[SYNC_WRITE_HEADER+k*(datalength+1)] = servo_id[i];
                memcpy(&outbuf[SYNC_WRITE_HEADER + (datalength+1)*k+1], &DynamixelMemoeries[i][from], (datalength) * sizeof(unsigned char) );
                k++;
            }
            
            //PrintSyncWrite(outbuf, from, to, nTo-nFrom+1);
            //printf("SEND\n");
            Send(outbuf);
            
            nFrom = nTo+1; // Next send from
            nTo = nTo + nrServoToSendTo; // Next send to
            if(nTo+1 > n)
            nTo = n-1;
        }
    }
    else
    {
        unsigned char outbuf[256] =
        {
            0XFF,
            0XFF,
            0XFE,
            static_cast<unsigned char>((datalength+1)*n+4),     // (datalength+1)*N+4
            0X83,                                               // sync_write
            static_cast<unsigned char>(from),                   // address
            static_cast<unsigned char>(datalength)              // length
        };
        
        for(int i=0; i<n; i++)
        {
            outbuf[SYNC_WRITE_HEADER+i*(datalength+1)] = servo_id[i];
            memcpy(&outbuf[SYNC_WRITE_HEADER + i*(datalength+1)+1], &DynamixelMemoeries[i][from], datalength * sizeof(unsigned char) );
            //PrintMemory(DynamixelMemoeries[i], 0, 70);
        }
        //PrintSyncWrite(outbuf,from,to, n);
        Send(outbuf);
    }
}


// MARK: Read functions

bool
DynamixelComm::ReadMemoryRange(int id, unsigned char * buffer, int from, int to)
{
    int datalength = to-from+1;
    
    unsigned char outbuf[256] =
    {
        0XFF,
        0XFF,
        static_cast<unsigned char>(id),
        4,
        INST_READ,
        static_cast<unsigned char>(from),
        static_cast<unsigned char>(to),
        0X00
    };
    Send(outbuf);
    
    
    unsigned char inbuf[256];
    int n = Receive(inbuf);
    
    //PrintRecive(inbuf, from, to);
    
    if(n==0)
    {
        //printf("Error\n\n");
        // Flusing serial
        FlushIn();
        FlushOut();
        return false;
    }
    // Check message
    
    // Check right ID
    if (id != inbuf[2])
    {
        //printf("Error: Reading Dynamixel ID\n If you get a lot of error check for multiple servos with the same ID\n\n");
        FlushIn();
        FlushOut();
        return false;
    }
    // Check length
    if (datalength != inbuf[3]-1)
    {
        //printf("Error: Reading Dynamixel Length\n If you get a lot of error check for multiple servos with the same ID\n\n");
        FlushIn();
        FlushOut();
        return false;
    }
    // Check error
    if (0 != inbuf[4])
    {
        //printf("Error: Reading Dynamixel Errors\n If you get a lot of error check for multiple servos with the same ID\n\n");
        FlushIn();
        FlushOut();
        return false;
    }
    
    // Check checksum
    unsigned char CheckSumAnswer = CalculateChecksum(inbuf);
    if (inbuf[inbuf[3]+3] != CheckSumAnswer)
    {
        //printf("Error: Reading Dynamixel checksum\n If you get a lot of error check for multiple servos with the same ID\n\n");
        FlushIn();
        FlushOut();
        return false;
    }
    
    // Copy data to buffer
    memcpy(&buffer[0], &inbuf[5], datalength * sizeof(unsigned char));
    
    return true;
}

// MARK: Misc functions
void DynamixelComm::PrintSyncWrite(unsigned char * outbuf, int from, int to, int n)
{
    int datalength = to-from+1;
    
    //printf("n = %i\n", n);
    //printf("datalength = %i\n", datalength);
    
    printf("\n\n============SYNC WRITE===========\n");
    printf("=================================\n");
    printf("==============HEADER=============\n");
    
    for(int j=0; j<SYNC_WRITE_HEADER; j++)
        printf("%3i  \t\tBUFFER: \%#04X\t(%3i)\n", j, outbuf[j], outbuf[j]);
    printf("=================================\n");
    
    for(int i=0; i<n; i++)
        for(int j=0; j<=datalength; j++)
        {
            if (j == 0)
            {
                printf("=============ID Byte=============\n");
                printf("%3i  \t\tBUFFER: %#04X \t(%3i)\n", SYNC_WRITE_HEADER+j+i*(datalength+1), outbuf[SYNC_WRITE_HEADER+j+i*(datalength+1)],outbuf[SYNC_WRITE_HEADER+j+i*(datalength+1)]);
            }
            else
            {
                printf("%3i (%2i) \tBUFFER: %#04X \t(%3i)\n", SYNC_WRITE_HEADER+j+i*(datalength+1), from+j-1, outbuf[SYNC_WRITE_HEADER+j+i*(datalength+1)],outbuf[SYNC_WRITE_HEADER+j+i*(datalength+1)]);
            }
            if (j == 0)
                printf("=================================\n");
        }
    printf("\n");
}
void DynamixelComm::PrintMemory(unsigned char * outbuf, int from, int to)
{
    int datalength = to-from+1;
    
    //printf("n = %i\n", n);
    printf("datalength = %i\n", datalength);
    
    
    
    printf("===============ID %i==============\n", outbuf[3]);
    
    
    //for(int i=0; i<n; i++)
    for(int j=from; j<=to; j++)
    {
        printf("%3s (%2i) \tBUFFER: %#04X \t(%3i)\n","", j, outbuf[j],outbuf[j]);
    }
    printf("=================================\n");
    
    printf("\n");
}

void DynamixelComm::PrintRecive(unsigned char * outbuf, int from, int to)
{
    int datalength = to-from+1;
    
    //printf("\nDatalength = %i\n", datalength);
    
    printf("==============RECIVE=============\n");
    printf("=================================\n");
    printf("==============HEADER=============\n");
    
    for(int j=0; j<RECIVE_HEADER; j++)
        printf("%3i  \t\tBUFFER: \%#04X\t(%3i)\n", j, outbuf[j], outbuf[j]);
    printf("=================================\n");
    
    for(int j=0; j<=datalength; j++)
    {
        if (j == 3)
        {
            printf("=============ID Byte=============\n");
            printf("%3i  \t\tBUFFER: %#04X \t(%3i)\n", RECIVE_HEADER+j, outbuf[RECIVE_HEADER+j],outbuf[RECIVE_HEADER+j]);
        }
        else
            printf("%3i (%2i) \tBUFFER: %#04X \t(%3i)\n", RECIVE_HEADER+j, from+j, outbuf[RECIVE_HEADER+j],outbuf[RECIVE_HEADER+j]);
        
        if (j == 3)
            printf("=================================\n");
        
    }}

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
        //printf("receive error (data size = %d) incorrect checksum\n", c);
        return 0;
    }
    
    return c;
}


// Ping a dynamixel to see if it exists
bool
DynamixelComm::Ping(int id)
{
    unsigned char outbuf[256] = {0XFF, 0XFF, static_cast<unsigned char>(id), 2, INST_PING, 0X00};
    unsigned char inbuf[256];
    
    Send(outbuf);
    int n = Receive(inbuf);
    
    return (n != 0);
}

void
DynamixelComm::ResetDynamixel(int id)
{
    
    printf("Trying to reset %i\n", id);
    unsigned char outbuf[256] =
    {
        0XFF,
        0XFF,
        static_cast<unsigned char>(id),
        2,
        INST_RESET,
        0X00
    };
    Send(outbuf);
    
    // Recive status packet but do not do anyting with it. Does this work if the baudrate change?
    unsigned char inbuf[256];
    Receive(inbuf);
}

