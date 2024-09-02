#include "ikaros.h"

using namespace ikaros;

class Median: public Module
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
        output = input.median();
    }
};


INSTALL_CLASS(Median)

