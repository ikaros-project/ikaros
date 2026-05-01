#include "ikaros.h"

using namespace ikaros;

class Noise: public Module
{
    parameter type;
    parameter min_;
    parameter max_;
    parameter mean;
    parameter stddev;

    matrix input;
    matrix output;

    void Init()
    {
        Bind(type, "type");
        Bind(min_, "min");
        Bind(max_, "max");
        Bind(mean, "mean");
        Bind(stddev, "stddev");

        Bind(input, "INPUT");
        Bind(output, "OUTPUT");
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

            output.apply([mu, sigma](float x) {
                return x + sample_normal_distribution(mu, sigma);
            });
        }
        else if (type.compare_string("uniform"))
        {
            const float lo = std::min(min_.as_float(), max_.as_float());
            const float hi = std::max(min_.as_float(), max_.as_float());
            output.apply([lo, hi](float x) {
                return x + lo + (float(::random()) / float(RAND_MAX)) * (hi - lo);
            });
        }
        else
            throw exception("Noise: type must be either uniform or gaussian.", path_);
    }
};

INSTALL_CLASS(Noise)
