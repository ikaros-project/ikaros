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
    matrix patches, transposed, scores;
    std::vector<float> norms, prototypeNorms;
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
        patches.realloc(n*ph*pw,k*k); transposed.realloc(k*k,c); scores.realloc(n*ph*pw,c);
        norms.resize(n*ph*pw); prototypeNorms.resize(c);
        distances.resize(c); sums.resize(c*k*k); counts.resize(c);
        std::mt19937 rng(randomSeed.as_int()); std::uniform_real_distribution<float> uniform(0,1);
        for(int i=0;i<c;++i) for(int j=0;j<k*k;++j) prototypes(i,j)=uniform(rng);
        usage=0;
    }


    void
    Tick()
    {
        if(train.as_bool())
        {
            CompetitiveTick();
            return;
        }
        population_math::range(temperature.as_double(),0,1e6);
        const float *x=input.contiguous_data();
        float *out=output.contiguous_data(), *patch=patches.contiguous_data(), *score=scores.contiguous_data();
        for(int b=0;b<n;++b) for(int y=0;y<ph;++y) for(int col=0;col<pw;++col)
        {
            const int row=(b*ph+y)*pw+col; float norm=0;
            for(int dy=0;dy<k;++dy) for(int dx=0;dx<k;++dx)
            {
                const float v=x[(b*h+y+dy)*w+col+dx]; patch[row*k*k+dy*k+dx]=v; norm+=v*v;
            }
            norms[row]=norm;
        }
        for(int j=0;j<c;++j)
        {
            float norm=0;
            for(int d=0;d<k*k;++d) { const float v=prototypes(j,d); transposed(d,j)=v; norm+=v*v; }
            prototypeNorms[j]=norm;
        }
        scores.matmul(patches,transposed);
        for(int row=0;row<n*ph*pw;++row)
        {
            int winner=0;
            for(int j=0;j<c;++j)
            {
                score[row*c+j]=norms[row]+prototypeNorms[j]-2*score[row*c+j];
                if(score[row*c+j]<score[row*c+winner]) winner=j;
            }
            const float minimum=score[row*c+winner]; double total=0;
            const int b=row/(ph*pw),y=(row/pw)%ph,col=row%pw;
            for(int j=0;j<c;++j)
            {
                const float v=temperature.as_double()>0 ? std::exp(-(score[row*c+j]-minimum)/(k*k*temperature.as_double())) : (j==winner ? 1.0f : 0.0f);
                out[((b*c+j)*ph+y)*pw+col]=v; total+=v;
            }
            for(int j=0;j<c;++j) out[((b*c+j)*ph+y)*pw+col]/=total;
        }
    }


    void
    CompetitiveTick()
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
