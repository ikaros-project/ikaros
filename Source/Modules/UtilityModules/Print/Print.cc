#include "ikaros.h"

using namespace ikaros;

class Print: public Module
{
    matrix input;

    void Init()
    {
        Bind(input, "INPUT");
    }

    void Tick()
    {
        input.print();
    }
};

INSTALL_CLASS(Print)

