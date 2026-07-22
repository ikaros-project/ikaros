#include "UM7Protocol.h"

#include <bit>


namespace ikaros::um7
{
    PacketParser::PacketParser()
    {
        buffer_.reserve(maxBufferSize);
    }


    void
    PacketParser::append(std::span<const char> bytes)
    {
        buffer_.append(bytes.data(), bytes.size());
    }


    std::uint8_t
    PacketParser::byteAt(const std::string & buffer, std::size_t index)
    {
        return static_cast<std::uint8_t>(static_cast<unsigned char>(buffer[index]));
    }


    void
    PacketParser::discardGarbageWithoutLosingHeaderPrefix()
    {
        std::size_t keep = 0;
        if(buffer_.size() >= 2 &&
           buffer_.compare(buffer_.size() - 2, 2, "sn") == 0)
            keep = 2;
        else if(!buffer_.empty() && buffer_.back() == 's')
            keep = 1;

        if(keep == 0)
            buffer_.clear();
        else
            buffer_.erase(0, buffer_.size() - keep);
    }


    ParseResult
    PacketParser::parse(Packet & packet)
    {
        if(buffer_.size() < 3)
            return ParseResult::incomplete;

        const std::size_t header = buffer_.find("snp");
        if(header == std::string::npos)
        {
            discardGarbageWithoutLosingHeaderPrefix();
            return ParseResult::discardedGarbage;
        }
        if(header != 0)
        {
            buffer_.erase(0, header);
            return ParseResult::discardedGarbage;
        }
        if(buffer_.size() < 7)
            return ParseResult::incomplete;

        const std::uint8_t packetType = byteAt(buffer_, 3);
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
                    buffer_.erase(0, 1);
                    return ParseResult::invalidPacketType;
                }
                dataLength = 4 * batchLength;
            }
            else
                dataLength = 4;
        }

        if(dataLength > packet.data.size())
        {
            buffer_.erase(0, 1);
            return ParseResult::invalidPacketType;
        }

        const std::size_t packetLength = 7 + dataLength;
        if(buffer_.size() < packetLength)
            return ParseResult::incomplete;

        packet.address = byteAt(buffer_, 4);
        packet.packetType = packetType;
        packet.dataLength = dataLength;

        std::uint16_t computedChecksum =
            static_cast<std::uint16_t>('s' + 'n' + 'p' + packetType + packet.address);
        for(std::size_t i = 0; i < dataLength; ++i)
        {
            packet.data[i] = byteAt(buffer_, 5 + i);
            computedChecksum = static_cast<std::uint16_t>(computedChecksum + packet.data[i]);
        }

        const std::uint16_t receivedChecksum =
            static_cast<std::uint16_t>(byteAt(buffer_, 5 + dataLength) << 8) |
            byteAt(buffer_, 6 + dataLength);
        if(receivedChecksum != computedChecksum)
        {
            buffer_.erase(0, 1);
            return ParseResult::checksumError;
        }

        buffer_.erase(0, packetLength);
        return ParseResult::packetReady;
    }


    bool
    PacketParser::trim()
    {
        if(buffer_.size() <= maxBufferSize)
            return false;

        discardGarbageWithoutLosingHeaderPrefix();
        return true;
    }


    float
    decodeFloat(std::span<const std::uint8_t, 4> bytes) noexcept
    {
        const std::uint32_t value =
            (static_cast<std::uint32_t>(bytes[0]) << 24) |
            (static_cast<std::uint32_t>(bytes[1]) << 16) |
            (static_cast<std::uint32_t>(bytes[2]) << 8) |
            static_cast<std::uint32_t>(bytes[3]);
        return std::bit_cast<float>(value);
    }


    int
    decodeSignedWord(std::uint8_t high, std::uint8_t low) noexcept
    {
        const int value = (static_cast<int>(high) << 8) | static_cast<int>(low);
        return value >= 0x8000 ? value - 0x10000 : value;
    }


    float
    decodeEulerAngle(std::uint8_t high, std::uint8_t low) noexcept
    {
        return static_cast<float>(decodeSignedWord(high, low)) / 91.02222f;
    }


    std::array<std::uint8_t, 11>
    makeWritePacket(std::uint8_t address,
                    const std::array<std::uint8_t, 4> & data) noexcept
    {
        std::array<std::uint8_t, 11> packet
        {
            's', 'n', 'p', 0x80, address,
            data[0], data[1], data[2], data[3], 0, 0,
        };
        std::uint16_t checksum = 0;
        for(std::size_t i = 0; i < 9; ++i)
            checksum = static_cast<std::uint16_t>(checksum + packet[i]);
        packet[9] = static_cast<std::uint8_t>(checksum >> 8);
        packet[10] = static_cast<std::uint8_t>(checksum);
        return packet;
    }
}
