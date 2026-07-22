//
//  IKAROS_Serial_BSD.cc        Serial I/O utilities for the IKAROS project
//
//  Copyright (C) 2006-2018 Christian Balkenius
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//

#include "serial.h"

#include <cerrno>
#include <chrono>
#include <climits>
#include <cstring>
#include <limits>
#include <memory>
#include <string>

#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

#ifdef MAC_OS_X
#include <IOKit/serial/ioss.h>
#include <sys/ioctl.h>
#endif


namespace
{
    struct BaudConstant
    {
        unsigned long baud_rate;
        speed_t constant;
    };


    constexpr BaudConstant baud_constants[] =
    {
        {75, B75},
        {110, B110},
        {134, B134},
        {150, B150},
        {200, B200},
        {300, B300},
        {600, B600},
        {1200, B1200},
        {1800, B1800},
        {2400, B2400},
        {4800, B4800},
        {9600, B9600},
        {19200, B19200},
#ifdef B38400
        {38400, B38400},
#endif
#ifdef B57600
        {57600, B57600},
#endif
#ifdef B115200
        {115200, B115200},
#endif
#ifdef B230400
        {230400, B230400},
#endif
#ifdef B460800
        {460800, B460800},
#endif
#ifdef B500000
        {500000, B500000},
#endif
#ifdef B576000
        {576000, B576000},
#endif
#ifdef B921600
        {921600, B921600},
#endif
#ifdef B1000000
        {1000000, B1000000},
#endif
#ifdef B1152000
        {1152000, B1152000},
#endif
#ifdef B1500000
        {1500000, B1500000},
#endif
#ifdef B2000000
        {2000000, B2000000},
#endif
#ifdef B2500000
        {2500000, B2500000},
#endif
#ifdef B3000000
        {3000000, B3000000},
#endif
#ifdef B3500000
        {3500000, B3500000},
#endif
#ifdef B4000000
        {4000000, B4000000},
#endif
    };


    bool
    baud_rate_constant(unsigned long baud_rate, speed_t & constant)
    {
        for(const auto & entry : baud_constants)
            if(entry.baud_rate == baud_rate)
            {
                constant = entry.constant;
                return true;
            }
        return false;
    }


    std::string
    system_error_message(const std::string & device_name, const std::string & operation,
                         int error_number)
    {
        return device_name + ": " + operation + ": " + std::strerror(error_number);
    }


    [[noreturn]] void
    throw_system_error(const std::string & device_name, const std::string & operation)
    {
        const int error_number = errno;
        throw ikaros::serial_error(
            system_error_message(device_name, operation, error_number));
    }


    using SteadyClock = std::chrono::steady_clock;


    int
    remaining_milliseconds(const SteadyClock::time_point & deadline)
    {
        const auto now = SteadyClock::now();
        if(now >= deadline)
            return 0;

        const auto remaining = deadline - now;
        const auto milliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(remaining);
        if(milliseconds.count() >= INT_MAX)
            return INT_MAX;
        return static_cast<int>(milliseconds.count()) + 1;
    }


    int
    wait_for_event(int fd, short events, const SteadyClock::time_point & deadline)
    {
        while(true)
        {
            const int timeout_ms = remaining_milliseconds(deadline);
            if(timeout_ms == 0)
                return 0;

            pollfd descriptor{fd, events, 0};
            const int result = poll(&descriptor, 1, timeout_ms);
            if(result > 0)
            {
                if((descriptor.revents & events) != 0)
                    return 1;
                if((descriptor.revents & POLLNVAL) != 0)
                    errno = EBADF;
                else
                    errno = EIO;
                return -1;
            }
            if(result == 0)
                return 0;
            if(errno != EINTR)
                return -1;
        }
    }


    int
    wait_for_event(int fd, short events)
    {
        while(true)
        {
            pollfd descriptor{fd, events, 0};
            const int result = poll(&descriptor, 1, -1);
            if(result > 0)
            {
                if((descriptor.revents & events) != 0)
                    return 1;
                if((descriptor.revents & POLLNVAL) != 0)
                    errno = EBADF;
                else
                    errno = EIO;
                return -1;
            }
            if(result < 0 && errno != EINTR)
                return -1;
        }
    }


    bool
    valid_buffer(const void * buffer, int length)
    {
        if(length < 0 || (buffer == nullptr && length != 0))
        {
            errno = EINVAL;
            return false;
        }
        return true;
    }
}


class SerialData
{
public:
    SerialData() = default;
    SerialData(const SerialData &) = delete;
    SerialData & operator=(const SerialData &) = delete;

    ~SerialData()
    {
        close();
    }

    void
    close() noexcept
    {
        if(fd == -1)
            return;

        const int descriptor = fd;
        fd = -1;
        if(original_options_valid)
            tcsetattr(descriptor, TCSANOW, &original_options);
        ::close(descriptor);
    }

    int fd = -1;
    termios original_options{};
    bool original_options_valid = false;
};


Serial::Serial() = default;


Serial::Serial(const std::string & device_name, unsigned long baud_rate)
{
    if(baud_rate == 0)
        throw ikaros::serial_error(device_name + ": baud rate must be greater than zero");

    time_per_byte = 10000.0f / static_cast<float>(baud_rate);
    data = std::make_unique<SerialData>();

    int open_flags = O_RDWR | O_NOCTTY | O_NONBLOCK;
#ifdef O_CLOEXEC
    open_flags |= O_CLOEXEC;
#endif
    data->fd = ::open(device_name.c_str(), open_flags);
    if(data->fd == -1)
        throw_system_error(device_name, "could not open serial device");

#ifndef O_CLOEXEC
    if(fcntl(data->fd, F_SETFD, FD_CLOEXEC) == -1)
        throw_system_error(device_name, "could not mark serial device close-on-exec");
#endif

    if(tcgetattr(data->fd, &data->original_options) == -1)
        throw_system_error(device_name, "could not read serial configuration");
    data->original_options_valid = true;

    termios options = data->original_options;
    options.c_iflag = 0;
    options.c_oflag = 0;
    options.c_lflag = 0;
    options.c_cflag &= ~(CSIZE | PARENB | CSTOPB);
#ifdef CRTSCTS
    options.c_cflag &= ~CRTSCTS;
#endif
    options.c_cflag |= CS8 | CLOCAL | CREAD;
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 0;

    speed_t baud_constant{};
    const bool standard_baud_rate = baud_rate_constant(baud_rate, baud_constant);
#ifndef MAC_OS_X
    if(!standard_baud_rate)
        throw ikaros::serial_error(device_name + ": unsupported baud rate " +
                                   std::to_string(baud_rate));
#endif

    const speed_t configured_baud_rate = standard_baud_rate ? baud_constant : B9600;
    if(cfsetispeed(&options, configured_baud_rate) == -1 ||
       cfsetospeed(&options, configured_baud_rate) == -1)
        throw_system_error(device_name, "could not configure the baud rate");
    if(tcsetattr(data->fd, TCSANOW, &options) == -1)
        throw_system_error(device_name, "could not apply serial configuration");

#ifdef MAC_OS_X
    if(!standard_baud_rate)
    {
        if(baud_rate > std::numeric_limits<speed_t>::max())
            throw ikaros::serial_error(device_name + ": baud rate is out of range");
        speed_t target_baud_rate = static_cast<speed_t>(baud_rate);
        if(ioctl(data->fd, IOSSIOSPEED, &target_baud_rate) == -1)
            throw_system_error(device_name, "could not configure the baud rate");
    }
#endif

    if(tcflush(data->fd, TCIOFLUSH) == -1)
        throw_system_error(device_name, "could not flush the serial device");
}


Serial::~Serial() = default;


void
Serial::Flush()
{
    if(data != nullptr && data->fd != -1)
        tcflush(data->fd, TCIOFLUSH);
}


void
Serial::FlushOut()
{
    if(data != nullptr && data->fd != -1)
        tcflush(data->fd, TCOFLUSH);
}


void
Serial::FlushIn()
{
    if(data != nullptr && data->fd != -1)
        tcflush(data->fd, TCIFLUSH);
}


int
Serial::SendString(const char * sendbuf)
{
    if(sendbuf == nullptr)
    {
        errno = EINVAL;
        return -1;
    }

    const std::size_t length = std::strlen(sendbuf);
    if(length > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        errno = EOVERFLOW;
        return -1;
    }
    return SendBytes(sendbuf, static_cast<int>(length));
}


int
Serial::SendBytes(const char * sendbuf, int length)
{
    if(!valid_buffer(sendbuf, length))
        return -1;
    if(data == nullptr || data->fd == -1 || length == 0)
        return 0;

    int bytes_written = 0;
    while(bytes_written < length)
    {
        const ssize_t result =
            write(data->fd, sendbuf + bytes_written,
                  static_cast<std::size_t>(length - bytes_written));
        if(result > 0)
        {
            bytes_written += static_cast<int>(result);
            continue;
        }
        if(result == 0)
        {
            errno = EIO;
            return bytes_written == 0 ? -1 : bytes_written;
        }
        if(errno == EINTR)
            continue;
        if(errno == EAGAIN || errno == EWOULDBLOCK)
        {
            if(wait_for_event(data->fd, POLLOUT) > 0)
                continue;
        }
        return bytes_written == 0 ? -1 : bytes_written;
    }
    return bytes_written;
}


int
Serial::ReceiveUntil(char * rcvbuf, int length, char c)
{
    return ReceiveUntil(rcvbuf, length, c, 100);
}


int
Serial::ReceiveUntil(char * rcvbuf, int length, char c, int timeout_ms)
{
    if(!valid_buffer(rcvbuf, length) || timeout_ms < 0)
    {
        errno = EINVAL;
        return -1;
    }
    if(data == nullptr || data->fd == -1 || length == 0)
        return 0;

    const auto deadline = SteadyClock::now() + std::chrono::milliseconds(timeout_ms);
    int bytes_read = 0;
    while(bytes_read < length)
    {
        const ssize_t result = read(data->fd, rcvbuf + bytes_read, 1);
        if(result > 0)
        {
            if(rcvbuf[bytes_read++] == c)
                break;
            continue;
        }
        if(result < 0 && errno == EINTR)
            continue;
        if(result < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
            return bytes_read == 0 ? -1 : bytes_read;

        const int wait_result = wait_for_event(data->fd, POLLIN, deadline);
        if(wait_result > 0)
            continue;
        if(wait_result < 0 && bytes_read == 0)
            return -1;
        break;
    }
    return bytes_read;
}


int
Serial::ReceiveBytes(char * rcvbuf, int length)
{
    return ReceiveBytes(rcvbuf, length, 100);
}


int
Serial::ReceiveBytes(char * rcvbuf, int length, int timeout_ms)
{
    if(!valid_buffer(rcvbuf, length) || timeout_ms < 0)
    {
        errno = EINVAL;
        return -1;
    }
    if(data == nullptr || data->fd == -1 || length == 0)
        return 0;

    const auto deadline = SteadyClock::now() + std::chrono::milliseconds(timeout_ms);
    int bytes_read = 0;
    while(bytes_read < length)
    {
        const ssize_t result =
            read(data->fd, rcvbuf + bytes_read,
                 static_cast<std::size_t>(length - bytes_read));
        if(result > 0)
        {
            bytes_read += static_cast<int>(result);
            continue;
        }
        if(result < 0 && errno == EINTR)
            continue;
        if(result < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
            return bytes_read == 0 ? -1 : bytes_read;

        const int wait_result = wait_for_event(data->fd, POLLIN, deadline);
        if(wait_result > 0)
            continue;
        if(wait_result < 0 && bytes_read == 0)
            return -1;
        break;
    }
    return bytes_read;
}


void
Serial::Close()
{
    if(data != nullptr)
        data->close();
}
