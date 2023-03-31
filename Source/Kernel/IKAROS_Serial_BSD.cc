//
//	IKAROS_Serial_BSD.cc		Serial IO utilities for the IKAROS project
//
//    Copyright (C) 2006-2018  Christian Balkenius
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


unsigned long baud_constants[][2] =
{
	75, B75,
	110, B110,
	134, B134,
	150, B150,
	200, B200,
	300, B300,
	600, B600,
	1200, B1200,
	1800, B1800,
	2400, B2400,
	4800, B4800,
	9600, B9600,
	19200, B19200,
#ifdef B38400
	38400, B38400,
#endif
#ifdef B57600
	57600, B57600,
#endif
#ifdef B115200
	115200, B115200,
#endif
#ifdef B230400
	230400, B230400,
#endif
#ifdef B460800
	460800, B460800,
#endif
#ifdef B500000
	500000, B500000,
#endif
#ifdef B576000
	576000, B576000,
#endif
#ifdef B921600
	921600, B921600,
#endif
#ifdef B1000000
	1000000, B1000000,
#endif
#ifdef B1152000
	1152000, B1152000,
#endif
#ifdef B1500000
	1500000, B1500000,
#endif
#ifdef B2000000
	2000000, B2000000,
#endif
#ifdef B2500000
	2500000, B2500000,
#endif
#ifdef B3000000
	3000000, B3000000,
#endif
#ifdef B3500000
	3500000, B3500000,
#endif
#ifdef B4000000
	4000000, B4000000,
#endif
	0, 0
};


#ifndef MAC_OS_X
static unsigned long
baud_rate_constant(unsigned long baud_rate)
{
	for(int i=0; baud_constants[i][0] != 0; i++)
		if(baud_rate == baud_constants[i][0])
			return baud_constants[i][1];
	return 0;
}
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
	time_per_byte = 1000.0/baud_rate * 10.0; // Time per bit in ms. 8 bytes of data and 2 bits of start and stop bit.

	data = new SerialData();
	
	data->fd = open(device_name, O_RDWR | O_NOCTTY | O_NDELAY);
	if(data->fd == -1)
		throw SerialException(device_name, "Could not open serial device.   ", errno);
	
	fcntl(data->fd, F_SETFL, 0); // reset flags => blocking
	tcgetattr(data->fd, &options); // get the current options // TODO: restore on destruction of the object
	
#ifndef MAC_OS_X
	if(!(baud_rate = baud_rate_constant(baud_rate)))
		throw SerialException(device_name, "Could not find baud rate constant for suppied baud rate.", errno);
	if(cfsetispeed(&options, baud_rate))
		throw SerialException(device_name, "Could not set baud rate for input", errno);
	if(cfsetospeed(&options, baud_rate))
		throw SerialException(device_name, "Could not set baud rate for output", errno);
#endif
	
	options.c_cflag |= (CS8 | CLOCAL | CREAD);
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	// Non blocking. Handle timeout manualy
	options.c_cc[VMIN]  = 0;
	// options.c_cc[VTIME] = 1;    	// tenth of seconds allowed between bytes of serial data but since VMIN = 0 we will wait at most 1/10 s for data and then return

	tcflush(data->fd, TCIOFLUSH);
	tcsetattr(data->fd, TCSANOW, &options);   // set the options
	
#ifdef MAC_OS_X // TODO: Check this section - was not used for original version
	cfmakeraw(&options); // necessary for ioctl to function; must come after setattr
	const speed_t TGTBAUD = baud_rate;
	int ret = ioctl(data->fd, IOSSIOSPEED, &TGTBAUD); // sets also non-standard baud rates
	if (ret)
		throw SerialException(device_name, "Could not set baud rate", errno);
#endif
	
	// To make sure that the buffers are empty flush them after 500 ms.
	Timer t;
	t.Sleep(500);
	Flush();
}

Serial::~Serial()
{
	Close();
	delete data;
}

void
Serial::Flush()
{
	usleep(5000); //5 ms
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
	usleep(5000); //5 ms. 
	tcflush(data->fd, TCIFLUSH);
}

// Send a string.

int
Serial::SendString(const char *sendbuf)
{
	if(data->fd == -1)
		return 0;
	return int(write(data->fd, sendbuf, strlen(sendbuf)));
}

// Read characters until we receive character c
int
Serial::ReceiveUntil(char *rcvbuf, int length, char c)
{
	return ReceiveUntil(rcvbuf, length, c, 100);
}

int
Serial::ReceiveUntil(char *rcvbuf, int length, char c, int timeout_ms)
{
	if(data->fd == -1)
		return 0;
	
	char *bufptr = rcvbuf;
	long read_bytes = 0, read_bytes_tot = 0;
	
	while (true)
	{
		read_bytes = read(data->fd, bufptr, length); // Was 1 instead of length
		
		if (read_bytes > 0)
		{
			length -= read_bytes; // do not read more than lenht characters - not tested
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
		printf("Serial::SendBytes: Could not send bytes (%d)\n", errno);
	
	return int(n);
}

int
Serial::ReceiveBytes(char *rcvbuf, int length)
{
	return ReceiveBytes(rcvbuf, length, 100);
}

int
Serial::ReceiveBytes(char *rcvbuf, int length, int timeout_ms)
{
	Timer t;
	
	if(data->fd == -1)
		return 0;
	
	char *bufptr = rcvbuf;
	long read_bytes = 0, read_bytes_tot = 0;
	
	while (t.GetTime() <= timeout_ms)
	{
		read_bytes = read(data->fd, bufptr, length - read_bytes_tot);
		
		if (read_bytes < 0)
			break;
		
		bufptr += read_bytes;
		read_bytes_tot += read_bytes;
		
		if (read_bytes_tot >= length)
			break;
		usleep(500); // Sleep 0.5 ms to reduce CPU Load.
	}
	
	//printf("read %i bytes %f ms %i timeoutms\n", length, t.GetTime(), timeout_ms);

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
