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
#include <bit>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>

#include "ikaros.h"

using namespace ikaros;

class UM7 : public Module
{
    enum class ParseResult
    {
        packetReady,
        incomplete,
        discardedGarbage,
        invalidPacketType,
        checksumError,
    };

    struct UM7Packet
    {
        std::uint8_t address = 0;
        std::uint8_t packetType = 0;
        std::size_t dataLength = 0;
        std::array<std::uint8_t, 64> data{};
    };

    static constexpr std::size_t maxBufferSize = 1024;
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
    UM7Packet packet;
    std::unique_ptr<Serial> serial;
    std::array<char, 256> rxData{};
    std::string rxBuffer;
    std::size_t parseErrorCount = 0;
    std::size_t serialErrorCount = 0;
    std::size_t dataErrorCount = 0;

    static float
    bytesToFloatBigEndian(const std::uint8_t * bytes)
    {
        const std::uint32_t value =
            (static_cast<std::uint32_t>(bytes[0]) << 24) |
            (static_cast<std::uint32_t>(bytes[1]) << 16) |
            (static_cast<std::uint32_t>(bytes[2]) << 8) |
            static_cast<std::uint32_t>(bytes[3]);
        return std::bit_cast<float>(value);
    }


    static int
    signedWord(std::uint8_t high, std::uint8_t low)
    {
        const int value = (static_cast<int>(high) << 8) | static_cast<int>(low);
        return value >= 0x8000 ? value - 0x10000 : value;
    }


    static std::uint8_t
    byteAt(const std::string & buffer, std::size_t index)
    {
        return static_cast<std::uint8_t>(static_cast<unsigned char>(buffer[index]));
    }


    int
    receiveBytes(int timeoutMs)
    {
        const int received = serial->ReceiveBytes(
            rxData.data(), static_cast<int>(rxData.size()), timeoutMs);
        if(received > 0)
            rxBuffer.append(rxData.data(), static_cast<std::size_t>(received));
        return received;
    }


    void
    discardGarbageWithoutLosingHeaderPrefix()
    {
        std::size_t keep = 0;
        if(rxBuffer.size() >= 2 &&
           rxBuffer.compare(rxBuffer.size() - 2, 2, "sn") == 0)
            keep = 2;
        else if(!rxBuffer.empty() && rxBuffer.back() == 's')
            keep = 1;

        if(keep == 0)
            rxBuffer.clear();
        else
            rxBuffer.erase(0, rxBuffer.size() - keep);
    }


    ParseResult
    parsePacket()
    {
        if(rxBuffer.size() < 3)
            return ParseResult::incomplete;

        const std::size_t header = rxBuffer.find("snp");
        if(header == std::string::npos)
        {
            discardGarbageWithoutLosingHeaderPrefix();
            return ParseResult::discardedGarbage;
        }
        if(header != 0)
        {
            rxBuffer.erase(0, header);
            return ParseResult::discardedGarbage;
        }
        if(rxBuffer.size() < 7)
            return ParseResult::incomplete;

        const std::uint8_t packetType = byteAt(rxBuffer, 3);
        const bool hasData = (packetType & 0x80) != 0;
        const bool isBatch = (packetType & 0x40) != 0;
        const std::size_t batchLength = (packetType >> 2) & 0x0F;

        std::size_t dataLength = 0;
        if(hasData)
        {
            if(isBatch)
            {
                if(batchLength == 0)
                {
                    rxBuffer.erase(0, 1);
                    return ParseResult::invalidPacketType;
                }
                dataLength = 4 * batchLength;
            }
            else
                dataLength = 4;
        }

        if(dataLength > packet.data.size())
        {
            rxBuffer.erase(0, 1);
            return ParseResult::invalidPacketType;
        }

        const std::size_t packetLength = 7 + dataLength;
        if(rxBuffer.size() < packetLength)
            return ParseResult::incomplete;

        packet.address = byteAt(rxBuffer, 4);
        packet.packetType = packetType;
        packet.dataLength = dataLength;

        std::uint16_t computedChecksum =
            static_cast<std::uint16_t>('s' + 'n' + 'p' + packetType + packet.address);
        for(std::size_t i = 0; i < dataLength; ++i)
        {
            packet.data[i] = byteAt(rxBuffer, 5 + i);
            computedChecksum = static_cast<std::uint16_t>(computedChecksum + packet.data[i]);
        }

        const std::uint16_t receivedChecksum =
            static_cast<std::uint16_t>(byteAt(rxBuffer, 5 + dataLength) << 8) |
            byteAt(rxBuffer, 6 + dataLength);
        if(receivedChecksum != computedChecksum)
        {
            rxBuffer.erase(0, 1);
            return ParseResult::checksumError;
        }

        rxBuffer.erase(0, packetLength);
        return ParseResult::packetReady;
    }


    void
    reportParseError(ParseResult result)
    {
        ++parseErrorCount;
        if(parseErrorCount != 1 && parseErrorCount % 100 != 0)
            return;

        switch(result)
        {
        case ParseResult::discardedGarbage:
            Warning("UM7 discarded serial data before the next packet header.");
            break;
        case ParseResult::invalidPacketType:
            Warning("UM7 received an invalid packet type.");
            break;
        case ParseResult::checksumError:
            Warning("UM7 received a packet with an invalid checksum.");
            break;
        default:
            break;
        }
    }


    void
    trimReceiveBuffer()
    {
        if(rxBuffer.size() <= maxBufferSize)
            return;

        discardGarbageWithoutLosingHeaderPrefix();
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
                const ParseResult result = parsePacket();
                if(result == ParseResult::packetReady)
                {
                    if(packet.address != address)
                        continue;
                    if((packet.packetType & 0x01) != 0)
                        throw std::runtime_error(
                            "UM7 rejected configuration register " + std::to_string(address));
                    if(packet.packetType == 0)
                        return;
                    continue;
                }
                if(result == ParseResult::incomplete)
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
        std::array<std::uint8_t, 11> packetData
        {
            's', 'n', 'p', 0x80, address,
            data[0], data[1], data[2], data[3], 0, 0,
        };
        std::uint16_t checksum = 0;
        for(std::size_t i = 0; i < 9; ++i)
            checksum = static_cast<std::uint16_t>(checksum + packetData[i]);
        packetData[9] = static_cast<std::uint8_t>(checksum >> 8);
        packetData[10] = static_cast<std::uint8_t>(checksum);

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

        gyroProc(0) = bytesToFloatBigEndian(packet.data.data());
        gyroProc(1) = bytesToFloatBigEndian(packet.data.data() + 4);
        gyroProc(2) = bytesToFloatBigEndian(packet.data.data() + 8);
    }


    void
    processAccelData()
    {
        if(packet.dataLength < 12)
        {
            reportDataError("UM7 accelerometer packet contained fewer than 12 data bytes.");
            return;
        }

        accelProc(0) = bytesToFloatBigEndian(packet.data.data());
        accelProc(1) = bytesToFloatBigEndian(packet.data.data() + 4);
        accelProc(2) = bytesToFloatBigEndian(packet.data.data() + 8);
    }


    void
    processEulerAngles()
    {
        if(packet.dataLength < 20)
        {
            reportDataError("UM7 Euler-angle packet contained fewer than 20 data bytes.");
            return;
        }

        const float rollValue = static_cast<float>(signedWord(packet.data[0], packet.data[1])) /
                                91.02222f;
        const float pitchValue = static_cast<float>(signedWord(packet.data[2], packet.data[3])) /
                                 91.02222f;
        const float yawValue = static_cast<float>(signedWord(packet.data[4], packet.data[5])) /
                               91.02222f;

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
            const ParseResult result = parsePacket();
            if(result == ParseResult::packetReady)
            {
                processPacket();
                continue;
            }
            if(result == ParseResult::incomplete)
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
        rxBuffer.reserve(maxBufferSize);

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
