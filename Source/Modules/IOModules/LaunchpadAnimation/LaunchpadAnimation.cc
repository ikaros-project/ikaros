#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "ikaros.h"

using namespace ikaros;

namespace
{
    constexpr int gridSize = 8;
    constexpr int padCount = gridSize * gridSize;
    constexpr int burstCount = 8;
    constexpr float pi = 3.14159265358979323846f;
    constexpr float twoPi = 2 * pi;


    struct RGB
    {
        float red;
        float green;
        float blue;
    };


    float
    Fraction(float value)
    {
        return value - std::floor(value);
    }


    float
    SmoothPulse(float value, float exponent)
    {
        return std::pow(std::max(0.0f, value), exponent);
    }


    RGB
    HSVToRGB(float hue, float saturation, float value)
    {
        hue = Fraction(hue);
        saturation = std::clamp(saturation, 0.0f, 1.0f);
        value = std::clamp(value, 0.0f, 1.0f);

        const float sector = hue * 6;
        const int index = static_cast<int>(std::floor(sector)) % 6;
        const float fraction = sector - std::floor(sector);
        const float p = value * (1 - saturation);
        const float q = value * (1 - saturation * fraction);
        const float t = value * (1 - saturation * (1 - fraction));

        switch(index)
        {
            case 0: return {value, t, p};
            case 1: return {q, value, p};
            case 2: return {p, value, t};
            case 3: return {p, q, value};
            case 4: return {t, p, value};
            default: return {value, p, q};
        }
    }


    float
    Hash01(std::uint32_t value)
    {
        value ^= value >> 16;
        value *= 0x7FEB352Du;
        value ^= value >> 15;
        value *= 0x846CA68Bu;
        value ^= value >> 16;
        return static_cast<float>(value & 0x00FFFFFFu) /
               static_cast<float>(0x01000000u);
    }


    bool
    NoteToPad(int note, float & x, float & y)
    {
        const int rowCode = note / 10;
        const int columnCode = note % 10;
        if(rowCode < 1 || rowCode > 8 ||
           columnCode < 1 || columnCode > 8)
            return false;

        const int row = 8 - rowCode;
        const int column = columnCode - 1;
        x = (static_cast<float>(column) - 3.5f) / 3.5f;
        y = (3.5f - static_cast<float>(row)) / 3.5f;
        return true;
    }
}


class LaunchpadAnimation: public Module
{
public:
    void
    Init() override
    {
        Bind(speed_, "speed");
        Bind(brightness_, "brightness");
        Bind(saturation_, "saturation");
        Bind(sparkle_, "sparkle");
        Bind(reactivity_, "reactivity");
        Bind(frameRate_, "frame_rate");
        Bind(key_, "KEY");
        Bind(trig_, "TRIG");
        Bind(eventCount_, "EVENT_COUNT");
        Bind(color_, "COLOR");

        if(color_.rank() != 2 || color_.rows() != padCount ||
           color_.cols() != 3)
            throw exception("LaunchpadAnimation COLOR must have shape 64 by 3.");

        color_.reset();
        for(int pad = 0; pad < padCount; ++pad)
            sparklePhase_[pad] = twoPi * Hash01(
                static_cast<std::uint32_t>(pad) + 0x9E3779B9u);
        nextFrameTime_ = -std::numeric_limits<double>::infinity();
        lastTrig_ = 0;
        lastEventCount_ = eventCount_.connected() ? eventCount_(0) : 0;
        lastKey_ = key_.connected() ? static_cast<int>(key_(0)) : -1;
    }


    void
    Tick() override
    {
        const double now = GetTime();
        const bool triggered = UpdateTriggers(now);
        const double frameRate = frameRate_.as_double();
        if(!std::isfinite(nextFrameTime_))
            nextFrameTime_ = now;
        if(!triggered && now + 1e-12 < nextFrameTime_)
            return;

        const double frameInterval = 1.0 / frameRate;
        while(nextFrameTime_ <= now + 1e-12)
            nextFrameTime_ += frameInterval;
        Render(static_cast<float>(now));
    }

private:
    struct Burst
    {
        double startTime = 0;
        float x = 0;
        float y = 0;
        float hue = 0;
        bool active = false;
    };


    bool
    UpdateTriggers(double now)
    {
        bool triggered = false;
        const float trig = trig_.connected() ? trig_(0) : 0;
        const float eventCount =
            eventCount_.connected() ? eventCount_(0) : lastEventCount_;
        const int key = key_.connected() ? static_cast<int>(key_(0)) : -1;

        if(eventCount_.connected())
            triggered = eventCount != lastEventCount_;
        else if(trig_.connected())
            triggered = trig > 0 &&
                        (lastTrig_ <= 0 || key != lastKey_);

        if(triggered)
            AddBurst(now, key);

        lastTrig_ = trig;
        lastEventCount_ = eventCount;
        lastKey_ = key;
        return triggered;
    }


    void
    AddBurst(double now, int note)
    {
        float x = 0;
        float y = 0;
        NoteToPad(note, x, y);

        Burst & burst = bursts_[nextBurst_];
        burst.startTime = now;
        burst.x = x;
        burst.y = y;
        burst.hue = Fraction(0.61803398875f * static_cast<float>(note + 129) +
                             0.11f * static_cast<float>(now));
        burst.active = true;
        nextBurst_ = (nextBurst_ + 1) % burstCount;
    }


    void
    Render(float now)
    {
        const float speed = static_cast<float>(speed_.as_double());
        const float brightness = static_cast<float>(brightness_.as_double());
        const float saturation = static_cast<float>(saturation_.as_double());
        const float sparkleStrength = static_cast<float>(sparkle_.as_double());
        const float reactivity = static_cast<float>(reactivity_.as_double());
        const float time = now * speed;

        for(int row = 0; row < gridSize; ++row)
            for(int column = 0; column < gridSize; ++column)
            {
                const int pad = row * gridSize + column;
                const float x = (static_cast<float>(column) - 3.5f) / 3.5f;
                const float y = (3.5f - static_cast<float>(row)) / 3.5f;
                const float radius = std::sqrt(x * x + y * y);
                const float angle = std::atan2(y, x);

                const float spiral =
                    0.5f + 0.5f * std::sin(10 * radius - 3 * angle - 3.2f * time);
                const float crossWave =
                    0.5f + 0.25f * std::sin(4.5f * x + 1.7f * time) +
                    0.25f * std::cos(4.5f * y - 1.3f * time);
                const float sparkle = sparkleStrength * SmoothPulse(
                    std::sin(6.7f * time + sparklePhase_[pad]), 18);
                const float center =
                    0.5f + 0.5f * std::cos(8 * radius - 2.1f * time);

                const float hue = Fraction(
                    angle / twoPi + 0.095f * time +
                    0.16f * std::sin(4.2f * radius - 1.4f * time) +
                    0.04f * crossWave);
                const float value = brightness * std::clamp(
                    0.14f + 0.37f * spiral + 0.22f * crossWave +
                    0.16f * center + 0.65f * sparkle,
                    0.0f, 1.0f);
                RGB rgb = HSVToRGB(hue, saturation, value);

                for(Burst & burst : bursts_)
                {
                    if(!burst.active)
                        continue;

                    const float age = static_cast<float>(now - burst.startTime);
                    if(age > 2.2f)
                    {
                        burst.active = false;
                        continue;
                    }

                    const float dx = x - burst.x;
                    const float dy = y - burst.y;
                    const float distance = std::sqrt(dx * dx + dy * dy);
                    const float ringRadius = 1.35f * age;
                    const float ringDelta = (distance - ringRadius) / 0.17f;
                    const float ring = std::exp(-ringDelta * ringDelta) *
                                       std::max(0.0f, 1 - age / 2.2f);
                    const float core = std::exp(-7 * distance * distance) *
                                       std::exp(-3.5f * age);
                    const float mix = std::clamp(
                        reactivity * (1.15f * ring + 0.8f * core), 0.0f, 1.0f);
                    const RGB burstRGB = HSVToRGB(
                        burst.hue + 0.12f * age,
                        std::min(1.0f, saturation + 0.08f),
                        brightness);
                    rgb.red += mix * (burstRGB.red - rgb.red);
                    rgb.green += mix * (burstRGB.green - rgb.green);
                    rgb.blue += mix * (burstRGB.blue - rgb.blue);
                }

                color_(pad, 0) = std::clamp(rgb.red, 0.0f, 1.0f);
                color_(pad, 1) = std::clamp(rgb.green, 0.0f, 1.0f);
                color_(pad, 2) = std::clamp(rgb.blue, 0.0f, 1.0f);
            }
    }


    parameter speed_;
    parameter brightness_;
    parameter saturation_;
    parameter sparkle_;
    parameter reactivity_;
    parameter frameRate_;
    matrix key_;
    matrix trig_;
    matrix eventCount_;
    matrix color_;
    std::array<float, padCount> sparklePhase_{};
    std::array<Burst, burstCount> bursts_{};
    int nextBurst_ = 0;
    float lastTrig_ = 0;
    float lastEventCount_ = 0;
    int lastKey_ = -1;
    double nextFrameTime_ = 0;
};

INSTALL_CLASS(LaunchpadAnimation)
