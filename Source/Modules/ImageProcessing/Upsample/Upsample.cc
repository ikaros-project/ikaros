#include "ikaros.h"

using namespace ikaros;

class Upsample : public Module 
{
    matrix input;
    matrix output;

    void Init() 
    {
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");
    }

    void Tick()
    {
        if (input.rank() == 2) 
            output.upsample(input);
        else
            for (int i = 0; i < input.size_z(); i++)
                output[i].upsample(input[i]);
    }
};

INSTALL_CLASS(Upsample)
