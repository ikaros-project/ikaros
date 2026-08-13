#include "ikaros.h"

using namespace ikaros;

class ArgMax: public Module
{
    matrix input;
    matrix output;
    matrix value;

    void Init()
    {
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");
        Bind(value, "VALUE");

        if(output.size() != 2 || value.size() != 1)
            throw exception("ArgMax: OUTPUT must have size 2 and VALUE must have size 1.", path_);

        if(input.rank() > 2)
            throw exception("ArgMax: INPUT must be a scalar, vector, or matrix.", path_);
    }

    void Tick()
    {
        if(input.size() == 0)
            throw exception("ArgMax: INPUT must not be empty.", path_);

        int maxIndex = 0;
        float maxValue = input.data()[0];

        for(int i = 1; i < input.size(); ++i)
        {
            if(input.data()[i] > maxValue)
            {
                maxValue = input.data()[i];
                maxIndex = i;
            }
        }

        if(input.rank() == 2)
        {
            const int cols = input.cols();
            output(0) = static_cast<float>(maxIndex % cols);
            output(1) = static_cast<float>(maxIndex / cols);
        }
        else
        {
            output(0) = static_cast<float>(maxIndex);
            output(1) = 0.0f;
        }

        value(0) = maxValue;
    }
};

INSTALL_CLASS(ArgMax)
