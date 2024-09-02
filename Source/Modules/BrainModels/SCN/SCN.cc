#include "ikaros.h"

#include <math.h>

using namespace ikaros;

class SCN: public Module
{
    parameter   frequency;
    matrix      input;
    matrix      output;
    matrix      diff;

    double      time_shift;

    void Init()
    {
        Bind(frequency, "frequency");
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");
        Bind(diff, "DIFF");
    }


    void Tick()
    {
        float time = kernel().GetTime();
        output = sin(2*M_PI*(time+time_shift)*frequency);


        if(input.empty())
            return;

        float i = input;
        float o = output;
        diff = o-i;

        time_shift += +0.0001 * diff;
    }
};

INSTALL_CLASS(SCN)

