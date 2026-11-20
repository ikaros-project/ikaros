#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "ikaros.h"
#include "../PopulationMath.h"

using namespace ikaros;

class ContentIntegration: public Module
{
    parameter rows;
    parameter features;
    parameter alpha;
    parameter fraction;
    matrix input;
    matrix prediction;
    matrix gate;
    matrix reset;
    matrix hold;
    matrix output;
    int n,f;
    void
    Init()
    {
        Bind(rows, "rows");
        Bind(features, "features");
        Bind(alpha, "alpha");
        Bind(fraction, "fraction");
        Bind(input, "INPUT");
        Bind(prediction, "PREDICTION");
        Bind(gate, "GATE");
        Bind(reset, "RESET");
        Bind(hold, "HOLD");
        population_math::size(hold, 1, true);
        Bind(output, "OUTPUT");
        n=population_math::dimension(rows); f=population_math::dimension(features);
        population_math::range(alpha.as_double(),0,1); population_math::range(fraction.as_double(),0,1);
        population_math::size(input,n*f); population_math::size(prediction,n*f); population_math::size(gate,n,true); population_math::size(reset,1,true);
    }


    void
    Tick()
    {
        if(!hold.empty() && hold(0) > 0)
            return;
        population_math::range(alpha.as_double(),0,1); population_math::range(fraction.as_double(),0,1);
        const float *x=input.contiguous_data(), *p=prediction.contiguous_data();
        float *out = output.contiguous_data();
        const bool initializing=!reset.empty() && reset(0)>0;
        for(int b=0;b<n;++b)
        {
            const double lam=fraction.as_double()*(gate.empty() ? 1 : std::clamp(double(gate.contiguous_data()[b]),0.0,1.0));
            for(int k=0;k<f;++k) out[b*f+k]=initializing ? x[b*f+k] : (1-alpha.as_double())*out[b*f+k]+alpha.as_double()*((1-lam)*x[b*f+k]+lam*p[b*f+k]);
        }
    }
};

INSTALL_CLASS(ContentIntegration)
