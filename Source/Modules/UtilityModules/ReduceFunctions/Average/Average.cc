#include "ikaros.h"

using namespace ikaros;

class Average: public Module
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
        output = input.average();
    }
};


INSTALL_CLASS(Average)

