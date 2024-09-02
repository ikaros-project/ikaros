#include "ikaros.h"

using namespace ikaros;

class Max: public Module
{
    matrix      input1;
    matrix      input2;
    matrix      output;

    void Init()
    {
        Bind(input1, "INPUT1");
        Bind(input2, "INPUT2");
        Bind(output, "OUTPUT");
    }


    void Tick()
    {

        input1.print();
        input2.print();

        output.max();
    }
};


INSTALL_CLASS(Max)

