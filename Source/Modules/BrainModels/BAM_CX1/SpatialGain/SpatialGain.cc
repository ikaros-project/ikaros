#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "ikaros.h"
#include "../PopulationMath.h"

using namespace ikaros;

class SpatialGain: public Module
{
    parameter height;
    parameter width;
    parameter fields;
    parameter fieldWidth;
    parameter origin;
    parameter sigma;
    parameter excitation;
    parameter inhibition;
    parameter alpha;
    matrix input;
    matrix attention;
    matrix reset;
    matrix hold;
    matrix output;
    matrix gain;
    int h,w,n;
    std::vector<float> kernel;
    void
    Init()
    {
        Bind(height, "height");
        Bind(width, "width");
        Bind(fields, "fields");
        Bind(fieldWidth, "field_width");
        Bind(origin, "origin");
        Bind(sigma, "sigma");
        Bind(excitation, "excitation");
        Bind(inhibition, "inhibition");
        Bind(alpha, "alpha");
        Bind(input, "INPUT");
        Bind(attention, "ATTENTION");
        Bind(reset, "RESET");
        Bind(hold, "HOLD");
        population_math::size(hold, 1, true);
        Bind(output, "OUTPUT");
        Bind(gain, "GAIN");
        h=population_math::dimension(height); w=population_math::dimension(width); n=population_math::dimension(fields);
        const int f=population_math::dimension(fieldWidth); population_math::range(sigma.as_double(),1e-6,1e6); population_math::range(alpha.as_double(),0,1);
        population_math::size(input,h*w); population_math::size(attention,n); population_math::size(reset,1,true); kernel.resize(n*w);
        for(int b=0;b<n;++b) for(int x=0;x<w;++x) { const double dx=(x-(origin.as_double()+b+(f-1)*.5))/sigma.as_double(); kernel[b*w+x]=std::exp(-.5*dx*dx); }
    }


    void
    Tick()
    {
        if(!hold.empty() && hold(0) > 0)
            return;
        const float *a=attention.contiguous_data(), *x=input.contiguous_data();
        population_math::range(alpha.as_double(),0,1); population_math::range(excitation.as_double(),0,1e6); population_math::range(inhibition.as_double(),0,1e6);
        double mass=0; for(int b=0;b<n;++b) mass+=a[b];
        if(mass<0) throw std::invalid_argument("SpatialGain: attention must be nonnegative.");
        for(int col=0;col<w;++col)
        {
            double drive=0; for(int b=0;b<n;++b) drive+=a[b]*kernel[b*w+col];
            gain(col)=(1+excitation.as_double()*drive)/(1+inhibition.as_double()*mass);
            for(int y=0;y<h;++y)
            {
                const float target=std::clamp(x[y*w+col]*gain(col),0.0f,1.0f);
                output(y,col)=!reset.empty() && reset(0)>0 ? x[y*w+col] : (1-alpha.as_double())*output(y,col)+alpha.as_double()*target;
            }
        }
    }
};

INSTALL_CLASS(SpatialGain)
