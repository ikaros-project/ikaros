#include "ikaros.h"

#include <math.h>

using namespace ikaros;

class Oscillator: public Module
{
    parameter   osc_type;
    matrix      frequency;
    matrix      output;

    void Init()
    {
        Bind(osc_type, "type");
        Bind(frequency, "frequency");
        Bind(output, "OUTPUT");
    }



    float func(float time, float freq)
    {
        switch(osc_type.as_int())
        {
            case 0: return sin(2*M_PI*time*freq);
            case 1: return sin(2*M_PI*time*freq) > 0 ? 1 : 0;
            default: return 0;
        }
    }

    void Tick()
    {
        float time = kernel().GetTime();
        output.apply(frequency, [=](float x, float f) {return func(time, f);});
    }
};

INSTALL_CLASS(Oscillator)

