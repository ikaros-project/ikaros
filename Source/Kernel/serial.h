//
//	IKAROS_Serial.h		Serial IO utilities for the IKAROS project
//
//    Copyright (C) 2006-2020  Christian Balkenius
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
//    Edited by Birger

#ifndef IKAROS_SERIAL
#define IKAROS_SERIAL


#define DEFAULT_MAX_FAILED_READS 100

//#include "ikaros.h"
//using namespace ikaros;

#include <string>
#include <cstdlib>
#include <cstdio>

#include "exceptions.h"
#include "timing.h"


/*
class SerialException
{
public:
    const char *    device;
	const char *    string;
	int             internal_reference;

	SerialException(const char * d, const char * s, int ref = 0) : device(d), string(s), internal_reference(ref) {};
};
*/


class SerialData;

class Serial
{
public:

    Serial();
    //Serial(const char * device, unsigned long baud_rate);   // defaults to 8 bits, no partity, 1 stop bit
    Serial(std::string device_name, unsigned long baud_rate);

    ~Serial();

	int SendString(const char *sendbuf);
	int SendBytes(const char *sendbuf, int length);

	int ReceiveUntil(char *rcvbuf, int length, char c);
	int ReceiveBytes(char *rcvbuf, int length);

	int ReceiveUntil(char *rcvbuf, int length, char c, int timeout); // Timeout in ms.
	int ReceiveBytes(char *rcvbuf, int length, int timeout);

    void Close();

    void Flush();
    void FlushOut();
    void FlushIn();

protected:
    int             max_failed_reads;
	float 			time_per_byte;
private:
    SerialData *    data;
};


#endif


