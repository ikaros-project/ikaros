#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>


namespace ikaros::um7
{
    enum class ParseResult
    {
        packetReady,
        incomplete,
        discardedGarbage,
        invalidPacketType,
        checksumError,
    };


    struct Packet
    {
        static constexpr std::size_t dataCapacity = 64;

        bool commandFailed() const noexcept { return (packetType & 0x01) != 0; }
        bool commandComplete() const noexcept { return packetType == 0; }

        std::uint8_t address = 0;
        std::uint8_t packetType = 0;
        std::size_t dataLength = 0;
        std::array<std::uint8_t, dataCapacity> data{};
    };


    class PacketParser
    {
    public:
        static constexpr std::size_t maxBufferSize = 1024;

        PacketParser();

        void append(std::span<const char> bytes);
        ParseResult parse(Packet & packet);
        bool trim();
        std::size_t bufferedSize() const noexcept { return buffer_.size(); }

    private:
        static std::uint8_t byteAt(const std::string & buffer, std::size_t index);
        void discardGarbageWithoutLosingHeaderPrefix();

        std::string buffer_;
    };


    float decodeFloat(std::span<const std::uint8_t, 4> bytes) noexcept;
    int decodeSignedWord(std::uint8_t high, std::uint8_t low) noexcept;
    float decodeEulerAngle(std::uint8_t high, std::uint8_t low) noexcept;

    std::array<std::uint8_t, 11> makeWritePacket(
        std::uint8_t address, const std::array<std::uint8_t, 4> & data) noexcept;
}
