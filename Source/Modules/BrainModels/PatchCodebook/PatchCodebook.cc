#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "ikaros.h"
#include "../PopulationMath.h"

using namespace ikaros;

class PatchCodebook: public Module
{
    parameter fields;
    parameter height;
    parameter width;
    parameter kernelSize;
    parameter codeCount;
    parameter temperature;
    parameter learningRate;
    parameter train;
    parameter randomSeed;
    matrix input;
    matrix output;
    matrix prototypes;
    matrix usage;
    int n,h,w,k,c,ph,pw;
    std::vector<double> distances, sums;
    std::vector<int> counts;
    void
    Init()
    {
        Bind(fields, "fields");
        Bind(height, "height");
        Bind(width, "width");
        Bind(kernelSize, "kernel_size");
        Bind(codeCount, "code_count");
        Bind(temperature, "temperature");
        Bind(learningRate, "learning_rate");
        Bind(train, "train");
        Bind(randomSeed, "random_seed");
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");
        Bind(prototypes, "PROTOTYPES");
        Bind(usage, "USAGE");
        n=population_math::dimension(fields); h=population_math::dimension(height); w=population_math::dimension(width);
        k=population_math::dimension(kernelSize); c=population_math::dimension(codeCount); ph=h-k+1; pw=w-k+1;
        if(ph<1 || pw<1) throw std::invalid_argument("PatchCodebook: kernel exceeds input.");
        population_math::range(temperature.as_double(),0,1e6); population_math::range(learningRate.as_double(),0,1);
        population_math::size(input,n*h*w); population_math::size(output,n*c*ph*pw);
        distances.resize(c); sums.resize(c*k*k); counts.resize(c);
        std::mt19937 rng(randomSeed.as_int()); std::uniform_real_distribution<float> uniform(0,1);
        for(int i=0;i<c;++i) for(int j=0;j<k*k;++j) prototypes(i,j)=uniform(rng);
        usage=0;
    }


    void
    Tick()
    {
        population_math::range(temperature.as_double(),0,1e6); population_math::range(learningRate.as_double(),0,1);
        for(int b=0;b<n;++b)
        {
            std::fill(sums.begin(),sums.end(),0); std::fill(counts.begin(),counts.end(),0);
            for(int y=0;y<ph;++y) for(int x=0;x<pw;++x)
            {
                int winner=0;
                for(int j=0;j<c;++j)
                {
                    double d=0;
                    for(int dy=0;dy<k;++dy) for(int dx=0;dx<k;++dx)
                    {
                        const double e=input.contiguous_data()[(b*h+y+dy)*w+x+dx]-prototypes(j,dy*k+dx); d+=e*e;
                    }
                    distances[j]=d;
                    if(d<distances[winner]) winner=j;
                }
                const double minimum=distances[winner]; double total=0;
                for(int j=0;j<c;++j)
                {
                    const double v=temperature.as_double()>0 ? std::exp(-(distances[j]-minimum)/(k*k*temperature.as_double())) : (j==winner ? 1.0 : 0.0);
                    output(b,j,y,x)=v; total+=v;
                }
                for(int j=0;j<c;++j) output(b,j,y,x)/=total;
                if(train.as_bool())
                {
                    ++counts[winner];
                    for(int dy=0;dy<k;++dy) for(int dx=0;dx<k;++dx)
                        sums[winner*k*k+dy*k+dx]+=input.contiguous_data()[(b*h+y+dy)*w+x+dx];
                }
            }
            if(train.as_bool()) for(int j=0;j<c;++j) if(counts[j])
            {
                for(int d=0;d<k*k;++d) prototypes(j,d)+=learningRate.as_float()*(sums[j*k*k+d]/counts[j]-prototypes(j,d));
                usage(j)+=counts[j];
            }
        }
    }
};

INSTALL_CLASS(PatchCodebook)
