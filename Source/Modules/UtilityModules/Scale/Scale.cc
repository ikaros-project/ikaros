
#include "ikaros.h"

using namespace ikaros;

class Scale: public Module
{
    matrix      input;
    matrix      output;
    parameter   factor;

    void Init()
    {
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");
        Bind(factor, "factor");
    }


    void Tick()
    {
        output.copy(input);
        output.scale(factor);
    }
};


INSTALL_CLASS(Scale)

