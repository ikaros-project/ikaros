#include "LaunchpadProtocol.h"

#include <algorithm>
#include <cmath>


namespace ikaros::launchpad
{
    namespace
    {
        constexpr std::uint8_t sysexStart = 0xF0;
        constexpr std::uint8_t sysexEnd = 0xF7;
        constexpr std::uint8_t novation0 = 0x00;
        constexpr std::uint8_t novation1 = 0x20;
        constexpr std::uint8_t novation2 = 0x29;
        constexpr std::uint8_t launchpadFamily = 0x02;
        constexpr std::uint8_t launchpadX = 0x0C;
        constexpr std::uint8_t ledLightingCommand = 0x03;
        constexpr std::uint8_t programmerModeCommand = 0x0E;
        constexpr std::uint8_t rgbLightingType = 0x03;
    }


    bool
    padLEDIndex(std::size_t sequence, std::size_t layoutWidth,
                std::uint8_t & index) noexcept
    {
        if(layoutWidth == 0 || layoutWidth > padColumns)
            return false;

        const std::size_t row = sequence / layoutWidth;
        const std::size_t column = sequence % layoutWidth;
        if(row >= padRows)
            return false;

        index = static_cast<std::uint8_t>((padRows - row) * 10 + column + 1);
        return true;
    }


    std::uint8_t
    colorComponent(float value, float brightness) noexcept
    {
        if(!std::isfinite(value) || !std::isfinite(brightness) ||
           value <= 0 || brightness <= 0)
            return 0;

        const float normalized = value <= 1
            ? std::min(value, 1.0f)
            : std::min(value, 255.0f) / 255.0f;
        const float scaled = normalized * std::min(brightness, 1.0f) * 127.0f;
        return static_cast<std::uint8_t>(std::lround(scaled));
    }


    void
    buildProgrammerModeMessage(bool enabled,
                               std::vector<std::uint8_t> & message)
    {
        message = {
            sysexStart,
            novation0,
            novation1,
            novation2,
            launchpadFamily,
            launchpadX,
            programmerModeCommand,
            static_cast<std::uint8_t>(enabled ? 1 : 0),
            sysexEnd,
        };
    }


    void
    buildRGBMessage(std::span<const LEDUpdate> updates,
                    std::vector<std::uint8_t> & message)
    {
        message.clear();
        if(updates.empty())
            return;

        message.reserve(8 + 5 * updates.size());
        message.insert(message.end(), {
            sysexStart,
            novation0,
            novation1,
            novation2,
            launchpadFamily,
            launchpadX,
            ledLightingCommand,
        });
        for(const LEDUpdate & update : updates)
        {
            message.push_back(rgbLightingType);
            message.push_back(update.index);
            message.push_back(update.color.red);
            message.push_back(update.color.green);
            message.push_back(update.color.blue);
        }
        message.push_back(sysexEnd);
    }
}
