#include "ikaros.h"

using namespace ikaros;

class Threshold: public Module
{
    parameter bypass;
    parameter threshold;
    parameter type;

    matrix input;
    matrix output;

    void Init()
    {
        Bind(bypass, "bypass");
        Bind(threshold, "threshold");
        Bind(type, "type");

        Bind(input, "INPUT");
        Bind(output, "OUTPUT");

        if(input.shape() != output.shape())
            throw exception("Threshold: OUTPUT shape must match INPUT shape.", path_);
    }

    void Tick()
    {
        if(bypass)
        {
            output.copy(input);
            return;
        }

        const float t = threshold.as_float();
        output.copy(input);

        switch(type.as_int())
        {
            case 0:
                output.apply([t](float x) { return x > t ? 1.0f : 0.0f; });
                break;

            case 1:
                output.apply([t](float x) { return x > t ? x - t : 0.0f; });
                break;

            default:
                throw exception("Threshold: type must be binary or linear.", path_);
        }
    }
};

INSTALL_CLASS(Threshold)
