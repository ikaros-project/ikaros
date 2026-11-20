#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "ikaros.h"
#include "../PopulationMath.h"

using namespace ikaros;

class FeedbackAlignment: public Module
{
    parameter fields;
    parameter categories;
    parameter references;
    parameter channels;
    parameter patchRows;
    parameter patchColumns;
    parameter sigma;
    parameter massThreshold;
    matrix slots;
    matrix attention;
    matrix category;
    matrix output;
    matrix gate;
    matrix bank;
    matrix blank;
    int n,c,r,k,h,w,g;
    std::vector<double> global,coverage;
    std::vector<float> kernel;
    void
    Init()
    {
        Bind(fields, "fields");
        Bind(categories, "categories");
        Bind(references, "references");
        Bind(channels, "channels");
        Bind(patchRows, "patch_rows");
        Bind(patchColumns, "patch_columns");
        Bind(sigma, "sigma");
        Bind(massThreshold, "mass_threshold");
        Bind(slots, "SLOTS");
        Bind(attention, "ATTENTION");
        Bind(category, "CATEGORY");
        Bind(output, "OUTPUT");
        Bind(gate, "GATE");
        Bind(bank, "BANK");
        Bind(blank, "BLANK");
        n=population_math::dimension(fields); c=population_math::dimension(categories); r=population_math::dimension(references); k=population_math::dimension(channels); h=population_math::dimension(patchRows); w=population_math::dimension(patchColumns); g=n+w-1;
        population_math::size(slots,n*c); population_math::size(attention,n); population_math::size(category,c); population_math::range(sigma.as_double(),1e-6,1e6);
        global.resize(k*h*g); coverage.resize(g); kernel.resize(n*n);
        for(int i=0;i<n;++i) for(int j=0;j<n;++j) { const double dx=(i-j)/sigma.as_double(); kernel[i*n+j]=std::exp(-.5*dx*dx); }
    }


    void
    Tick()
    {
        float *out = output.contiguous_data();
        const float *a=attention.contiguous_data(), *v=category.contiguous_data(), *s=slots.contiguous_data();
        double mass=0,cmass=0; for(int i=0;i<n;++i) mass+=a[i]; for(int j=0;j<c;++j) cmass+=v[j];
        output=0; gate=0;
        if(mass<=massThreshold.as_double() || cmass<=massThreshold.as_double()) return;
        std::fill(global.begin(),global.end(),0); std::fill(coverage.begin(),coverage.end(),0);
        const float *refs=bank.contiguous_data(), *empty=blank.contiguous_data();
        for(int i=0;i<n;++i)
        {
            double drive=0; for(int j=0;j<n;++j) drive+=a[j]*kernel[j*n+i]; gate(i)=std::clamp(drive/std::max(1.0,mass),0.0,1.0);
            if(a[i]==0) continue;
            const double weight=a[i]/mass;
            for(int x=0;x<w;++x) coverage[i+x]+=weight;
            for(int j=0;j<c;++j)
            {
                if(v[j]==0) continue;
                const float slot=s[i*c+j];
                if(!std::isfinite(slot) || slot<0 || slot>=r || std::floor(slot)!=slot) throw std::invalid_argument("FeedbackAlignment: invalid reference slot.");
                const float *ref=refs+(j*r+int(slot))*k*h*w; const double q=weight*v[j]/cmass;
                for(int ch=0;ch<k;++ch) for(int y=0;y<h;++y) for(int x=0;x<w;++x) global[(ch*h+y)*g+i+x]+=q*ref[(ch*h+y)*w+x];
            }
        }
        for(int i=0;i<n;++i) for(int ch=0;ch<k;++ch) for(int y=0;y<h;++y) for(int x=0;x<w;++x)
            out[((i*k+ch)*h+y)*w+x]=global[(ch*h+y)*g+i+x]+std::max(0.0,1-coverage[i+x])*empty[(ch*h+y)*w+x];
    }
};

INSTALL_CLASS(FeedbackAlignment)
