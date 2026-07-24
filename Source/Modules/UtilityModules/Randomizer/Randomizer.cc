#include <algorithm>
#include <cmath>
#include <random>

#include "ikaros.h"

using namespace ikaros;

class Randomizer: public Module
{
    parameter min_;
    parameter max_;
    parameter randomSeed;
    matrix output;
    std::mt19937 randomGenerator;
    std::uniform_real_distribution<float> uniformDistribution;
    float uniformMin = 0.0f;
    float uniformMax = 0.0f;
    bool warnedAboutBounds = false;

    bool UpdateBounds()
    {
        const float first = min_.as_float();
        const float second = max_.as_float();
        if(!std::isfinite(first) || !std::isfinite(second))
            return false;

        const float lo = std::min(first, second);
        const float hi = std::max(first, second);
        if(!std::isfinite(hi - lo))
            return false;

        uniformMin = lo;
        uniformMax = hi;
        warnedAboutBounds = false;
        return true;
    }

    void Init()
    {
        Bind(min_, "min");
        Bind(max_, "max");
        Bind(randomSeed, "seed");
        Bind(output, "OUTPUT");

        const int seed = randomSeed.as_int();
        if(seed < 0)
            randomGenerator.seed(std::random_device{}());
        else
            randomGenerator.seed(static_cast<std::mt19937::result_type>(seed));

        if(!UpdateBounds())
            throw exception(
                "Randomizer: bounds must form a finite representable range.",
                path_);
    }


    void Tick()
    {
        if(!UpdateBounds() && !warnedAboutBounds)
        {
            Warning(
                "Randomizer bounds must form a finite representable range; "
                "using the last valid bounds.",
                path_);
            warnedAboutBounds = true;
        }

        const std::uniform_real_distribution<float>::param_type bounds(
            uniformMin, uniformMax);
        output.apply([this, bounds](float) {
            return uniformDistribution(randomGenerator, bounds);
        });
    }
};

INSTALL_CLASS(Randomizer)
