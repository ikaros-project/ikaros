#include "ikaros.h"


using namespace ikaros;


    static double max(double x, double y) { return x>y ? x : y; } // std::max does not work since it is a template function that requires identical types of x and y


class Nucleus: public Module
{
    parameter   alpha;          // resting level
    parameter   beta;           // excitation gain 
    parameter   gamma;           // inhibition gain
    parameter   delta;          // decay rate
    parameter   psi;            // shunting weight
    parameter   sigma;          // standard deviation for noise
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

    void Init()
    {
        Bind(alpha, "alpha");
        Bind(beta, "beta");
        Bind(gamma, "gamma");
        Bind(delta, "delta");
        Bind(psi, "psi");
        Bind(sigma, "sigma");
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

        float dx_dt = alpha + beta * (1/(1+psi*S)) * E - gamma * I - delta*x + sample_normal_distribution(0, sigma);

        x += epsilon * dx_dt; // Euler integration

        float o = 0;

        switch(activation_function.as_int())
        {
            case 0: // atan
                    o = atan(x-theta)/atan(1); 
                    break;
            case 1: // threshold
                    if(x > theta)
                    {
                        o = 1;
                        burst_end_time = GetTime() + burst_time;
                    }

                    break;
            case 2: // ReLU
                    o =  (x > theta ? x-theta : 0); 
                    break;
            case 3: // tanh
                    o =  (tanh(x-theta)); 
                    break;
            case 4: // sigmoid
                    o =  1 / (1 + exp(-(x-theta))); 
                    break;
            case 5: // linear
            default:
                    o =  (x-theta); 
                    break;
        }

        output = output_offset + output_scale * o;
    }
};

INSTALL_CLASS(Nucleus)

