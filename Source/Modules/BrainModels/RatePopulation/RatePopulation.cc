#include <algorithm>
#include <cmath>

#include "ikaros.h"

using namespace ikaros;

class RatePopulation: public Module
{
    enum ActivationFunction
    {
        linear,
        relu,
        sigmoid,
    };

    parameter populationSize;
    parameter activationFunction;
    parameter timeConstant;
    parameter gain;
    parameter bias;
    parameter sigmoidMidpoint;
    parameter minimumOutput;
    parameter maximumOutput;
    parameter initialOutput;

    matrix input;
    matrix excitation;
    matrix inhibition;
    matrix modulation;
    matrix reset;
    matrix output;
    matrix activation;

    void Init()
    {
        Bind(populationSize, "population_size");
        Bind(activationFunction, "activation_function");
        Bind(timeConstant, "time_constant");
        Bind(gain, "gain");
        Bind(bias, "bias");
        Bind(sigmoidMidpoint, "sigmoid_midpoint");
        Bind(minimumOutput, "minimum_output");
        Bind(maximumOutput, "maximum_output");
        Bind(initialOutput, "initial_output");

        Bind(input, "INPUT");
        Bind(excitation, "EXCITATION");
        Bind(inhibition, "INHIBITION");
        Bind(modulation, "MODULATION");
        Bind(reset, "RESET");
        Bind(output, "OUTPUT");
        Bind(activation, "ACTIVATION");

        const int size = populationSize.as_int();
        ValidateSize(input, size, "INPUT");
        ValidateSize(excitation, size, "EXCITATION");
        ValidateSize(inhibition, size, "INHIBITION");
        ValidateSize(modulation, size, "MODULATION");
        ValidateSize(reset, size, "RESET");
        ValidateSize(output, size, "OUTPUT");
        ValidateSize(activation, size, "ACTIVATION");

        if(GetTickDuration() <= 0)
            throw exception("RatePopulation: tick_duration must be positive.", path_);
        if(maximumOutput.as_double() < minimumOutput.as_double())
            throw exception("RatePopulation: maximum_output must not be smaller than minimum_output.", path_);

        output = std::clamp(initialOutput.as_float(), minimumOutput.as_float(), maximumOutput.as_float());
        activation = output;
    }

    void Tick()
    {
        const double decay = std::exp(-GetTickDuration() / timeConstant.as_double());

        for(int i = 0; i < output.size(); ++i)
        {
            if(!reset.empty() && reset(i) > 0.0f)
                output(i) = std::clamp(initialOutput.as_float(), minimumOutput.as_float(),
                                       maximumOutput.as_float());

            const double drive = Value(input, i) + Value(excitation, i) -
                                 Value(inhibition, i) + Value(modulation, i) + bias.as_double();
            const double target = Activate(drive);
            activation(i) = static_cast<float>(target);
            output(i) = static_cast<float>(target + (output(i) - target) * decay);
        }
    }

    void ValidateSize(const matrix & value, int expected, const std::string & name)
    {
        if(!value.empty() && value.size() != expected)
            throw exception("RatePopulation: " + name + " must contain population_size elements.", path_);
    }

    float Value(const matrix & value, int index) const
    {
        return value.empty() ? 0.0f : value(index);
    }

    double Activate(double drive) const
    {
        const double minimum = minimumOutput.as_double();
        const double maximum = maximumOutput.as_double();
        double value;

        if(activationFunction.as_int() == sigmoid)
        {
            const double exponent = std::clamp(-gain.as_double() *
                                                   (drive - sigmoidMidpoint.as_double()),
                                               -60.0, 60.0);
            value = minimum + (maximum - minimum) / (1.0 + std::exp(exponent));
        }
        else
        {
            value = gain.as_double() * drive;
            if(activationFunction.as_int() == relu)
                value = std::max(0.0, value);
            value = std::clamp(value, minimum, maximum);
        }

        return value;
    }
};

INSTALL_CLASS(RatePopulation)
