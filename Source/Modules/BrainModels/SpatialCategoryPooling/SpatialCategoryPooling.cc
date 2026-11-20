#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "ikaros.h"
#include "../PopulationMath.h"

using namespace ikaros;

class SpatialCategoryPooling: public Module
{
    parameter fields;
    parameter categories;
    parameter epsilon;
    matrix input;
    matrix attention;
    matrix output;
    int n,c;
    void
    Init()
    {
        Bind(fields, "fields");
        Bind(categories, "categories");
        Bind(epsilon, "epsilon");
        Bind(input, "INPUT");
        Bind(attention, "ATTENTION");
        Bind(output, "OUTPUT");
        n=population_math::dimension(fields); c=population_math::dimension(categories); population_math::range(epsilon.as_double(),1e-12,1);
        population_math::size(input,n*c); population_math::size(attention,n);
    }


    void
    Tick()
    {
        const float *t=input.contiguous_data(), *a=attention.contiguous_data(); output=0;
        double total=epsilon.as_double();
        for(int b=0;b<n;++b) for(int j=0;j<c;++j) { const double v=t[b*c+j]*a[b]; output(0,j)+=v; total+=v; }
        for(int j=0;j<c;++j) output(0,j)/=total;
    }
};

INSTALL_CLASS(SpatialCategoryPooling)
