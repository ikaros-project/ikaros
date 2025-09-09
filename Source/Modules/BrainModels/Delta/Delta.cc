#include "ikaros.h"

#include <math.h>

using namespace ikaros;

class Delta: public Module
{
    parameter decay_type;
    parameter decay_rate;
    parameter excitation_gain;
    parameter inhibition_gain;

    matrix excitation;
    matrix inhibition;
    matrix output;

    double      x = 0;

    void Init()
    {
        Bind(decay_type, "decay_type");
        Bind(excitation_gain, "excitation_gain");
        Bind(inhibition_gain, "inhibition_gain");

        Bind(excitation, "EXCITATION");
        Bind(inhibition, "INHIBITION");
        Bind(output, "OUTPUT");
    }


    void Tick()
    {
        x += excitation_gain * excitation.sum() - inhibition_gain *inhibition.sum();

        switch(decay_type.as_int())
        {
            case 0: // none
                break;

            case 1: // linear
                x -= decay_rate;
                break;

            case 2: // exponential
                x *= (1-decay_rate);
                break;

            default:
                break;
        }

        if(x  < 0)
            x = 0;

        output = x;
    }
};

INSTALL_CLASS(Delta)

