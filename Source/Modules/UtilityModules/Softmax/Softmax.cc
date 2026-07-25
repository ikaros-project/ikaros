#include <cmath>
#include <limits>

#include "ikaros.h"

using namespace ikaros;

class Softmax: public Module
{
    matrix input;
    matrix output;
    bool warnedAboutNaN = false;

    void Init()
    {
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");
    }

    void Tick()
    {
        if(input.empty())
            return;

        bool containsNaN = false;
        int positiveInfinityCount = 0;
        for(int index = 0; index < input.size(); ++index)
        {
            const float value = input(index);
            containsNaN = containsNaN || std::isnan(value);
            positiveInfinityCount +=
                std::isinf(value) && value > 0.0f ? 1 : 0;
        }

        if(containsNaN)
        {
            output.set(std::numeric_limits<float>::quiet_NaN());
            if(!warnedAboutNaN)
            {
                Warning(
                    "Softmax input contains NaN; output is NaN.",
                    path_);
                warnedAboutNaN = true;
            }
            return;
        }
        warnedAboutNaN = false;

        if(positiveInfinityCount > 0)
        {
            output.copy(input);
            const float probability =
                1.0f / static_cast<float>(positiveInfinityCount);
            output.apply([probability](float value) {
                return std::isinf(value) && value > 0.0f
                    ? probability : 0.0f;
            });
            return;
        }

        const float maximum = input.max();
        if(maximum == -std::numeric_limits<float>::infinity())
        {
            output.set(1.0f / static_cast<float>(input.size()));
            return;
        }

        output.copy(input);
        output.apply(
            [maximum](float value) { return std::exp(value - maximum); });

        const float sum = output.sum();
        output.scale(1.0f / sum);
    }
};

INSTALL_CLASS(Softmax)
