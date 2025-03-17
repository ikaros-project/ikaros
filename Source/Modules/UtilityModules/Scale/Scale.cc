
#include "ikaros.h"

using namespace ikaros;

class Scale : public Module
{
    matrix input;
    matrix scale;
    matrix output;
    parameter factor;

    void Init()
    {
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");
        Bind(factor, "factor");
        Bind(scale, "SCALE");
    }

    void Tick()
    {
        output.copy(input);
        output.scale(factor);
        if (scale.connected())
            output.scale(scale(0));
    }
};

INSTALL_CLASS(Scale)
