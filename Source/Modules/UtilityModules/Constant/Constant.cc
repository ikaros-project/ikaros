#include "ikaros.h"

using namespace ikaros;

class Constant: public Module
{
    matrix data;
    matrix output;

    void Init()
    {
        Bind(data, "data");
        Bind(output, "OUTPUT");
    }


    void Tick()
    {
        output.copy(data);
    }
};

INSTALL_CLASS(Constant)
