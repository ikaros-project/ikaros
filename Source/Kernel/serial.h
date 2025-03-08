// serial.h		Serial IO utilities for the IKAROS project

#pragma once

#define DEFAULT_MAX_FAILED_READS 100

#include <string>
#include <cstdlib>
#include <cstdio>

#include "exceptions.h"
#include "timing.h"


class SerialData;

class Serial
{
public:

    Serial();
    Serial(std::string device_name, unsigned long baud_rate); // defaults to 8 bits, no partity, 1 stop bit

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

