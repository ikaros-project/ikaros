#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>


namespace ikaros::launchpad
{
    struct RGBColor
    {
        std::uint8_t red = 0;
        std::uint8_t green = 0;
        std::uint8_t blue = 0;

        bool operator==(const RGBColor &) const = default;
    };


    struct LEDUpdate
    {
        std::uint8_t index = 0;
        RGBColor color;
    };


    constexpr std::size_t padRows = 8;
    constexpr std::size_t padColumns = 8;
    constexpr std::size_t padCount = padRows * padColumns;

    bool padLEDIndex(std::size_t sequence, std::size_t layoutWidth,
                     std::uint8_t & index) noexcept;
    std::uint8_t colorComponent(float value, float brightness) noexcept;
    void buildProgrammerModeMessage(bool enabled,
                                    std::vector<std::uint8_t> & message);
    void buildRGBMessage(std::span<const LEDUpdate> updates,
                         std::vector<std::uint8_t> & message);
}
