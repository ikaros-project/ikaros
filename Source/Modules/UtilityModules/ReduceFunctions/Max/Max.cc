#include "ikaros.h"

using namespace ikaros;

class Max: public Module
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
    
        output = input.max();
    }
};


INSTALL_CLASS(Max)

