#include "ikaros.h"

using namespace ikaros;

class Invert: public Module
{
    matrix input;
    matrix output;

    void Init()
    {
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");
    }

    void Tick()
    {
        output.copy(input);
        output.apply([](float x) { return 1.0f / x; });
    }
};

INSTALL_CLASS(Invert)
