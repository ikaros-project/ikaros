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
        output += alpha * input.sum() - beta * (output - gamma);

        if(output(0) < gamma)
            output(0) = gamma;
    }
};

INSTALL_CLASS(Concentration)

