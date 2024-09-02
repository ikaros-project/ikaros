#include "ikaros.h"
#include <cmath>

using namespace ikaros;

class FunctionGenerator : public Module
{
public:
    parameter size;
    parameter type;
    parameter offset;
    parameter amplitude;
    parameter frequency;
    parameter shift;
    parameter duty;
    parameter basetime;
    parameter tickduty;
    parameter sample_rate;

    int t;
    matrix output;

    void Init()
    {
        Bind(size, "size");
        Bind(type, "type");
        Bind(offset, "offset");
        Bind(amplitude, "amplitude");
        Bind(frequency, "frequency");
        Bind(shift, "shift");
        Bind(duty, "duty");
        Bind(basetime, "basetime");
        Bind(tickduty, "tickduty");
        Bind(sample_rate, "sample_rate");

        Bind(output, "OUTPUT");

        t = 0;
    }

    static float triangle(float t)
    {
        float v = 2 * (t / (2 * M_PI) - trunc(t / (2 * M_PI)));
        return (v < 1.0) ? v : 2 - v;
    }

    static float ramp(float t)
    {
        return t / (2 * M_PI) - trunc(t / (2 * M_PI));
    }

    static float square(float t, float d)
    {
        float v = t / (2 * M_PI) - trunc(t / (2 * M_PI));
        return (v < d) ? 1 : 0;
    }

    static float ticksquare(int tick, int basetime, int duty)
    {
        return ((tick % basetime) < duty) ? 1 : 0;
    }

    void Tick()
    {
        for (int i = 0; i < size; i++)
        {
            float time = (float(t) + i) / sample_rate;
            switch (type.as_int())
            {
                case 0:
                    output[i] = offset + amplitude * sin(frequency * (time + shift));
                    break;
                case 1:
                    output[i] = offset + amplitude * triangle(frequency * (time + shift));
                    break;
                case 2:
                    output[i] = offset + amplitude * ramp(frequency * (time + shift));
                    break;
                case 3:
                    output[i] = offset + amplitude * square(frequency * (time + shift), duty);
                    break;
                case 4:
                    output[i] = offset + amplitude * ticksquare(t + i, basetime, tickduty);
                    break;
                default:
                    output[i] = 0;
            }
        }
        t += size;
    }
};

INSTALL_CLASS(FunctionGenerator);