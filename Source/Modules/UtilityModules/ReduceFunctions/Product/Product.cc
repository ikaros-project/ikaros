#include "ikaros.h"

using namespace ikaros;

class Product: public Module
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
        output = input.product();
    }
};


INSTALL_CLASS(Product)

