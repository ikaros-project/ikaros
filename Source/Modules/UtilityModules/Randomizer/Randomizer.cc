#include <algorithm>
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
    }


    void Tick()
    {
        const float lo = std::min(min_.as_float(), max_.as_float());
        const float hi = std::max(min_.as_float(), max_.as_float());
        const std::uniform_real_distribution<float>::param_type bounds(
            lo, hi);
        output.apply([this, bounds](float) {
            return uniformDistribution(randomGenerator, bounds);
        });
    }
};

INSTALL_CLASS(Randomizer)
