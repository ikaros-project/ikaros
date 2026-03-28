#include "ikaros.h"

using namespace ikaros;

class Abs: public Module
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
        output.apply([](float x) { return std::abs(x); });
    }
};

INSTALL_CLASS(Abs)
