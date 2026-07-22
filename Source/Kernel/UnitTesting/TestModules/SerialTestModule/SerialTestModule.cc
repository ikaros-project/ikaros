#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <future>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include "ikaros.h"

using namespace ikaros;

namespace
{
    using SteadyClock = std::chrono::steady_clock;


    void
    require(bool condition, const std::string & message)
    {
        if(!condition)
            throw exception("SerialTestModule: " + message);
    }


    template<typename Function>
    void
    require_serial_error(Function function, const std::string & message)
    {
        try
        {
            function();
        }
        catch(const serial_error &)
        {
            return;
        }
        throw exception("SerialTestModule: " + message);
    }


    class PseudoTerminal
    {
    public:
        PseudoTerminal()
        {
            master_ = posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
            if(master_ == -1)
                fail("could not open pseudo-terminal");
            if(grantpt(master_) == -1 || unlockpt(master_) == -1)
                fail("could not initialize pseudo-terminal");
            const char * name = ptsname(master_);
            if(name == nullptr)
                fail("could not resolve pseudo-terminal name");
            slave_name_ = name;
        }

        PseudoTerminal(const PseudoTerminal &) = delete;
        PseudoTerminal & operator=(const PseudoTerminal &) = delete;

        ~PseudoTerminal()
        {
            if(master_ != -1)
                close(master_);
        }

        const std::string &
        slave_name() const noexcept
        {
            return slave_name_;
        }

        void
        write_all(std::span<const char> data, int timeout_ms = 3000)
        {
            const auto deadline = SteadyClock::now() +
                                  std::chrono::milliseconds(timeout_ms);
            std::size_t offset = 0;
            while(offset < data.size())
            {
                const ssize_t result =
                    write(master_, data.data() + offset, data.size() - offset);
                if(result > 0)
                {
                    offset += static_cast<std::size_t>(result);
                    continue;
                }
                if(result < 0 && errno == EINTR)
                    continue;
                if(result < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
                    fail("could not write to pseudo-terminal");
                wait_for(POLLOUT, deadline);
            }
        }

        std::vector<char>
        read_exact(std::size_t size, int timeout_ms = 3000)
        {
            std::vector<char> result(size);
            const auto deadline = SteadyClock::now() +
                                  std::chrono::milliseconds(timeout_ms);
            std::size_t offset = 0;
            while(offset < result.size())
            {
                const ssize_t count =
                    read(master_, result.data() + offset, result.size() - offset);
                if(count > 0)
                {
                    offset += static_cast<std::size_t>(count);
                    continue;
                }
                if(count < 0 && errno == EINTR)
                    continue;
                if(count < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
                    fail("could not read from pseudo-terminal");
                wait_for(POLLIN, deadline);
            }
            return result;
        }

    private:
        [[noreturn]] void
        fail(const std::string & operation) const
        {
            const int error_number = errno;
            throw std::runtime_error(operation + ": " + std::strerror(error_number));
        }

        void
        wait_for(short events, const SteadyClock::time_point & deadline) const
        {
            while(true)
            {
                const auto now = SteadyClock::now();
                if(now >= deadline)
                    throw std::runtime_error("pseudo-terminal operation timed out");
                const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                    deadline - now);
                const int timeout_ms = static_cast<int>(remaining.count()) + 1;
                pollfd descriptor{master_, events, 0};
                const int poll_result = poll(&descriptor, 1, timeout_ms);
                if(poll_result > 0 && (descriptor.revents & events) != 0)
                    return;
                if(poll_result == 0)
                    throw std::runtime_error("pseudo-terminal operation timed out");
                if(poll_result < 0 && errno == EINTR)
                    continue;
                fail("could not poll pseudo-terminal");
            }
        }

        int master_ = -1;
        std::string slave_name_;
    };
}


class SerialTestModule : public Module
{
    void
    Init() override
    {
        require_serial_error([]
        {
            Serial serial("unused", 0);
        }, "zero baud rate was accepted");

        Serial closed_serial;
        char byte = 0;
        require(closed_serial.SendBytes("x", 1) == 0 &&
                closed_serial.ReceiveBytes(&byte, 1, 0) == 0,
                "default-constructed serial port was not safely closed");
        closed_serial.Flush();
        closed_serial.FlushIn();
        closed_serial.FlushOut();
        closed_serial.Close();

        PseudoTerminal terminal;
        Serial serial(terminal.slave_name(), 9600);

        const std::string inbound = "abc\nrest";
        terminal.write_all(std::span(inbound.data(), inbound.size()));
        char line[16]{};
        const int line_length = serial.ReceiveUntil(line, sizeof(line), '\n', 100);
        require(line_length == 4 && std::string(line, line_length) == "abc\n",
                "ReceiveUntil did not stop at the first delimiter");
        char remainder[4]{};
        require(serial.ReceiveBytes(remainder, sizeof(remainder), 100) == 4 &&
                std::string(remainder, sizeof(remainder)) == "rest",
                "ReceiveUntil consumed bytes after the delimiter");

        const std::string partial_input = "xy";
        terminal.write_all(std::span(partial_input.data(), partial_input.size()));
        char partial[4]{};
        require(serial.ReceiveBytes(partial, sizeof(partial), 30) == 2 &&
                std::string(partial, 2) == partial_input,
                "ReceiveBytes discarded partial data at its deadline");

        const std::string bounded_input = "full";
        terminal.write_all(std::span(bounded_input.data(), bounded_input.size()));
        char bounded[4]{};
        require(serial.ReceiveUntil(bounded, sizeof(bounded), '\n', 100) == 4 &&
                std::string(bounded, sizeof(bounded)) == bounded_input,
                "ReceiveUntil exceeded or mishandled a full destination");

        const auto timeout_start = SteadyClock::now();
        require(serial.ReceiveBytes(&byte, 1, 30) == 0,
                "ReceiveBytes did not report a timeout without data");
        const auto timeout_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            SteadyClock::now() - timeout_start);
        require(timeout_duration.count() >= 20 && timeout_duration.count() < 2000,
                "ReceiveBytes did not honor its timeout");

        const std::string message = "serial output";
        require(serial.SendString(message.c_str()) == static_cast<int>(message.size()) &&
                terminal.read_exact(message.size()) ==
                    std::vector<char>(message.begin(), message.end()),
                "SendString did not transmit the complete string");

        std::vector<char> large_message(256 * 1024);
        for(std::size_t i = 0; i < large_message.size(); ++i)
            large_message[i] = static_cast<char>(i % 251);
        auto received = std::async(std::launch::async, [&terminal, size = large_message.size()]
        {
            return terminal.read_exact(size);
        });
        require(serial.SendBytes(large_message.data(),
                                 static_cast<int>(large_message.size())) ==
                    static_cast<int>(large_message.size()) &&
                received.get() == large_message,
                "SendBytes did not complete a large partial write");

        std::vector<char> blocked_message(8 * 1024 * 1024, 'x');
        const auto write_timeout_start = SteadyClock::now();
        const int blocked_write = serial.SendBytes(
            blocked_message.data(), static_cast<int>(blocked_message.size()), 30);
        const auto write_timeout_duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                SteadyClock::now() - write_timeout_start);
        require(blocked_write < static_cast<int>(blocked_message.size()) &&
                write_timeout_duration.count() >= 20 &&
                write_timeout_duration.count() < 2000,
                "SendBytes did not honor its write timeout");

        require(serial.SendBytes(nullptr, 1, 0) == -1 && errno == EINVAL &&
                serial.SendBytes("x", 1, -1) == -1 && errno == EINVAL &&
                serial.ReceiveBytes(nullptr, 1, 0) == -1 && errno == EINVAL &&
                serial.ReceiveBytes(&byte, 1, -1) == -1 && errno == EINVAL,
                "serial I/O accepted invalid arguments");

        serial.Close();
        serial.Close();
        serial.Flush();
        require(serial.SendBytes("x", 1) == 0 &&
                serial.ReceiveBytes(&byte, 1, 0) == 0,
                "closed serial port remained active");

        std::cout << "SERIAL TEST OK" << std::endl;
    }
};

INSTALL_CLASS(SerialTestModule)
