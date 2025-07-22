#include "ikaros.h"

using namespace ikaros;

class Downsample : public Module 
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
            output.downsample(input);
        else
            for (int i = 0; i < input.size_z(); i++)
                output[i].downsample(input[i]);
    }
};

INSTALL_CLASS(Downsample)

