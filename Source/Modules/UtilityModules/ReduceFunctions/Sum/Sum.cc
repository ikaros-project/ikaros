#include "ikaros.h"

using namespace ikaros;

class Sum: public Module
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
        output = input.sum();
    }
};


INSTALL_CLASS(Sum)

