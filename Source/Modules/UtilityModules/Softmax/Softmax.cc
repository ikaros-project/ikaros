#include <cmath>

#include "ikaros.h"

using namespace ikaros;

class Softmax: public Module
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
        const float maximum = input.max();
        output.copy(input);
        output.apply(
            [maximum](float value) { return std::exp(value - maximum); });

        const float sum = output.sum();
        output.scale(1.0f / sum);
    }
};

INSTALL_CLASS(Softmax)
