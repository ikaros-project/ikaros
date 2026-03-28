#include "ikaros.h"

using namespace ikaros;

class Exp: public Module
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
        output.apply([](float x) { return std::exp(x); });
    }
};

INSTALL_CLASS(Exp)
