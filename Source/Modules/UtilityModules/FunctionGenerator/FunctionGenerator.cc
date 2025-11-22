#include "ikaros.h"
#include <cmath>

using namespace ikaros;

class FunctionGenerator : public Module
{
public:
    parameter type;
    parameter offset;
    parameter amplitude;
    parameter frequency;
    parameter shift;
    parameter duty;
    parameter basetime;
    parameter tickduty;

    matrix output;

    void Init()
    {
        Bind(type, "type");
        Bind(offset, "offset");
        Bind(amplitude, "amplitude");
        Bind(frequency, "frequency");
        Bind(shift, "shift");
        Bind(duty, "duty");
        Bind(basetime, "basetime");
        Bind(tickduty, "tickduty");

        Bind(output, "OUTPUT");
    }

    static float triangle(double t)
    {
        float v = 2 * (t / (2 * M_PI) - trunc(t / (2 * M_PI)));
        return (v < 1.0) ? v : 2 - v;
    }

    static float ramp(double t)
    {
        return t / (2 * M_PI) - trunc(t / (2 * M_PI));
    }

    static float square(double t, double d)
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

        double time = kernel().GetTime();

        switch (type.as_int())
        {
            case 0:
                output = offset + amplitude * sin(frequency * (time + shift));
                break;
            case 1:
                output = offset + amplitude * triangle(frequency * (time + shift));
                break;
            case 2:
                output = offset + amplitude * ramp(frequency * (time + shift));
                break;
            case 3:
                output = offset + amplitude * square(frequency * (time + shift), duty);
                break;
            case 4:
                output = offset + amplitude * ticksquare(time, basetime, tickduty);
                break;
            default:
                output = 0;
        }
    }
};

INSTALL_CLASS(FunctionGenerator);