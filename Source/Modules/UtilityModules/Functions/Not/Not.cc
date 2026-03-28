#include "ikaros.h"

using namespace ikaros;

class Not: public Module
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
        output.apply([](float x) { return x == 0.0f ? 1.0f : 0.0f; });
    }
};

INSTALL_CLASS(Not)
