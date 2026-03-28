#include "ikaros.h"

using namespace ikaros;

class Clip: public Module
{
    matrix input;
    matrix output;
    parameter a;
    parameter b;

    void Init()
    {
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");
        Bind(a, "a");
        Bind(b, "b");
    }

    void Tick()
    {
        float lo = std::min(float(a), float(b));
        float hi = std::max(float(a), float(b));

        output.copy(input);
        output.apply([lo, hi](float x) { return std::clamp(x, lo, hi); });
    }
};

INSTALL_CLASS(Clip)
