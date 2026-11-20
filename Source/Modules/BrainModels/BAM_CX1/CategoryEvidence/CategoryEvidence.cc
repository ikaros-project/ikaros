#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "ikaros.h"
#include "../PopulationMath.h"

using namespace ikaros;

class CategoryEvidence: public Module
{
    parameter rows;
    parameter inputSize;
    parameter categories;
    parameter unitsPerCategory;
    parameter train;
    parameter learningRate;
    parameter labelSmoothing;
    parameter weightDecay;
    parameter randomSeed;
    matrix input;
    matrix labels;
    matrix output;
    matrix units;
    matrix weights;
    matrix bias;
    matrix first_moment;
    matrix second_moment;
    matrix bias_first;
    matrix bias_second;
    matrix step;
    int n,d,c,u,q;
    matrix values;
    std::vector<double> gradient, biasGradient, conditional;
    void
    Init()
    {
        Bind(rows, "rows");
        Bind(inputSize, "input_size");
        Bind(categories, "categories");
        Bind(unitsPerCategory, "units_per_category");
        Bind(train, "train");
        Bind(learningRate, "learning_rate");
        Bind(labelSmoothing, "label_smoothing");
        Bind(weightDecay, "weight_decay");
        Bind(randomSeed, "random_seed");
        Bind(input, "INPUT");
        Bind(labels, "LABELS");
        Bind(output, "OUTPUT");
        Bind(units, "UNITS");
        Bind(weights, "WEIGHTS");
        Bind(bias, "BIAS");
        Bind(first_moment, "FIRST_MOMENT");
        Bind(second_moment, "SECOND_MOMENT");
        Bind(bias_first, "BIAS_FIRST");
        Bind(bias_second, "BIAS_SECOND");
        Bind(step, "STEP");
        n=population_math::dimension(rows); d=population_math::dimension(inputSize); c=population_math::dimension(categories); u=population_math::dimension(unitsPerCategory); q=c*u;
        population_math::size(input,n*d); population_math::size(labels,n,true); population_math::size(output,n*c); population_math::size(units,n*q);
        population_math::range(labelSmoothing.as_double(),0,1); population_math::range(learningRate.as_double(),0,1); population_math::range(weightDecay.as_double(),0,1);
        values.realloc(n,d); gradient.resize(d*q); biasGradient.resize(q); conditional.resize(n*q);
        std::mt19937 rng(randomSeed.as_int()); std::normal_distribution<float> normal(0,.01);
        for(int i=0;i<d;++i) for(int j=0;j<q;++j) weights(i,j)=normal(rng);
    }


    void
    Tick()
    {
        std::copy(input.contiguous_data(),input.contiguous_data()+n*d,values.contiguous_data());
        units.matmul(values,weights); output=0;
        for(int i=0;i<n;++i)
        {
            float maximum=-std::numeric_limits<float>::infinity();
            for(int j=0;j<q;++j) { units(i,j)+=bias(j); maximum=std::max(maximum,units(i,j)); }
            for(int cat=0;cat<c;++cat)
            {
                double mx=-std::numeric_limits<double>::infinity(), norm=0;
                for(int j=0;j<u;++j) mx=std::max(mx,double(units(i,cat*u+j)));
                for(int j=0;j<u;++j) { conditional[i*q+cat*u+j]=std::exp(units(i,cat*u+j)-mx); norm+=conditional[i*q+cat*u+j]; }
                for(int j=0;j<u;++j) conditional[i*q+cat*u+j]/=norm;
            }
            double total=0;
            for(int j=0;j<q;++j) { units(i,j)=std::exp(units(i,j)-maximum); total+=units(i,j); }
            for(int j=0;j<q;++j) { units(i,j)/=total; output(i,j/u)+=units(i,j); }
        }
        if(!train.as_bool()) return;
        if(labels.empty()) throw std::invalid_argument("CategoryEvidence: training requires labels.");
        population_math::range(labelSmoothing.as_double(),0,1); population_math::range(learningRate.as_double(),0,1); population_math::range(weightDecay.as_double(),0,1);
        std::fill(gradient.begin(),gradient.end(),0); std::fill(biasGradient.begin(),biasGradient.end(),0);
        for(int i=0;i<n;++i)
        {
            const float label=labels.contiguous_data()[i];
            if(!std::isfinite(label) || label<0 || label>=c || label!=std::floor(label)) throw std::invalid_argument("CategoryEvidence: invalid category label.");
            for(int j=0;j<q;++j)
            {
                const double target=labelSmoothing.as_double()/c+(j/u==int(label) ? 1-labelSmoothing.as_double() : 0);
                const double delta=(units(i,j)-target*conditional[i*q+j])/n;
                biasGradient[j]+=delta;
                for(int k=0;k<d;++k) gradient[k*q+j]+=values(i,k)*delta;
            }
        }
        step(0)+=1; const double b1=1-std::pow(.9,step(0)), b2=1-std::pow(.999,step(0));
        const double lr=learningRate.as_double();
        for(int j=0;j<q;++j)
        {
            const double g=biasGradient[j]; bias_first(j)=.9*bias_first(j)+.1*g; bias_second(j)=.999*bias_second(j)+.001*g*g;
            bias(j)-=lr*(bias_first(j)/b1)/(std::sqrt(bias_second(j)/b2)+1e-8);
            for(int k=0;k<d;++k)
            {
                const double v=gradient[k*q+j]; first_moment(k,j)=.9*first_moment(k,j)+.1*v; second_moment(k,j)=.999*second_moment(k,j)+.001*v*v;
                weights(k,j)*=1-lr*weightDecay.as_double(); weights(k,j)-=lr*(first_moment(k,j)/b1)/(std::sqrt(second_moment(k,j)/b2)+1e-8);
            }
        }
    }
};

INSTALL_CLASS(CategoryEvidence)
