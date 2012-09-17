//
//	IKAROS_Serial_BSD.cc		Serial IO utilities for the IKAROS project
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

#include <cstdio>
#include <cstring>
#include <cerrno>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>


#ifdef MAC_OS_X
#include <IOKit/serial/ioss.h>
#include <sys/ioctl.h>
#endif



//
//	SerialData (for Unix this is simply the file descriptor)
//

class SerialData
{
public:
    int		fd;
    
    SerialData() : fd(-1)
    {};
};



Serial::Serial(const char * device_name, unsigned long baud_rate)
{
    struct termios options;
    
    max_failed_reads = DEFAULT_MAX_FAILED_READS;
    
    data = new SerialData();

    data->fd = open(device_name, O_RDWR | O_NOCTTY | O_NDELAY);
    if(data->fd == -1)
        throw SerialException("Could not open serial device.   ", errno);
    
    fcntl(data->fd, F_SETFL, 0); // blocking
    tcgetattr(data->fd, &options); // get the current options // TODO: restore on destruction of the object

#ifndef MAC_OS_X
    if(cfsetispeed(&options, baud_rate))
		throw SerialException("Could not set baud rate for input", errno);
    if(cfsetospeed(&options, baud_rate))
		throw SerialException("Could not set baud rate for output", errno);
#endif

    options.c_cflag |= (CS8 | CLOCAL | CREAD);
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    options.c_cc[VMIN]  = 0;
    options.c_cc[VTIME] = 1;    // tenth of seconds allowed between bytes of serial data
                                // but since VMIN = 0 we will wait at most 1/10 s for data then return
    tcflush(data->fd, TCIOFLUSH);
    tcsetattr(data->fd, TCSANOW, &options);   // set the options

#ifdef MAC_OS_X // TODO: Check this section - was not used for original version
    cfmakeraw(&options); // necessary for ioctl to function; must come after setattr
	const speed_t TGTBAUD = baud_rate;
	int ret = ioctl(data->fd, IOSSIOSPEED, &TGTBAUD); // sets also non-standard baud rates
	if (ret)
		throw SerialException("Could not set baud rate", errno);
#endif
}



Serial::~Serial()
{
    Close();
    delete data;
}



void
Serial::Flush()
{
    tcflush(data->fd, TCIOFLUSH);
}



void
Serial::FlushOut()
{
    tcflush(data->fd, TCOFLUSH);
}



void
Serial::FlushIn()
{
    tcflush(data->fd, TCOFLUSH);
}



// Send a string.

int
Serial::SendString(const char *sendbuf)
{
    if(data->fd == -1)
        return 0;
    return int(write(data->fd, sendbuf, strlen(sendbuf)));
}



// Read characters until we recive character c

int
Serial::ReceiveUntil(char *rcvbuf, char c)
{
    if(data->fd == -1)
        return 0;

    char *bufptr = rcvbuf;
    long read_bytes = 0, read_bytes_tot = 0, failed_reads = 0;

    while (failed_reads < max_failed_reads)
    {
        read_bytes = read(data->fd, bufptr, 1);

        if (read_bytes == 0)
        {
            failed_reads++;
            continue;
        }

        if (read_bytes > 0)
        {
            bufptr += read_bytes;
            read_bytes_tot += read_bytes;
        }

        if (read_bytes < 0)
            break;

        if (bufptr[-1] == c)
            break;
    }

    if (read_bytes_tot == 0)
        return int(read_bytes);
    else
        return int(read_bytes_tot);
}



int
Serial::SendBytes(const char *sendbuf, int length)
{
    if(data->fd == -1)
        return 0;

    long n = write(data->fd, sendbuf, length);
    
    if(n == -1)
        printf("Could not send bytes (%d)", errno);
    
    return int(n);
}



int
Serial::ReceiveBytes(char *rcvbuf, int length)
{
    if(data->fd == -1)
        return 0;

    char *bufptr = rcvbuf;
    long read_bytes = 0, read_bytes_tot = 0, failed_reads = 0;

    while (failed_reads < max_failed_reads)
    {
        read_bytes = read(data->fd, bufptr, length - read_bytes_tot);

        if (read_bytes == 0)
        {
            failed_reads++;
            continue;
        }

        if (read_bytes < 0)
            break;

        bufptr += read_bytes;
        read_bytes_tot += read_bytes;

        if (read_bytes_tot >= length)
            break;
    }

    if (read_bytes_tot == 0)
        return int(read_bytes);
    else
        return int(read_bytes_tot);
}



void Serial::Close()
{
    Flush();
    if(data->fd != -1)
        close(data->fd);
}

