#include "ikaros.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

using namespace ikaros;

class AudioReverb: public Module
{
    parameter   sample_rate;
    parameter   mix;
    parameter   decay;
    parameter   damping;
    parameter   pre_delay;

    matrix      input;
    matrix      output;

    struct DelayLine
    {
        std::vector<float> buffer;
        size_t index = 0;

        void resize(size_t size)
        {
            if(size == 0)
                size = 1;
            buffer.assign(size, 0.0f);
            index = 0;
        }

        float read() const
        {
            return buffer.empty() ? 0.0f : buffer[index];
        }

        void write(float sample)
        {
            if(buffer.empty())
                return;
            buffer[index] = sample;
            index += 1;
            if(index >= buffer.size())
                index = 0;
        }
    };

    static constexpr std::array<double, 4> comb_times = {0.0297, 0.0371, 0.0411, 0.0437};
    static constexpr std::array<double, 2> allpass_times = {0.0050, 0.0017};

    DelayLine   predelay_line;
    std::array<DelayLine, 4> comb_lines;
    std::array<double, 4> comb_filters = {0.0, 0.0, 0.0, 0.0};
    std::array<DelayLine, 2> allpass_lines;
    bool        warned_buffer_size = false;

    double EffectiveSampleRate() const
    {
        double sr = sample_rate;
        return sr > 0.0 ? sr : 1.0 / GetTickDuration();
    }

    void WarnIfBufferGeometryIsFractional(double sr)
    {
        if(warned_buffer_size || sr <= 0.0)
            return;

        const double exact_samples = sr * GetTickDuration();
        const double rounded_samples = std::round(exact_samples);
        if(std::abs(exact_samples - rounded_samples) > 1e-6)
        {
            Warning("AudioReverb sample_rate * tick_duration is not an integer number of samples (" + std::to_string(exact_samples) + "). Audio buffers may not match timing exactly.");
            warned_buffer_size = true;
        }
    }

    double ReadSample(const matrix & m, int index, double fallback = 0.0) const
    {
        if(!m.connected() || m.size() == 0)
            return fallback;
        if(m.rank() == 1 && index < m.size_x())
            return m(index);
        return m(0);
    }

    static size_t SamplesFromSeconds(double seconds, double sr)
    {
        return static_cast<size_t>(std::max(1.0, std::round(std::max(seconds, 0.0) * sr)));
    }

    void ResetState()
    {
        predelay_line.resize(predelay_line.buffer.size());
        for(size_t i = 0; i < comb_lines.size(); ++i)
        {
            comb_lines[i].resize(comb_lines[i].buffer.size());
            comb_filters[i] = 0.0;
        }
        for(auto & line : allpass_lines)
            line.resize(line.buffer.size());
    }

    void ConfigureLines(double sr)
    {
        predelay_line.resize(SamplesFromSeconds(pre_delay.as_double(), sr));

        for(size_t i = 0; i < comb_lines.size(); ++i)
            comb_lines[i].resize(SamplesFromSeconds(comb_times[i], sr));

        for(size_t i = 0; i < allpass_lines.size(); ++i)
            allpass_lines[i].resize(SamplesFromSeconds(allpass_times[i], sr));

        std::fill(comb_filters.begin(), comb_filters.end(), 0.0);
    }

    void Init()
    {
        Bind(sample_rate, "sample_rate");
        Bind(mix, "mix");
        Bind(decay, "decay");
        Bind(damping, "damping");
        Bind(pre_delay, "pre_delay");
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");

        ConfigureLines(EffectiveSampleRate());
    }

    void Tick()
    {
        if(output.rank() != 1)
            return;

        const double sr = EffectiveSampleRate();
        WarnIfBufferGeometryIsFractional(sr);
        const size_t wanted_predelay = SamplesFromSeconds(pre_delay.as_double(), sr);
        if(predelay_line.buffer.size() != wanted_predelay)
            predelay_line.resize(wanted_predelay);

        const double wet_mix = std::clamp(mix.as_double(), 0.0, 1.0);
        const double dry_mix = 1.0 - wet_mix;
        const double room_decay = std::clamp(decay.as_double(), 0.0, 0.98);
        const double damp = std::clamp(damping.as_double(), 0.0, 1.0);
        const double allpass_feedback = 0.5;

        for(int i = 0; i < output.size_x(); ++i)
        {
            const float input_sample = static_cast<float>(ReadSample(input, i, 0.0));

            const float pre_delayed = predelay_line.read();
            predelay_line.write(input_sample);

            float comb_sum = 0.0f;
            for(size_t c = 0; c < comb_lines.size(); ++c)
            {
                const float delayed = comb_lines[c].read();
                comb_filters[c] += damp * (static_cast<double>(delayed) - comb_filters[c]);
                const float filtered = static_cast<float>(comb_filters[c]);
                comb_lines[c].write(pre_delayed + static_cast<float>(room_decay) * filtered);
                comb_sum += delayed;
            }

            float reverb_sample = comb_sum * 0.25f;
            for(auto & allpass : allpass_lines)
            {
                const float delayed = allpass.read();
                const float y = -allpass_feedback * reverb_sample + delayed;
                const float write_sample = reverb_sample + allpass_feedback * delayed;
                allpass.write(write_sample);
                reverb_sample = y;
            }

            output(i) = static_cast<float>(dry_mix * input_sample + wet_mix * reverb_sample);
        }
    }
};

INSTALL_CLASS(AudioReverb)
