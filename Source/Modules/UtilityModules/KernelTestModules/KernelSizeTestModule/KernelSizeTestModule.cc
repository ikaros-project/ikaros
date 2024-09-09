#include "ikaros.h"

using namespace ikaros;

class KernelSizeTestModule: public Module
{
    matrix output;

    void Init()
    {
        Bind(output, "OUTPUT");    
    }


    void Tick()
    {
        output.print("OUTPUT"); 
    }
};

INSTALL_CLASS(KernelSizeTestModule)

