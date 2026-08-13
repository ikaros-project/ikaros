// serial.h		Serial IO utilities for the IKAROS project

#pragma once

#include <memory>
#include <string>

#include "exceptions.h"


class SerialData;

class Serial
{
public:
    Serial();
    Serial(const Serial &) = delete;
    Serial & operator=(const Serial &) = delete;
    Serial(Serial &&) = delete;
    Serial & operator=(Serial &&) = delete;
    // Configures 8 data bits, no parity, and one stop bit.
    explicit Serial(const std::string & device_name, unsigned long baud_rate);

    ~Serial();

    int SendString(const char * sendbuf);
    int SendBytes(const char * sendbuf, int length);
    int SendBytes(const char * sendbuf, int length, int timeout_ms);

    int ReceiveUntil(char * rcvbuf, int length, char c);
    int ReceiveBytes(char * rcvbuf, int length);

    int ReceiveUntil(char * rcvbuf, int length, char c, int timeout_ms);
    int ReceiveBytes(char * rcvbuf, int length, int timeout_ms);

    void Close();

    void Flush();
    void FlushOut();
    void FlushIn();

protected:
    float time_per_byte = 0.0f;

private:
    std::unique_ptr<SerialData> data;
};
