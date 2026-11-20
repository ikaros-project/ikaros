#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "ikaros.h"
#include "../PopulationMath.h"

using namespace ikaros;

class CategoryPrediction: public Module
{
    parameter rows;
    parameter categories;
    parameter features;
    matrix input;
    matrix activity;
    matrix output;
    int n,c,f;
    void
    Init()
    {
        Bind(rows, "rows");
        Bind(categories, "categories");
        Bind(features, "features");
        Bind(input, "INPUT");
        Bind(activity, "ACTIVITY");
        Bind(output, "OUTPUT");
        n=population_math::dimension(rows); c=population_math::dimension(categories); f=population_math::dimension(features);
        population_math::size(input,n*c*f); population_math::size(activity,n*c);
    }


    void
    Tick()
    {
        const float *x=input.contiguous_data(), *a=activity.contiguous_data(); output=0;
        for(int b=0;b<n;++b)
        {
            double mass=0; for(int j=0;j<c;++j) mass+=std::max(0.0f,a[b*c+j]);
            if(mass==0) continue;
            for(int j=0;j<c;++j) for(int k=0;k<f;++k) output(b,k)+=std::max(0.0f,a[b*c+j])/mass*x[(b*c+j)*f+k];
        }
    }
};

INSTALL_CLASS(CategoryPrediction)
