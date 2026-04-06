// serial.h		Serial IO utilities for the IKAROS project

#pragma once

inline constexpr int DEFAULT_MAX_FAILED_READS = 100;

#include <string>
#include <cstdlib>
#include <cstdio>

#include "exceptions.h"
#include "timing.h"


class SerialData;

class Serial
{
public:
    Serial() = default;
    Serial(const Serial &) = delete;
    Serial & operator=(const Serial &) = delete;
    Serial(Serial &&) = delete;
    Serial & operator=(Serial &&) = delete;
    explicit Serial(const std::string & device_name, unsigned long baud_rate); // defaults to 8 bits, no partity, 1 stop bit

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
    int             max_failed_reads = DEFAULT_MAX_FAILED_READS;
	float 			time_per_byte = 0.0f;
private:
    SerialData *    data = nullptr;
};
