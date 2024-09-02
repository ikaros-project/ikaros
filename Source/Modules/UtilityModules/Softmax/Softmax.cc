#include "ikaros.h"

using namespace ikaros;

class Softmax: public Module
{
    //parameter data;
    matrix input;
    matrix output;

    void Init()
    {
        //Bind(data, "data");
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");
    }


    void Tick()
    {
        // input_cache = input;
        
        // Compute exp(x - max(x)) for numerical stability
        float max_val = input.max();
        matrix exp_input = input;
        exp_input.apply([max_val](float x) { return std::exp(x - max_val); });
        
        // Compute sum of exponentials
        float sum_exp = exp_input.sum();
        
        // Compute softmax: exp(x) / sum(exp(x))
        output.copy(exp_input);
        output.apply([sum_exp](float x) { return x / sum_exp; });
    }
};

INSTALL_CLASS(Softmax)

