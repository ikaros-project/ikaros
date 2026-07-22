//
//  UM7.cc        This file is a part of the IKAROS project
//
//    Copyright (C) 2026 Birger Johansson and Pierre Klintefors
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

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>

#include "ikaros.h"
#include "UM7Protocol.h"

using namespace ikaros;

class UM7 : public Module
{
    static constexpr int configurationTimeoutMs = 250;
    static constexpr int configurationWriteTimeoutMs = 100;

    parameter port;
    matrix roll;
    matrix pitch;
    matrix yaw;
    matrix gyroProc;
    matrix accelProc;
    matrix eulerAngles;

    int baudrate = 115200;
    um7::Packet packet;
    um7::PacketParser packetParser;
    std::unique_ptr<Serial> serial;
    std::array<char, 256> rxData{};
    std::size_t parseErrorCount = 0;
    std::size_t serialErrorCount = 0;
    std::size_t dataErrorCount = 0;


    int
    receiveBytes(int timeoutMs)
    {
        const int received = serial->ReceiveBytes(
            rxData.data(), static_cast<int>(rxData.size()), timeoutMs);
        if(received > 0)
            packetParser.append(std::span(rxData.data(), static_cast<std::size_t>(received)));
        return received;
    }


    void
    reportParseError(um7::ParseResult result)
    {
        ++parseErrorCount;
        if(parseErrorCount != 1 && parseErrorCount % 100 != 0)
            return;

        switch(result)
        {
        case um7::ParseResult::discardedGarbage:
            Warning("UM7 discarded serial data before the next packet header.");
            break;
        case um7::ParseResult::invalidPacketType:
            Warning("UM7 received an invalid packet type.");
            break;
        case um7::ParseResult::checksumError:
            Warning("UM7 received a packet with an invalid checksum.");
            break;
        default:
            break;
        }
    }


    void
    trimReceiveBuffer()
    {
        if(!packetParser.trim())
            return;

        ++parseErrorCount;
        if(parseErrorCount == 1 || parseErrorCount % 100 == 0)
            Warning("UM7 receive buffer exceeded its limit and was resynchronized.");
    }


    void
    reportDataError(const std::string & message)
    {
        ++dataErrorCount;
        if(dataErrorCount == 1 || dataErrorCount % 100 == 0)
            Warning(message);
    }


    void
    awaitAcknowledgement(std::uint8_t address)
    {
        const auto deadline = std::chrono::steady_clock::now() +
                              std::chrono::milliseconds(configurationTimeoutMs);

        while(std::chrono::steady_clock::now() < deadline)
        {
            bool needsMoreData = false;
            for(int attempts = 0; attempts < 64; ++attempts)
            {
                const um7::ParseResult result = packetParser.parse(packet);
                if(result == um7::ParseResult::packetReady)
                {
                    if(packet.address != address)
                        continue;
                    if(packet.commandFailed())
                        throw std::runtime_error(
                            "UM7 rejected configuration register " + std::to_string(address));
                    if(packet.commandComplete())
                        return;
                    continue;
                }
                if(result == um7::ParseResult::incomplete)
                {
                    needsMoreData = true;
                    break;
                }
                reportParseError(result);
            }

            if(!needsMoreData)
                continue;

            const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - std::chrono::steady_clock::now());
            const int timeoutMs = std::clamp(static_cast<int>(remaining.count()), 1, 20);
            const int received = receiveBytes(timeoutMs);
            if(received < 0)
            {
                const int errorNumber = errno;
                throw std::runtime_error(
                    "Could not receive UM7 configuration acknowledgement: " +
                    std::string(std::strerror(errorNumber)));
            }
        }

        throw std::runtime_error(
            "Timed out waiting for UM7 configuration register " + std::to_string(address));
    }


    void
    sendPacket(const std::array<std::uint8_t, 11> & packetData)
    {
        const int sent = serial->SendBytes(
            reinterpret_cast<const char *>(packetData.data()),
            static_cast<int>(packetData.size()), configurationWriteTimeoutMs);
        if(sent != static_cast<int>(packetData.size()))
            throw std::runtime_error("Could not send the complete UM7 configuration packet");
    }


    void
    sendConfig(std::uint8_t address, std::array<std::uint8_t, 4> data)
    {
        const auto packetData = um7::makeWritePacket(address, data);
        sendPacket(packetData);
        awaitAcknowledgement(address);
    }


    void
    disableBroadcast()
    {
        for(std::uint8_t address = 1; address <= 7; ++address)
            sendConfig(address, {0, 0, 0, 0});
    }


    void
    processGyroData()
    {
        if(packet.dataLength < 12)
        {
            reportDataError("UM7 gyro packet contained fewer than 12 data bytes.");
            return;
        }

        gyroProc(0) = um7::decodeFloat(
            std::span<const std::uint8_t, 4>(packet.data.data(), 4));
        gyroProc(1) = um7::decodeFloat(
            std::span<const std::uint8_t, 4>(packet.data.data() + 4, 4));
        gyroProc(2) = um7::decodeFloat(
            std::span<const std::uint8_t, 4>(packet.data.data() + 8, 4));
    }


    void
    processAccelData()
    {
        if(packet.dataLength < 12)
        {
            reportDataError("UM7 accelerometer packet contained fewer than 12 data bytes.");
            return;
        }

        accelProc(0) = um7::decodeFloat(
            std::span<const std::uint8_t, 4>(packet.data.data(), 4));
        accelProc(1) = um7::decodeFloat(
            std::span<const std::uint8_t, 4>(packet.data.data() + 4, 4));
        accelProc(2) = um7::decodeFloat(
            std::span<const std::uint8_t, 4>(packet.data.data() + 8, 4));
    }


    void
    processEulerAngles()
    {
        if(packet.dataLength < 20)
        {
            reportDataError("UM7 Euler-angle packet contained fewer than 20 data bytes.");
            return;
        }

        const float rollValue = um7::decodeEulerAngle(packet.data[0], packet.data[1]);
        const float pitchValue = um7::decodeEulerAngle(packet.data[2], packet.data[3]);
        const float yawValue = um7::decodeEulerAngle(packet.data[4], packet.data[5]);

        eulerAngles(0) = rollValue;
        eulerAngles(1) = pitchValue;
        eulerAngles(2) = yawValue;
        roll(0) = rollValue;
        pitch(0) = pitchValue;
        yaw(0) = yawValue;
    }


    void
    processPacket()
    {
        switch(packet.address)
        {
        case 0x61:
            processGyroData();
            break;
        case 0x65:
            processAccelData();
            break;
        case 0x70:
            processEulerAngles();
            break;
        default:
            break;
        }
    }


    void
    processAvailablePackets()
    {
        while(true)
        {
            const um7::ParseResult result = packetParser.parse(packet);
            if(result == um7::ParseResult::packetReady)
            {
                processPacket();
                continue;
            }
            if(result == um7::ParseResult::incomplete)
                break;
            reportParseError(result);
        }

        trimReceiveBuffer();
    }

public:
    void
    Init() override
    {
        Bind(port, "port");
        Bind(roll, "ROLL");
        Bind(pitch, "PITCH");
        Bind(yaw, "YAW");
        Bind(gyroProc, "ProcessedGyro");
        Bind(accelProc, "ProcessedAccel");
        Bind(eulerAngles, "EulerAngles");

        serial = std::make_unique<Serial>(std::string(port), baudrate);
        Debug("Serial port for UM7 opened on " + std::string(port) +
              " with baudrate " + std::to_string(baudrate));
        disableBroadcast();
        sendConfig(0x05, {0, 100, 0, 0});
        sendConfig(0x03, {100, 100, 0, 0});
        Debug("UM7 configured for 100 Hz gyro, accelerometer, and Euler-angle broadcasts.");
    }


    void
    Tick() override
    {
        while(true)
        {
            const int received = receiveBytes(0);
            if(received < 0)
            {
                const int errorNumber = errno;
                ++serialErrorCount;
                if(serialErrorCount == 1 || serialErrorCount % 100 == 0)
                    Warning("UM7 serial receive failed: " +
                            std::string(std::strerror(errorNumber)));
                return;
            }
            if(received == 0)
                return;

            processAvailablePackets();

            if(received < static_cast<int>(rxData.size()))
                return;
        }
    }
};

INSTALL_CLASS(UM7)
