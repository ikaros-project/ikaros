//
//	IKAROS_Serial_WIN.cc		Serial IO utilities for the IKAROS project
//
//    Copyright (C) 2006-2010  Christian Balkenius
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


#include "IKAROS_Serial.h"

#ifdef USE_WIN_SERIAL

//#define SHOWCOM

#include <windows.h>

//for debuging
#include <iostream>
#include <bitset>

//
//	SerialData (the state variables needed for the connection)
//

class SerialData
{
public:
    char* Port;
    char* Comport;
    HANDLE hCom;
    
    SerialData() {};
    ~SerialData() {}; // FIXME: delete Port; delete Comport;
};




Serial::Serial(const char * device_name, unsigned long baud_rate)
{
    data = new SerialData();

#ifdef SHOWCOM
    data->Comport = new char[strlen(device_name)+1];
    strcpy(data->Comport, device_name);
#endif

#ifdef SHOWCOM
    printf("(IKAROS_Serial_Win.cc)(%s) Trying to open port", data->Comport);
#endif

    data->hCom = CreateFile(device_name,
                      GENERIC_READ | GENERIC_WRITE,
                      0, // must be opened with exclusive-access
                      NULL, // no security attributes
                      OPEN_EXISTING, // must use OPEN_EXISTING
                      0,    // not overlapped I/O
                      NULL  // hTemplate must be NULL for comm devices
                     );

    if (data->hCom == INVALID_HANDLE_VALUE)
    {
        printf("...Faild to create serialport\n");
        printf("Ikaros is shuting down....\n");
        exit(1);
    }
#ifdef SHOWCOM
    printf("...Succesfully (maybe)\n");
#endif

    DCB dcb;

#ifdef SHOWCOM
    printf("(IKAROS_Serial_Win.cc)(%s) Getting parameters", data->Comport);
#endif

    if (!GetCommState(data->hCom, &dcb))
    {
        printf("...Faild to get prameters\n");
        printf("Ikaros is shutdown....\n");
        exit(1);
    }

#ifdef SHOWCOM
    printf("...Succesfully\n");
#endif

// Settings Hardcoded. Baudrate needs to have CBR_ before the baudrate. Differs form linux
// Lunux
//    dcb.BaudRate = BaudRate; // set the baud rate
//    dcb.ByteSize = ByteSize; // data size, xmit, and rcv
//    dcb.Parity = Parity;  // no parity bit
//    dcb.StopBits = StopBits; // one stop bit

// Windows
    dcb.BaudRate = baud_rate;     // set the baud rate, for example CBR_57600 for windows
    dcb.ByteSize = 8;             // data size, xmit, and rcv
    dcb.Parity = NOPARITY;        // no parity bit
    dcb.StopBits = ONESTOPBIT;    // one stop bit

#ifdef SHOWCOM
    printf("(IKAROS_Serial_Win.cc)(%s) Setting parameters", data->Comport);
    printf("%i,%i,%i,%i", baud_rate, ByteSize, Parity, StopBits);
#endif

    if (!SetCommState(data->hCom, &dcb))
    {
        printf("...Faild to set prameters\n");
        printf("Ikaros is shutdown....\n");
        exit(1);
    }

#ifdef SHOWCOM
    printf("...Succesfully\n");
#endif

#ifdef SHOWCOM
    printf("(IKAROS_Serial_Win.cc)(%s) Setting Timeouts", data->Comport);
#endif
    //// Creating Timeouts for Serial communication
    COMMTIMEOUTS timeouts;
    timeouts.ReadIntervalTimeout = 400;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.ReadTotalTimeoutConstant = 400;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 400;

    if (!SetCommTimeouts(data->hCom, &timeouts))
    {
        printf("...Faild to set Timeouts\n");
        printf("Ikaros is shuting down....\n");
        exit(1);
    }
#ifdef SHOWCOM
    printf("...Succesfully\n");
#endif
}



Serial::~Serial()
{
    delete data;
}



void
Serial::Flush()
{
    // printf("%i",tcflush(fd, TCIOFLUSH));
    FlushFileBuffers(data->hCom);
}



void
Serial::FlushOut()
{
    //printf("%i",tcflush(fd, TCIOFLUSH));
    // tcflush(fd, TCOFLUSH);
}



void
Serial::FlushIn()
{
    // printf("%i",tcflush(fd, TCIOFLUSH));
    // tcflush(fd, TCOFLUSH);
}



int
Serial::SendString(const char * sendbuf)
{
    //flush();
    unsigned long writed = 0;

    if (!WriteFile(data->hCom, sendbuf, strlen(sendbuf), &writed, 0))
    {
        printf("...Faild to set send!\n");
        return(0);
    }

#ifdef SHOWCOM
    printf("Send (String)\t");
    for (int i = 0; i < strlen(sendbuf); i++)
        printf("%c",sendbuf[i]);
    printf("\n(%i of total %i bytes sent)\n", writed,strlen(sendbuf));
    printf("=============================\n");
#endif
    if (writed < 0)
        return(0);
    else
        return(1);
}



// Read characters until we recive a '\n'
int Serial::ReceiveUntil(char * buffer, char c)
{
    char * bufptr = buffer;
    int nbytes = 0;
    int counter = 0;
    bool start_recive = false;

    unsigned long readed = 0;

    //while ((!ReadFile(hCom, buffer, 1, &readed, 0)) || start_recive == false)
    while ((nbytes = ReadFile(data->hCom, bufptr, 1, &readed, 0)))
    {
        //printf("in recieve string loop %i",bufptr);
        //printf("nbyte = %i and readed = %i\t read= %c\n",nbytes,readed, buffer[0]);
        bufptr += readed;
        if (nbytes > 0)
            start_recive = true;
        nbytes = 0;
        counter++;
        if (bufptr[-1] == c)
            break;

        // Loop 50 times and then return with a error
//        if (counter > 50 && start_recive == false)
//        {
//            // No resonse from the e-puck. Wait for 0.5 sec and then flush the buffer.
//            Timer::Sleep(200);
//            Flush();
//            return (-2);
//        }
    }

#ifdef SHOWCOM
    printf("Recive (String)\t");
    for (int i = 0; i < strlen(buffer); i++)
        printf("%c",buffer[i]);
    printf("\n(%i bytes recived)\n", strlen(buffer));
    printf("==================\n");
#endif
    return(strlen(buffer));
}




int
Serial::SendBytes(const char  *Command, int length)
{
    unsigned long writed = 0;
    int n = WriteFile(data->hCom, Command, length, &writed, 0);
#ifdef SHOWCOM
    printf("Send (Binary)\t\t");
    for (int i = 0; i < length; i++)
        printf(" %x",Command[i]);
    printf("\n(%i of %i total bytes sent)\n", n, length );
    printf("=========================\n");
#endif

    if (n < 0)
        return(0);
    else
        return(1);
}


int
Serial::ReceiveBytes(char *rcvbuf, int length)
{
    char * bufptr = rcvbuf;
    int nbytes = 0;
    int bytes_left = length;
    int sum_bytes = 0;
    int counter = 0;
    bool start_recive = false;

    unsigned long readed = 0;

    //FlushFileBuffers(hCom);

    if (!ReadFile(data->hCom, rcvbuf, length, &readed, 0))
    {
        printf("...Faild to set send!\n");
        return(false);
    }
    else
    {
        if ((int)readed != length)
        {
#ifdef SHOWCOM
            printf("...Timedouted!\n");
#endif

            printf("...Timedouted!\n");
            return(false);
        }
        else
        {
#ifdef SHOWCOM
            for (int i = 0; i < length; i++)
                printf("%x",buffer[i]);
            printf("\n");
#endif
        return(readed);
        }
    }
//////////////////////////////////

//    while ((nbytes = ReadFile(hCom, buffer, length, &readed, 0)) > 0 || start_recive == false)
//    {
//
//        sum_bytes += nbytes;
//        if (nbytes == bytes_left)
//            break;
//        bufptr += nbytes;
//        bytes_left -= nbytes;
//        if (nbytes > 0)
//            start_recive = true;
//        nbytes = 0;
//        counter++;
//        // Loop 10 times and then return with a error
//        if (counter > 10 && start_recive == false)
//        {
//            // No resonse from the e-puck. Wait for 0.5 sec and then flush the buffer.
//            Timer::Sleep(50);
//            Flush();
//            return (-2);
//        }
//    }
//    if (sum_bytes != length)
//        return (-1);

#ifdef SHOWCOM
    printf("Recived (Binary)\t\t");
    for (int i = 0; i < length; i++)
        printf(" %c",buffer[i]);
    printf("\n(%i bytes recived)\n", sum_bytes);
    printf("===================\n");
#endif
    return sum_bytes;
}



void Serial::Close()
{
}

#endif
