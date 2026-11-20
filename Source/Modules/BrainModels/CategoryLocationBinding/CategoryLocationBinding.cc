#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "ikaros.h"
#include "../PopulationMath.h"

using namespace ikaros;

class CategoryLocationBinding: public Module
{
    parameter fields;
    parameter categories;
    parameter alpha;
    parameter inhibition;
    parameter supportGain;
    parameter mismatchGain;
    parameter categoryGain;
    parameter threshold;
    parameter normalizeEvidence;
    matrix input;
    matrix support;
    matrix mismatch;
    matrix category;
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
        Bind(supportGain, "support_gain");
        Bind(mismatchGain, "mismatch_gain");
        Bind(categoryGain, "category_gain");
        Bind(threshold, "threshold");
        Bind(normalizeEvidence, "normalize_evidence");
        Bind(input, "INPUT");
        Bind(support, "SUPPORT");
        Bind(mismatch, "MISMATCH");
        Bind(category, "CATEGORY");
        Bind(reset, "RESET");
        Bind(hold, "HOLD");
        population_math::size(hold, 1, true);
        Bind(output, "OUTPUT");
        n=population_math::dimension(fields); c=population_math::dimension(categories); population_math::range(alpha.as_double(),0,1);
        population_math::size(input,n*c); population_math::size(support,n); population_math::size(mismatch,n); population_math::size(category,c);
    }


    void
    Tick()
    {
        if(!hold.empty() && hold(0) > 0)
            return;
        const float *v=input.contiguous_data(), *m=support.contiguous_data(), *e=mismatch.contiguous_data(), *a=category.contiguous_data();
        population_math::range(alpha.as_double(),0,1); population_math::range(inhibition.as_double(),0,1e6);
        if(!reset.empty() && reset(0)>0) output=0;
        for(int b=0;b<n;++b)
        {
            double total=0,maximum=1e-6; for(int j=0;j<c;++j) { total+=output(b,j); maximum=std::max(maximum,double(v[b*c+j])); }
            for(int j=0;j<c;++j)
            {
                const double evidence=normalizeEvidence.as_bool() ? v[b*c+j]/maximum : v[b*c+j];
                const double drive=supportGain.as_double()*m[b]*evidence*(1+categoryGain.as_double()*a[j])-mismatchGain.as_double()*e[b]-threshold.as_double();
                const double target=std::clamp(drive-inhibition.as_double()*(total-output(b,j)),0.0,1.0);
                output(b,j)=(1-alpha.as_double())*output(b,j)+alpha.as_double()*target;
            }
        }
    }
};

INSTALL_CLASS(CategoryLocationBinding)
