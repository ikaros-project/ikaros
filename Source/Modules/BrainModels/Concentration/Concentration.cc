#include "ikaros.h"

using namespace ikaros;

class Concentration: public Module
{

    parameter   alpha;      // input gain
    parameter   beta;       // decay rate
    parameter   gamma;      // nominal level

    matrix      input;      // increase output
    matrix      output;     // state output

    void Init()
    {
        Bind(alpha, "alpha");
        Bind(beta, "beta");
        Bind(gamma, "gamma");
    
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");

        output(0) = gamma;
    }


    void Tick()
    {
        float & output_value = output.scalar();
        output_value += alpha * input.sum() - beta * (output_value - gamma);

        if(output_value < gamma)
            output_value = gamma;
    }
};

INSTALL_CLASS(Concentration)
