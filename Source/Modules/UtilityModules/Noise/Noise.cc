#include <random>

#include "ikaros.h"

using namespace ikaros;

class Noise: public Module
{
    parameter type;
    parameter min_;
    parameter max_;
    parameter mean;
    parameter stddev;
    parameter randomSeed;

    matrix input;
    matrix output;
    std::mt19937 randomGenerator;
    std::normal_distribution<float> gaussianDistribution;
    std::uniform_real_distribution<float> uniformDistribution;

    void Init()
    {
        Bind(type, "type");
        Bind(min_, "min");
        Bind(max_, "max");
        Bind(mean, "mean");
        Bind(stddev, "stddev");
        Bind(randomSeed, "seed");

        Bind(input, "INPUT");
        Bind(output, "OUTPUT");

        const int seed = randomSeed.as_int();
        if(seed < 0)
            randomGenerator.seed(std::random_device{}());
        else
            randomGenerator.seed(static_cast<std::mt19937::result_type>(seed));
    }

    void Tick()
    {
        output.copy(input);

        if (type.compare_string("gaussian"))
        {
            const float mu = mean.as_float();
            const float sigma = stddev.as_float();
            if (sigma < 0.0f)
                throw exception("Noise: stddev must be greater than or equal to 0.", path_);

            output.apply([this, mu, sigma](float x) {
                return x + sample_normal_distribution(
                    randomGenerator, gaussianDistribution, mu, sigma);
            });
        }
        else if (type.compare_string("uniform"))
        {
            const float lo = std::min(min_.as_float(), max_.as_float());
            const float hi = std::max(min_.as_float(), max_.as_float());
            const std::uniform_real_distribution<float>::param_type bounds(
                lo, hi);
            output.apply([this, bounds](float x) {
                return x + uniformDistribution(randomGenerator, bounds);
            });
        }
        else
            throw exception("Noise: type must be either uniform or gaussian.", path_);
    }
};

INSTALL_CLASS(Noise)
