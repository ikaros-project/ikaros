#include "ikaros.h"

using namespace ikaros;

class Square: public Module
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
        output.apply([](float x) { return x * x; });
    }
};

INSTALL_CLASS(Square)
