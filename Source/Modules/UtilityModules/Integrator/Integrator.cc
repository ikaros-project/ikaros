#include "ikaros.h"

using namespace ikaros;

class Integrator: public Module
{
    parameter alpha;
    parameter beta;
    parameter minValue;
    parameter maxValue;

    matrix input_;
    matrix output_;

    void Init()
    {
        Bind(alpha, "alpha");
        Bind(beta, "beta");
        Bind(minValue, "min");
        Bind(maxValue, "max");

        Bind(input_, "INPUT");
        Bind(output_, "OUTPUT");

        if(input_.shape() != output_.shape())
            throw exception("Integrator: OUTPUT shape must match INPUT shape.", path_);

        output_ = 0.0f;
    }

    void Tick()
    {
        const float a = alpha.as_float();
        const float b = beta.as_float();
        const float minimum = minValue.as_float();
        const float maximum = maxValue.as_float();

        for(int i = 0; i < output_.size(); ++i)
        {
            float value = a * output_.data()[i] + b * input_.data()[i];

            if(value < minimum)
                value = minimum;
            if(value > maximum)
                value = maximum;

            output_.data()[i] = value;
        }
    }
};

INSTALL_CLASS(Integrator)
