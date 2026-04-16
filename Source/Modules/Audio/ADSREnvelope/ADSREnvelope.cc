#include "ikaros.h"

#include <algorithm>

using namespace ikaros;

class ADSREnvelope: public Module
{
    enum EnvelopeState
    {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

    parameter   sample_rate;
    parameter   attack;
    parameter   decay;
    parameter   sustain;
    parameter   release;

    matrix      gate;
    matrix      trig;
    matrix      output;

    EnvelopeState state = Idle;
    double      level = 0.0;
    double      release_start = 0.0;
    bool        previous_gate_high = false;

    void Init()
    {
        Bind(sample_rate, "sample_rate");
        Bind(attack, "attack");
        Bind(decay, "decay");
        Bind(sustain, "sustain");
        Bind(release, "release");
        Bind(gate, "GATE");
        Bind(trig, "TRIG");
        Bind(output, "OUTPUT");

        state = Idle;
        level = 0.0;
        release_start = 0.0;
        previous_gate_high = false;
    }

    double EffectiveSampleRate() const
    {
        double sr = sample_rate;
        return sr > 0.0 ? sr : 1.0 / GetTickDuration();
    }

    double ReadSample(const matrix & m, int index) const
    {
        if(!m.connected() || m.size() == 0)
            return 0.0;
        if(m.rank() == 1 && index < m.size_x())
            return m(index);
        return m(0);
    }

    bool ReadBoolSample(const matrix & m, int index) const
    {
        return ReadSample(m, index) > 0.0;
    }

    void StartAttack()
    {
        state = Attack;
    }

    void StartRelease()
    {
        release_start = level;
        state = Release;
    }

    void Tick()
    {
        double sr = EffectiveSampleRate();
        double dt = 1.0 / sr;

        double a = std::max(attack.as_double(), 0.0);
        double d = std::max(decay.as_double(), 0.0);
        double s = std::clamp(sustain.as_double(), 0.0, 1.0);
        double r = std::max(release.as_double(), 0.0);

        if(output.rank() != 1)
            return;

        for(int i = 0; i < output.size_x(); ++i)
        {
            bool gate_high = ReadBoolSample(gate, i);
            bool trig_high = ReadBoolSample(trig, i);

            if(trig_high || (gate_high && !previous_gate_high))
                StartAttack();
            else if(!gate_high && previous_gate_high)
                StartRelease();

            switch(state)
            {
                case Idle:
                    level = 0.0;
                    break;

                case Attack:
                    if(a == 0.0)
                    {
                        level = 1.0;
                        state = Decay;
                    }
                    else
                    {
                        level += dt / a;
                        if(level >= 1.0)
                        {
                            level = 1.0;
                            state = Decay;
                        }
                    }
                    break;

                case Decay:
                    if(d == 0.0)
                    {
                        level = s;
                        if(gate_high)
                            state = Sustain;
                        else
                            StartRelease();
                    }
                    else
                    {
                        level -= dt * (1.0 - s) / d;
                        if(level <= s)
                        {
                            level = s;
                            if(gate_high)
                                state = Sustain;
                            else
                                StartRelease();
                        }
                    }
                    break;

                case Sustain:
                    level = s;
                    if(!gate_high)
                        StartRelease();
                    break;

                case Release:
                    if(r == 0.0)
                    {
                        level = 0.0;
                        state = Idle;
                    }
                    else
                    {
                        level -= dt * release_start / r;
                        if(level <= 0.0)
                        {
                            level = 0.0;
                            state = Idle;
                        }
                    }
                    break;
            }

            output(i) = level;
            previous_gate_high = gate_high;
        }
    }
};

INSTALL_CLASS(ADSREnvelope)
