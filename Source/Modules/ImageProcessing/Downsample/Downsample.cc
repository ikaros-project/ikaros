#include "ikaros.h"

using namespace ikaros;

class Downsample : public Module 
{
    matrix input;
    matrix output;
    matrix temporaryRow;

    void Init() 
    {
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");
    }

    void Tick()
    {
        if (input.rank() == 2) 
            output.downsample(input, temporaryRow);
        else
            for (int i = 0; i < input.size_z(); i++)
                output[i].downsample(input[i], temporaryRow);
    }
};

INSTALL_CLASS(Downsample)
