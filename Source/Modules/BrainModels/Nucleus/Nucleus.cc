#include <cmath>
#include <random>

#include "ikaros.h"


using namespace ikaros;


class Nucleus: public Module
{
    parameter   alpha;          // resting level
    parameter   beta;           // excitation gain 
    parameter   gamma;           // inhibition gain
    parameter   delta;          // decay rate
    parameter   psi;            // shunting weight
    parameter   sigma;          // standard deviation for noise
    parameter   randomSeed;     // random seed for noise
    parameter   theta;          // threshold for output
    parameter   epsilon;        // time constant
    parameter   scale_inputs;   // use average instead of sum of inputs
    parameter   activation_function; // 0 = ReLU, 1 = tanh, 2 = sigmoid, 3 = linear
    parameter   burst_time;     // burst time in s for threshold function: 0 means a single tick
    parameter   output_offset;  // offset for output, default is 0
    parameter   output_scale;   // scaling of output, default is 1

    matrix      excitation; 
    matrix      inhibition;
    matrix      shunting_inhibition;
    matrix      x;              // internal state
    matrix      output;

    double      burst_end_time = 0;
    std::mt19937 gaussianGenerator;
    std::normal_distribution<float> gaussianDistribution;

    void Init()
    {
        Bind(alpha, "alpha");
        Bind(beta, "beta");
        Bind(gamma, "gamma");
        Bind(delta, "delta");
        Bind(psi, "psi");
        Bind(sigma, "sigma");
        Bind(randomSeed, "seed");
        Bind(theta, "theta");
        Bind(epsilon, "epsilon");


        Bind(scale_inputs, "scale_inputs");
        Bind(activation_function, "activation_function");
        Bind(burst_time, "burst_time");

        Bind(output_offset, "output_offset");
        Bind(output_scale, "output_scale");     

 
        Bind(excitation, "EXCITATION");
        Bind(inhibition, "INHIBITION");
        Bind(shunting_inhibition, "SHUNTING_INHIBITION");
        Bind(x, "X");
        Bind(output, "OUTPUT");

        const int seed = randomSeed.as_int();
        if(seed < 0)
            gaussianGenerator.seed(std::random_device{}());
        else
            gaussianGenerator.seed(static_cast<std::mt19937::result_type>(seed));
    }

    
    void Tick()
    {
        if(GetTime() < burst_end_time)
            return;
    
        float E = 0;
        float I = 0;
        float S = 0;

        if(scale_inputs)
        {
            E = excitation.average();
            I = inhibition.average();
            S = shunting_inhibition.average();
        }
        else
        {
            E = excitation.sum();
            I = inhibition.sum();
            S = shunting_inhibition.sum();
        }

        float & x_value = x(0);
        float deterministic_drive = alpha + beta * (1/(1+psi*S)) * E - gamma * I - delta*x_value;
        float noise_increment = sample_normal_distribution(
            gaussianGenerator, gaussianDistribution, 0,
            sigma.as_float() * std::sqrt(GetTickDuration()));

        x_value += epsilon * deterministic_drive + noise_increment; // Euler-Maruyama integration

        float o = 0;

        switch(activation_function.as_int())
        {
            case 0: // atan
                    o = atan(x_value-theta)/atan(1);
                    break;
            case 1: // threshold
                    if(x_value > theta)
                    {
                        o = 1;
                        x_value = 0; // reset
                        burst_end_time = GetTime() + burst_time;
                    }

                    break;
            case 2: // ReLU
                    o =  (x_value > theta ? x_value-theta : 0);
                    break;
            case 3: // tanh
                    o =  (tanh(x_value-theta));
                    break;
            case 4: // sigmoid
                    o =  1 / (1 + exp(-(x_value-theta)));
                    break;
            case 5: // linear
            default:
                    o =  (x_value-theta);
                    break;
        }

        output = output_offset + output_scale * o;
    }
};

INSTALL_CLASS(Nucleus)
