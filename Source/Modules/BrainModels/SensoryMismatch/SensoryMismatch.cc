#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "ikaros.h"
#include "../PopulationMath.h"

using namespace ikaros;

class SensoryMismatch: public Module
{
    parameter fields;
    parameter pixels;
    parameter alpha;
    matrix input;
    matrix prediction;
    matrix reset;
    matrix hold;
    matrix output;
    matrix positive;
    matrix negative;
    int n,f;
    void
    Init()
    {
        Bind(fields, "fields");
        Bind(pixels, "pixels");
        Bind(alpha, "alpha");
        Bind(input, "INPUT");
        Bind(prediction, "PREDICTION");
        Bind(reset, "RESET");
        Bind(hold, "HOLD");
        population_math::size(hold, 1, true);
        Bind(output, "OUTPUT");
        Bind(positive, "POSITIVE");
        Bind(negative, "NEGATIVE");
        n=population_math::dimension(fields); f=population_math::dimension(pixels); population_math::range(alpha.as_double(),0,1);
        population_math::size(input,n*f); population_math::size(prediction,n*f); population_math::size(reset,1,true);
    }


    void
    Tick()
    {
        if(!hold.empty() && hold(0) > 0)
            return;
        const float *x=input.contiguous_data(), *y=prediction.contiguous_data(); const double a=alpha.as_double(); population_math::range(a,0,1);
        if(!reset.empty() && reset(0)>0) { output=0; positive=0; negative=0; }
        for(int b=0;b<n;++b)
        {
            double ep=0,em=0,mean=0;
            for(int k=0;k<f;++k) { const double d=x[b*f+k]-y[b*f+k]; ep+=std::max(d,0.0); em+=std::max(-d,0.0); mean+=x[b*f+k]; }
            const double support=std::max(mean/f-positive(b),0.0);
            output(b)=(1-a)*output(b)+a*support;
            positive(b)=(1-a)*positive(b)+a*ep/f; negative(b)=(1-a)*negative(b)+a*em/f;
        }
    }
};

INSTALL_CLASS(SensoryMismatch)
