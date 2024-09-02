#include "ikaros.h"

using namespace ikaros;

class Min: public Module
{
    matrix      input;
    matrix      output;

    void Init()
    {
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");
    }


    void Tick()
    {
        output = input.min();
    }
};


INSTALL_CLASS(Min)

