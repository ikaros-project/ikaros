#include "ikaros.h"

using namespace ikaros;

class Add: public Module
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
        output.add(input1, input2);
    }
};


INSTALL_CLASS(Add)

