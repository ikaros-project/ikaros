#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "ikaros.h"
#include "../PopulationMath.h"

using namespace ikaros;

class CategoryCompetition: public Module
{
    parameter rows;
    parameter categories;
    parameter alpha;
    parameter inhibition;
    parameter normalizeInput;
    parameter maximum;
    matrix input;
    matrix cue;
    matrix reset;
    matrix hold;
    matrix output;
    matrix weights;
    int n,c;
    void
    Init()
    {
        Bind(rows, "rows");
        Bind(categories, "categories");
        Bind(alpha, "alpha");
        Bind(inhibition, "inhibition");
        Bind(normalizeInput, "normalize_input");
        Bind(maximum, "maximum");
        Bind(input, "INPUT");
        Bind(cue, "CUE");
        Bind(reset, "RESET");
        Bind(hold, "HOLD");
        population_math::size(hold, 1, true);
        Bind(output, "OUTPUT");
        Bind(weights, "WEIGHTS");
        n=population_math::dimension(rows); c=population_math::dimension(categories);
        population_math::range(alpha.as_double(),0,1); population_math::range(inhibition.as_double(),0,1e6); population_math::range(maximum.as_double(),0,1e6);
        population_math::size(input,n*c); population_math::size(cue,n*c,true); population_math::size(reset,1,true);
    }


    void
    Tick()
    {
        population_math::range(alpha.as_double(),0,1); population_math::range(inhibition.as_double(),0,1e6);
        const bool holding = !hold.empty() && hold(0) > 0;
        if(!holding && !reset.empty() && reset(0)>0) output=0;
        const float *v=input.contiguous_data(); const float *drive=cue.empty() ? nullptr : cue.contiguous_data();
        for(int b=0;b<n;++b)
        {
            double sum=0,maxEvidence=1e-12;
            for(int j=0;j<c;++j) { sum+=output(b,j); maxEvidence=std::max(maxEvidence,double(v[b*c+j])); }
            double mass=0;
            for(int j=0;j<c;++j)
            {
                const double evidence=normalizeInput.as_bool() ? v[b*c+j]/maxEvidence : v[b*c+j];
                const double target=std::clamp(evidence+(drive ? drive[b*c+j] : 0)-inhibition.as_double()*(sum-output(b,j)),0.0,maximum.as_double());
                if(!holding) output(b,j)=(1-alpha.as_double())*output(b,j)+alpha.as_double()*target;
                mass+=output(b,j);
            }
            for(int j=0;j<c;++j) weights(b,j)=mass>0 ? output(b,j)/mass : 0;
        }
    }
};

INSTALL_CLASS(CategoryCompetition)
