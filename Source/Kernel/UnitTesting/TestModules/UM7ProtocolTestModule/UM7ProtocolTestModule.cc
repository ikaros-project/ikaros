#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <vector>

#include "ikaros.h"
#include "Modules/RobotModules/UM7/UM7Protocol.h"

using namespace ikaros;

namespace
{
    void
    require(bool condition, const std::string & message)
    {
        if(!condition)
            throw exception("UM7ProtocolTestModule: " + message);
    }


    std::vector<char>
    makePacket(std::uint8_t packetType, std::uint8_t address,
               std::span<const std::uint8_t> data = {})
    {
        std::vector<char> result
        {
            's', 'n', 'p', static_cast<char>(packetType), static_cast<char>(address),
        };
        result.reserve(7 + data.size());
        for(const std::uint8_t byte : data)
            result.push_back(static_cast<char>(byte));

        std::uint16_t checksum = 0;
        for(const char byte : result)
            checksum = static_cast<std::uint16_t>(
                checksum + static_cast<unsigned char>(byte));
        result.push_back(static_cast<char>(checksum >> 8));
        result.push_back(static_cast<char>(checksum));
        return result;
    }


    void
    append(um7::PacketParser & parser, const std::vector<char> & bytes)
    {
        parser.append(std::span(bytes.data(), bytes.size()));
    }


    void
    append(um7::PacketParser & parser, const std::vector<char> & bytes,
           std::size_t offset)
    {
        parser.append(std::span(bytes.data() + offset, bytes.size() - offset));
    }


    void
    append(um7::PacketParser & parser, const std::string & bytes)
    {
        parser.append(std::span(bytes.data(), bytes.size()));
    }


    void
    appendBoth(um7::PacketParser & parser, const std::vector<char> & first,
               const std::vector<char> & second)
    {
        std::vector<char> bytes = first;
        bytes.insert(bytes.end(), second.begin(), second.end());
        append(parser, bytes);
    }


    void
    requireNextPacket(um7::PacketParser & parser, um7::Packet & packet,
                      std::uint8_t expectedAddress)
    {
        for(int attempt = 0; attempt < 8; ++attempt)
        {
            const um7::ParseResult result = parser.parse(packet);
            if(result == um7::ParseResult::packetReady)
            {
                require(packet.address == expectedAddress,
                        "recovery produced the wrong packet");
                return;
            }
            require(result != um7::ParseResult::incomplete,
                    "parser stopped before reaching the following packet");
        }
        throw exception("UM7ProtocolTestModule: parser did not recover to the next packet");
    }


    void
    testSplitHeader()
    {
        const auto acknowledgement = makePacket(0, 0x05);
        um7::PacketParser parser;
        um7::Packet packet;

        parser.append(std::span(acknowledgement.data(), 1));
        require(parser.parse(packet) == um7::ParseResult::incomplete,
                "single-byte header was not retained");
        parser.append(std::span(acknowledgement.data() + 1, 1));
        require(parser.parse(packet) == um7::ParseResult::incomplete,
                "two-byte header was not retained");
        append(parser, acknowledgement, 2);
        require(parser.parse(packet) == um7::ParseResult::packetReady &&
                packet.address == 0x05 && packet.commandComplete(),
                "packet split inside its header was not parsed");
    }


    void
    testGarbageRecovery()
    {
        const auto acknowledgement = makePacket(0, 0x03);
        um7::PacketParser parser;
        um7::Packet packet;

        append(parser, std::string("noise sn"));
        require(parser.parse(packet) == um7::ParseResult::discardedGarbage &&
                parser.bufferedSize() == 2,
                "garbage recovery lost a partial header suffix");
        append(parser, acknowledgement, 2);
        require(parser.parse(packet) == um7::ParseResult::packetReady &&
                packet.address == 0x03,
                "parser did not resume from a retained header suffix");
    }


    void
    testMaximumBatch()
    {
        std::array<std::uint8_t, 60> data{};
        for(std::size_t i = 0; i < data.size(); ++i)
            data[i] = static_cast<std::uint8_t>(i);
        const auto batch = makePacket(0xFC, 0x20, data);

        um7::PacketParser parser;
        um7::Packet packet;
        append(parser, batch);
        require(parser.parse(packet) == um7::ParseResult::packetReady &&
                packet.address == 0x20 && packet.dataLength == data.size() &&
                std::equal(data.begin(), data.end(), packet.data.begin()),
                "maximum encoded batch was not copied safely");
    }


    void
    testInvalidBatchRecovery()
    {
        const auto invalidBatch = makePacket(0xC0, 0x30);
        const auto acknowledgement = makePacket(0, 0x04);
        um7::PacketParser parser;
        um7::Packet packet;
        appendBoth(parser, invalidBatch, acknowledgement);

        require(parser.parse(packet) == um7::ParseResult::invalidPacketType,
                "zero-length batch was accepted");
        requireNextPacket(parser, packet, 0x04);
    }


    void
    testChecksumRecovery()
    {
        auto damaged = makePacket(0, 0x01);
        damaged.back() ^= 1;
        const auto acknowledgement = makePacket(0, 0x02);
        um7::PacketParser parser;
        um7::Packet packet;
        appendBoth(parser, damaged, acknowledgement);

        require(parser.parse(packet) == um7::ParseResult::checksumError,
                "damaged checksum was accepted");
        requireNextPacket(parser, packet, 0x02);
    }


    void
    testBackToBackResponses()
    {
        const auto complete = makePacket(0, 0x05);
        const auto failed = makePacket(1, 0x03);
        um7::PacketParser parser;
        um7::Packet packet;
        appendBoth(parser, complete, failed);

        require(parser.parse(packet) == um7::ParseResult::packetReady &&
                packet.commandComplete() && !packet.commandFailed(),
                "COMMAND_COMPLETE response was not recognized");
        require(parser.parse(packet) == um7::ParseResult::packetReady &&
                packet.commandFailed() && !packet.commandComplete(),
                "COMMAND_FAILED response was not recognized");
    }


    void
    testWritePacket()
    {
        const std::array<std::uint8_t, 4> data{1, 2, 3, 4};
        const auto writePacket = um7::makeWritePacket(0x05, data);
        require(writePacket[0] == 's' && writePacket[1] == 'n' &&
                writePacket[2] == 'p' && writePacket[3] == 0x80 &&
                writePacket[4] == 0x05 &&
                std::equal(data.begin(), data.end(), writePacket.begin() + 5),
                "configuration write packet fields were incorrect");

        std::uint16_t checksum = 0;
        for(std::size_t i = 0; i < 9; ++i)
            checksum = static_cast<std::uint16_t>(checksum + writePacket[i]);
        require(writePacket[9] == static_cast<std::uint8_t>(checksum >> 8) &&
                writePacket[10] == static_cast<std::uint8_t>(checksum),
                "configuration write packet checksum was incorrect");
    }


    void
    testNumericDecoding()
    {
        const std::array<std::uint8_t, 4> one{0x3F, 0x80, 0x00, 0x00};
        const std::array<std::uint8_t, 4> negative{0xC0, 0x20, 0x00, 0x00};
        require(um7::decodeFloat(one) == 1.0f &&
                um7::decodeFloat(negative) == -2.5f,
                "big-endian float decoding was incorrect");
        require(um7::decodeSignedWord(0x7F, 0xFF) == 32767 &&
                um7::decodeSignedWord(0x80, 0x00) == -32768 &&
                um7::decodeSignedWord(0xFF, 0xFF) == -1,
                "signed-word decoding was incorrect");

        const float positiveAngle = um7::decodeEulerAngle(0x23, 0x8E);
        const float negativeAngle = um7::decodeEulerAngle(0xDC, 0x72);
        require(std::abs(positiveAngle - 9102.0f / 91.02222f) < 0.0001f &&
                std::abs(negativeAngle + 9102.0f / 91.02222f) < 0.0001f,
                "Euler-angle decoding was incorrect");
    }


    void
    testBufferLimitRecovery()
    {
        std::string garbage(um7::PacketParser::maxBufferSize + 1, 'x');
        garbage[garbage.size() - 2] = 's';
        garbage[garbage.size() - 1] = 'n';

        um7::PacketParser parser;
        um7::Packet packet;
        append(parser, garbage);
        require(parser.trim() && parser.bufferedSize() == 2,
                "buffer limiting lost its retained header suffix");

        const auto acknowledgement = makePacket(0, 0x06);
        append(parser, acknowledgement, 2);
        require(parser.parse(packet) == um7::ParseResult::packetReady &&
                packet.address == 0x06,
                "parser did not recover after enforcing its buffer limit");
    }
}


class UM7ProtocolTestModule : public Module
{
    void
    Init() override
    {
        testSplitHeader();
        testGarbageRecovery();
        testMaximumBatch();
        testInvalidBatchRecovery();
        testChecksumRecovery();
        testBackToBackResponses();
        testWritePacket();
        testNumericDecoding();
        testBufferLimitRecovery();
        std::cout << "UM7 PROTOCOL TEST OK" << std::endl;
    }
};

INSTALL_CLASS(UM7ProtocolTestModule)
