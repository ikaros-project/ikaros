#include "ikaros.h"

#include <algorithm>
#include <cmath>

using namespace ikaros;

class AudioFilter: public Module
{
    parameter   sample_rate;
    parameter   cutoff;
    parameter   resonance;
    parameter   envelope_amount;

    matrix      input;
    matrix      cutoff_input;
    matrix      cutoff_mod;
    matrix      resonance_mod;
    matrix      output;

    double      z1 = 0.0;
    double      z2 = 0.0;
    double      z3 = 0.0;
    double      z4 = 0.0;

    void ResetState()
    {
        z1 = z2 = z3 = z4 = 0.0;
    }

    bool StateIsFinite() const
    {
        return std::isfinite(z1) && std::isfinite(z2) && std::isfinite(z3) && std::isfinite(z4);
    }

    void Init()
    {
        Bind(sample_rate, "sample_rate");
        Bind(cutoff, "cutoff");
        Bind(resonance, "resonance");
        Bind(envelope_amount, "envelope_amount");

        Bind(input, "INPUT");
        Bind(cutoff_input, "CUTOFF");
        Bind(cutoff_mod, "CUTOFF_MOD");
        Bind(resonance_mod, "RESONANCE_MOD");
        Bind(output, "OUTPUT");

        ResetState();
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

    void Tick()
    {
        if(output.rank() != 1)
            return;

        if(!StateIsFinite())
            ResetState();

        const double sr = EffectiveSampleRate();
        const double min_cutoff = 20.0;
        const double max_cutoff = 0.45 * sr;
        const double max_feedback = 1.2;
        const double max_state = 1.0e6;

        for(int i = 0; i < output.size_x(); ++i)
        {
            const double x_in = ReadSample(input, i, 0.0);
            const double base_cutoff = ReadSample(cutoff_input, i, cutoff.as_double());
            const double env = ReadSample(cutoff_mod, i, 0.0);
            const double cutoff_hz = std::clamp(base_cutoff * std::pow(2.0, envelope_amount.as_double() * env), min_cutoff, max_cutoff);
            const double g = 1.0 - std::exp(-2.0 * M_PI * cutoff_hz / sr);

            const double resonant_feedback = std::clamp(resonance.as_double() + ReadSample(resonance_mod, i, 0.0), 0.0, max_feedback);
            const double x = x_in - resonant_feedback * z4;

            z1 += g * (x - z1);
            z2 += g * (z1 - z2);
            z3 += g * (z2 - z3);
            z4 += g * (z3 - z4);

            if(!StateIsFinite()
            || std::abs(z1) > max_state
            || std::abs(z2) > max_state
            || std::abs(z3) > max_state
            || std::abs(z4) > max_state)
            {
                ResetState();
                output(i) = 0.0;
                continue;
            }

            output(i) = z4;
        }
    }
};

INSTALL_CLASS(AudioFilter)
