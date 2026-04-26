
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
        Bind(scale, "SCALE");
        Bind(output, "OUTPUT");
        Bind(factor, "factor");
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
