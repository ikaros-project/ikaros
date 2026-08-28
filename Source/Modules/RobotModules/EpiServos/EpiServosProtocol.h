#ifndef IKAROS_EPISERVOS_PROTOCOL_H
#define IKAROS_EPISERVOS_PROTOCOL_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ikaros::episervos
{

constexpr std::size_t torsoServoCount = 6;
constexpr std::size_t fullServoCount = 19;
constexpr std::size_t writePacketSize = 9;
constexpr std::size_t readPacketSize = 7;
constexpr double positionCountsPerRevolution = 4096.0;
constexpr double degreesPerRevolution = 360.0;
constexpr double currentMilliampPerCount = 3.36;
constexpr double pwmPercentPerCount = 0.113;

constexpr std::uint8_t writeGoalPosition = 1U << 0;
constexpr std::uint8_t writeGoalCurrent = 1U << 1;
constexpr std::uint8_t writeTorqueEnable = 1U << 2;
constexpr std::uint8_t writeGoalPWM = 1U << 3;

enum class ServoBus
{
    head,
    pupil,
    leftArm,
    rightArm,
    body,
};

struct ServoRoute
{
    ServoBus bus;
    std::uint8_t id;
    std::size_t ioIndex;
    bool supportsGoalCurrent;
    bool supportsIndirectAddress;
};

constexpr std::array<ServoRoute, fullServoCount> servoRoutes = {{
    {ServoBus::head, 2, 0, true, true},
    {ServoBus::head, 3, 1, true, true},
    {ServoBus::head, 4, 2, false, true},
    {ServoBus::head, 5, 3, false, true},
    {ServoBus::pupil, 2, 4, false, false},
    {ServoBus::pupil, 3, 5, false, false},
    {ServoBus::leftArm, 2, 6, true, true},
    {ServoBus::leftArm, 3, 7, true, true},
    {ServoBus::leftArm, 4, 8, true, true},
    {ServoBus::leftArm, 5, 9, true, true},
    {ServoBus::leftArm, 6, 10, true, true},
    {ServoBus::leftArm, 7, 11, false, true},
    {ServoBus::rightArm, 2, 12, true, true},
    {ServoBus::rightArm, 3, 13, true, true},
    {ServoBus::rightArm, 4, 14, true, true},
    {ServoBus::rightArm, 5, 15, true, true},
    {ServoBus::rightArm, 6, 16, true, true},
    {ServoBus::rightArm, 7, 17, false, true},
    {ServoBus::body, 2, 18, true, true},
}};

struct WriteCommand
{
    bool torqueEnable;
    std::uint32_t goalPosition;
    std::int32_t goalCurrent;
    std::int32_t goalPWM;
};

struct ReadState
{
    std::uint32_t presentPosition;
    std::int32_t presentCurrent;
    std::uint8_t temperature;
};

inline std::string_view trim(std::string_view value)
{
    const std::size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos)
        return {};

    const std::size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

inline std::uint8_t parseWriteSelection(std::string_view dataToWrite)
{
    if (trim(dataToWrite).empty())
        throw std::invalid_argument("DataToWrite must not be empty");

    std::uint8_t selection = 0;
    std::size_t start = 0;
    while (start <= dataToWrite.size())
    {
        const std::size_t comma = dataToWrite.find(',', start);
        const std::size_t end = comma == std::string_view::npos ? dataToWrite.size() : comma;
        const std::string_view item = trim(dataToWrite.substr(start, end - start));
        if (item.empty())
            throw std::invalid_argument("DataToWrite contains an empty field");

        std::uint8_t field = 0;
        if (item == "Goal Position")
            field = writeGoalPosition;
        else if (item == "Goal Current")
            field = writeGoalCurrent;
        else if (item == "Torque Enable")
            field = writeTorqueEnable;
        else if (item == "Goal PWM")
            field = writeGoalPWM;
        else
            throw std::invalid_argument("DataToWrite contains an unknown field: " + std::string(item));

        if ((selection & field) != 0)
            throw std::invalid_argument("DataToWrite contains a duplicate field: " + std::string(item));
        selection |= field;

        if (comma == std::string_view::npos)
            break;
        start = comma + 1;
    }

    if ((selection & writeGoalPosition) == 0)
        throw std::invalid_argument("DataToWrite must include Goal Position");
    return selection;
}

inline std::size_t validatedWritePacketSize(std::string_view dataToWrite)
{
    parseWriteSelection(dataToWrite);
    return writePacketSize;
}

inline std::uint32_t degreesToRawPosition(double degrees)
{
    if (!std::isfinite(degrees))
        throw std::invalid_argument("position must be finite");

    const double raw = std::round(degrees / degreesPerRevolution * positionCountsPerRevolution);
    return static_cast<std::uint32_t>(std::clamp(raw, 0.0, positionCountsPerRevolution - 1.0));
}

inline double rawPositionToDegrees(std::uint32_t raw)
{
    const std::uint32_t saturated = std::min(raw, static_cast<std::uint32_t>(positionCountsPerRevolution - 1.0));
    return saturated / positionCountsPerRevolution * degreesPerRevolution;
}

inline std::int32_t decodeSignedWord(std::uint16_t raw)
{
    if (raw <= 0x7fffU)
        return raw;
    return static_cast<std::int32_t>(raw) - 0x10000;
}

inline std::int32_t currentMilliampToRaw(double milliamp)
{
    if (!std::isfinite(milliamp))
        throw std::invalid_argument("current must be finite");

    const double raw = std::round(milliamp / currentMilliampPerCount);
    return static_cast<std::int32_t>(std::clamp(raw, -32768.0, 32767.0));
}

inline double rawCurrentToMilliamp(std::uint16_t raw)
{
    return decodeSignedWord(raw) * currentMilliampPerCount;
}

inline std::int32_t pwmPercentToRaw(double percent)
{
    if (!std::isfinite(percent))
        throw std::invalid_argument("PWM must be finite");

    const double boundedPercent = std::clamp(percent, -100.0, 100.0);
    return static_cast<std::int32_t>(std::round(boundedPercent / pwmPercentPerCount));
}

inline std::int64_t jsonParameterToRaw(std::string_view parameterName, double value)
{
    if (parameterName == "Goal Position")
        return degreesToRawPosition(value);
    if (parameterName == "Goal PWM")
        return pwmPercentToRaw(value);
    if (!std::isfinite(value))
        throw std::invalid_argument("servo parameter must be finite");
    return static_cast<std::int64_t>(value);
}

inline double rawPWMToPercent(std::uint16_t raw)
{
    return decodeSignedWord(raw) * pwmPercentPerCount;
}

inline void encodeWord(std::array<std::uint8_t, writePacketSize> & packet,
                       std::size_t offset,
                       std::int32_t value)
{
    const std::uint16_t raw = static_cast<std::uint16_t>(value);
    packet.at(offset) = static_cast<std::uint8_t>(raw & 0xffU);
    packet.at(offset + 1) = static_cast<std::uint8_t>((raw >> 8U) & 0xffU);
}

inline std::array<std::uint8_t, writePacketSize> encodeWritePacket(const WriteCommand & command)
{
    std::array<std::uint8_t, writePacketSize> packet{};
    packet[0] = command.torqueEnable ? 1 : 0;
    packet[1] = static_cast<std::uint8_t>(command.goalPosition & 0xffU);
    packet[2] = static_cast<std::uint8_t>((command.goalPosition >> 8U) & 0xffU);
    packet[3] = static_cast<std::uint8_t>((command.goalPosition >> 16U) & 0xffU);
    packet[4] = static_cast<std::uint8_t>((command.goalPosition >> 24U) & 0xffU);
    encodeWord(packet, 5, command.goalCurrent);
    encodeWord(packet, 7, command.goalPWM);
    return packet;
}

inline std::uint32_t decodeDoubleWord(const std::array<std::uint8_t, readPacketSize> & packet,
                                      std::size_t offset)
{
    return static_cast<std::uint32_t>(packet.at(offset)) |
           (static_cast<std::uint32_t>(packet.at(offset + 1)) << 8U) |
           (static_cast<std::uint32_t>(packet.at(offset + 2)) << 16U) |
           (static_cast<std::uint32_t>(packet.at(offset + 3)) << 24U);
}

inline ReadState decodeReadPacket(const std::array<std::uint8_t, readPacketSize> & packet)
{
    const std::uint16_t current = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(packet[4]) |
        (static_cast<std::uint16_t>(packet[5]) << 8U));
    return {decodeDoubleWord(packet, 0), decodeSignedWord(current), packet[6]};
}

template<typename Scalar>
inline void simulateStep(Scalar * present,
                         std::size_t presentSize,
                         const Scalar * goal,
                         std::size_t goalSize,
                         std::size_t activeServoCount,
                         double tickSeconds,
                         double maximumDegreesPerSecond = 45.0)
{
    if (activeServoCount > presentSize || activeServoCount > goalSize)
        throw std::invalid_argument("simulation buffers are smaller than the active servo count");
    if (!std::isfinite(tickSeconds) || tickSeconds < 0 ||
        !std::isfinite(maximumDegreesPerSecond) || maximumDegreesPerSecond < 0)
        throw std::invalid_argument("simulation timing must be finite and non-negative");

    for (std::size_t i = 0; i < activeServoCount; ++i)
        if (!std::isfinite(goal[i]))
            throw std::invalid_argument("simulation goal must be finite");

    const double maximumChange = maximumDegreesPerSecond * tickSeconds;
    for (std::size_t i = 0; i < activeServoCount; ++i)
    {
        const double difference = std::clamp(static_cast<double>(goal[i]) - present[i],
                                             -maximumChange, maximumChange);
        present[i] = static_cast<Scalar>(present[i] + 0.9 * difference);
    }
}

} // namespace ikaros::episervos

#endif
