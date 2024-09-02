#include "ikaros.h"

#include <math.h>

using namespace ikaros;

class Oscillator: public Module
{
    parameter   osc_type;
    matrix      frequency;
    parameter   sample_rate;
    matrix      input;
    matrix      output;
    double       modulator = 0;

    void Init()
    {
        Bind(osc_type, "type");
        Bind(frequency, "frequency");
        Bind(sample_rate, "sample_rate");
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");

        //if(input.connected() && frequency.shape() != output.shape())
        //    throw exception("INPUT must have the same size as frequency");

    }


    float func(float time, float freq)
    {
        if(input.connected())
            freq += 50*input(0);
    
        switch(osc_type.as_int())
        {
            case 0: return sin(2*M_PI*time*freq);
            case 1: return sin(2*M_PI*time*freq) > 0 ? 1 : 0;
            default: return 0;
        }
    }

    void Tick()
    {
        float time = kernel().GetNominalTime();

        // If no buffer is used, apply function to every element of the output

        if(sample_rate==0)
        {
            output.apply(frequency, [=](float x, float f) {return func(time, f);});
            return;
        }

        // // Iterate over all parameters and fill the buffer for each

        double sr = sample_rate;
        double sx = output.size_x();
        double time_increment = kernel().GetTickDuration()/double(output.size_x()); // Dimension of the last dimension that holds the buffer

        if(output.rank() == 2)
        {
            for(int row=0; row<output.size_y(); row++) // Iterate over fist dimension
                for(int i=0; i<output.size_x(); i++) // Fill buffer
                    output(row, i) = func(time+double(i)*time_increment, frequency(row));
            return;
        }
        
        if(output.rank() == 3)
        {
            for(int row=0; row<output.size(0); row++) // Iterate over fist dimension
                for(int col=0; col<output.size(1); col++) // Iterate over second dimension
                for(int i=0; i<output.size(2); i++) // Fill buffer
                    output(row, col, i) = func(time+double(i)*time_increment, frequency(row, col));
            return;
        }
    }
};

INSTALL_CLASS(Oscillator)

