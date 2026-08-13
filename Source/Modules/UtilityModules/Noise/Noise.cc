#include <algorithm>
#include <cmath>
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
    float uniformMin = 0.0f;
    float uniformMax = 0.0f;
    bool hasUniformBounds = false;
    bool warnedAboutUniformBounds = false;

    bool UpdateUniformBounds()
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
        hasUniformBounds = true;
        warnedAboutUniformBounds = false;
        return true;
    }

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

        if(!UpdateUniformBounds() && type.compare_string("uniform"))
            throw exception(
                "Noise: uniform bounds must form a finite representable range.",
                path_);
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
            if(!UpdateUniformBounds())
            {
                if(!warnedAboutUniformBounds)
                {
                    Warning(
                        "Noise uniform bounds must form a finite representable "
                        "range; using the last valid bounds.",
                        path_);
                    warnedAboutUniformBounds = true;
                }
                if(!hasUniformBounds)
                    return;
            }

            const std::uniform_real_distribution<float>::param_type bounds(
                uniformMin, uniformMax);
            output.apply([this, bounds](float x) {
                return x + uniformDistribution(randomGenerator, bounds);
            });
        }
        else
            throw exception("Noise: type must be either uniform or gaussian.", path_);
    }
};

INSTALL_CLASS(Noise)
