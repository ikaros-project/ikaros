#include "ikaros.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace ikaros;

class AudioDelay: public Module
{
    parameter   sample_rate;
    parameter   delay_time;
    parameter   feedback_gain;
    parameter   mix;
    parameter   lowpass_cutoff;

    matrix      input;
    matrix      output;

    std::vector<float> delay_buffer;
    size_t      write_index = 0;
    double      filtered_delay_state = 0.0;

    void Init()
    {
        Bind(sample_rate, "sample_rate");
        Bind(delay_time, "delay_time");
        Bind(feedback_gain, "feedback_gain");
        Bind(mix, "mix");
        Bind(lowpass_cutoff, "lowpass_cutoff");
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");

        delay_buffer.clear();
        write_index = 0;
        filtered_delay_state = 0.0;
    }

    double EffectiveSampleRate() const
    {
        double sr = sample_rate;
        return sr > 0.0 ? sr : 1.0 / GetTickDuration();
    }

    double ReadSample(const matrix & m, int index, double fallback = 0.0) const
    {
        if(!m.connected() || m.size() == 0)
            return fallback;
        if(m.rank() == 1 && index < m.size_x())
            return m(index);
        return m(0);
    }

    void EnsureBufferSize(size_t required_size)
    {
        if(required_size == 0)
            required_size = 1;

        if(delay_buffer.size() == required_size)
            return;

        delay_buffer.assign(required_size, 0.0f);
        write_index = 0;
        filtered_delay_state = 0.0;
    }

    void Tick()
    {
        if(output.rank() != 1)
            return;

        const double sr = EffectiveSampleRate();
        const double clamped_delay_time = std::max(delay_time.as_double(), 0.0);
        const size_t delay_samples = static_cast<size_t>(std::max(1.0, std::round(clamped_delay_time * sr)));
        const double feedback = std::clamp(feedback_gain.as_double(), -0.999, 0.999);
        const double wet_mix = std::clamp(mix.as_double(), 0.0, 1.0);
        const double dry_mix = 1.0 - wet_mix;
        const double min_cutoff = 20.0;
        const double max_cutoff = 0.45 * sr;
        const double cutoff_hz = std::clamp(lowpass_cutoff.as_double(), min_cutoff, max_cutoff);
        const double lowpass_g = 1.0 - std::exp(-2.0 * M_PI * cutoff_hz / sr);

        EnsureBufferSize(delay_samples);

        for(int i = 0; i < output.size_x(); ++i)
        {
            const float delayed_sample = delay_buffer[write_index];
            const float input_sample = static_cast<float>(ReadSample(input, i, 0.0));
            filtered_delay_state += lowpass_g * (static_cast<double>(delayed_sample) - filtered_delay_state);
            const float filtered_delay = static_cast<float>(filtered_delay_state);
            const float write_sample = static_cast<float>(input_sample + feedback * filtered_delay);

            output(i) = static_cast<float>(dry_mix * input_sample + wet_mix * filtered_delay);
            delay_buffer[write_index] = write_sample;

            write_index += 1;
            if(write_index >= delay_buffer.size())
                write_index = 0;
        }
    }
};

INSTALL_CLASS(AudioDelay)
