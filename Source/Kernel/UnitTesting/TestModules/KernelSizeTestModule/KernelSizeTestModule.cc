#include "ikaros.h"

using namespace ikaros;

class KernelSizeTestModule: public Module
{
    matrix input;
    matrix output;

    void Init()
    {
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");

        Trace("Init-Trace");
        Print("Init-Print");
        Warning("Init-Warning");
    }


    void Tick()
    {
        output.print("OUTPUT");

        Trace("Tick-Trace");
        Print("Tick-Print");
        Warning("Tick-Warning");

        double x = input;
        double y = input.last();

        std::cout << "DIFF: " << x << " " << y << std::endl;
    }
};

INSTALL_CLASS(KernelSizeTestModule)

