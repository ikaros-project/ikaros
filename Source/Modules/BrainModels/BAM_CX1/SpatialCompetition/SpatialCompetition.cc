#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "ikaros.h"
#include "../PopulationMath.h"

using namespace ikaros;

class SpatialCompetition: public Module
{
    parameter fields;
    parameter categories;
    parameter alpha;
    parameter inhibition;
    parameter categoryGain;
    parameter driveGain;
    matrix input;
    matrix category;
    matrix cue;
    matrix reset;
    matrix hold;
    matrix output;
    int n,c;
    void
    Init()
    {
        Bind(fields, "fields");
        Bind(categories, "categories");
        Bind(alpha, "alpha");
        Bind(inhibition, "inhibition");
        Bind(categoryGain, "category_gain");
        Bind(driveGain, "drive_gain");
        Bind(input, "INPUT");
        Bind(category, "CATEGORY");
        Bind(cue, "CUE");
        Bind(reset, "RESET");
        Bind(hold, "HOLD");
        population_math::size(hold, 1, true);
        Bind(output, "OUTPUT");
        n=population_math::dimension(fields); c=population_math::dimension(categories); population_math::range(alpha.as_double(),0,1);
        population_math::size(input,n*c); population_math::size(category,c); population_math::size(cue,n,true); population_math::size(reset,1,true);
    }


    void
    Tick()
    {
        if(!hold.empty() && hold(0) > 0)
            return;
        const float *t=input.contiguous_data(), *v=category.contiguous_data();
        population_math::range(alpha.as_double(),0,1); population_math::range(inhibition.as_double(),0,1e6); population_math::range(categoryGain.as_double(),0,1e6);
        if(!reset.empty() && reset(0)>0) output=0;
        double total=0,cmass=0; for(int j=0;j<c;++j) cmass+=v[j]; for(int b=0;b<n;++b) total+=output(b);
        const double denominator=1+categoryGain.as_double()*cmass;
        if(denominator<=0) throw std::invalid_argument("SpatialCompetition: invalid category activity.");
        for(int b=0;b<n;++b)
        {
            double drive=0; for(int j=0;j<c;++j) drive+=t[b*c+j]*(1+categoryGain.as_double()*v[j])/denominator;
            const double target=std::clamp(driveGain.as_double()*drive+(cue.empty() ? 0 : cue.contiguous_data()[b])-inhibition.as_double()*(total-output(b)),0.0,1.0);
            output(b)=(1-alpha.as_double())*output(b)+alpha.as_double()*target;
        }
    }
};

INSTALL_CLASS(SpatialCompetition)
