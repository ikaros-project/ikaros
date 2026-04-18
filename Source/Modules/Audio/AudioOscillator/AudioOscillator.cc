#include "ikaros.h"

#include <math.h>

using namespace ikaros;

class AudioOscillator: public Module
{
    parameter   wave_shape;
    parameter   frequency;
    parameter   sample_rate;
    parameter   multiplier;
    parameter   detune;
    parameter   volume;
    parameter   modulation_gain;
    parameter   duty_cycle;
    matrix      input;
    matrix      modulation;
    matrix      output;
    double      phase = 0.0;
    bool        warned_buffer_size = false;

    void WarnIfBufferGeometryIsFractional(double sr)
    {
        if(warned_buffer_size || sr <= 0.0)
            return;

        const double exact_samples = sr * GetTickDuration();
        const double rounded_samples = std::round(exact_samples);
        if(std::abs(exact_samples - rounded_samples) > 1e-6)
        {
            Warning("AudioOscillator sample_rate * tick_duration is not an integer number of samples (" + std::to_string(exact_samples) + "). Audio buffers may not match timing exactly.");
            warned_buffer_size = true;
        }
    }

    void Init()
    {
        Bind(wave_shape, "wave_shape");
        Bind(frequency, "frequency");
        Bind(sample_rate, "sample_rate");
        Bind(multiplier, "multiplier"); 
        Bind(detune, "detune");
        Bind(volume, "volume");
        Bind(modulation_gain, "modulation_gain");
        Bind(duty_cycle, "duty_cycle");
        Bind(input, "INPUT");
        Bind(modulation, "MODULATION");
        Bind(output, "OUTPUT");

        phase = 0.0;
    }


    float func(double phase_position) const
    {
        double wrapped_phase = phase_position - floor(phase_position);
        double duty = std::clamp(duty_cycle.as_double(), 0.0, 1.0);

        switch(wave_shape.as_int())
        {
            case 0: return sin(2 * M_PI * wrapped_phase);
            case 1: return sin(2 * M_PI * wrapped_phase) > 0 ? 1 : -1;
            case 2: return static_cast<float>(2.0 * wrapped_phase - 1.0);                    // saw
            case 3: return static_cast<float>(4.0 * fabs(wrapped_phase - 0.5) - 1.0);        // triangle
            case 4: return static_cast<float>(1.0 - 2.0 * wrapped_phase);                    // ramp
            case 5: return wrapped_phase < duty ? 1.0f : -1.0f;                               // pulse
            default: return 0;
        }
    }

    void Tick()
    {
        double sr = sample_rate;
        if(sr <= 0)
            sr = 1.0 / GetTickDuration();

        WarnIfBufferGeometryIsFractional(sr);

        if(output.rank() != 1)
            return;

        auto read_sample = [](const matrix & m, int index) -> double
        {
            if(!m.connected() || m.size() == 0)
                return 0.0;
            if(m.rank() == 1 && index < m.size_x())
                return m(index);
            return m(0);
        };

        for(int i = 0; i < output.size_x(); ++i)
        {
            double base_frequency = input.connected() ? read_sample(input, i) : static_cast<double>(frequency);
            double mod = modulation.connected() ? modulation_gain.as_double() * read_sample(modulation, i) : 0.0;
            double instant_frequency = base_frequency * multiplier.as_double() + detune.as_double() + mod;
            if(instant_frequency < 0.0)
                instant_frequency = 0.0;

            output(i) = volume.as_double() * func(phase);
            phase += instant_frequency / sr;
            phase -= floor(phase);
        }
    }
};

INSTALL_CLASS(AudioOscillator)
