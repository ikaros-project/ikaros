#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "ikaros.h"
#include "../PopulationMath.h"

using namespace ikaros;

class PatchReconstruction: public Module
{
    parameter fields;
    parameter height;
    parameter width;
    parameter kernelSize;
    parameter codeCount;
    parameter clip;
    matrix input;
    matrix output;
    matrix weights;
    int n,h,w,k,c,ph,pw;
    std::vector<int> coverage;
    void
    Init()
    {
        Bind(fields, "fields");
        Bind(height, "height");
        Bind(width, "width");
        Bind(kernelSize, "kernel_size");
        Bind(codeCount, "code_count");
        Bind(clip, "clip");
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");
        Bind(weights, "WEIGHTS");
        n=population_math::dimension(fields); h=population_math::dimension(height); w=population_math::dimension(width);
        k=population_math::dimension(kernelSize); c=population_math::dimension(codeCount); ph=h-k+1; pw=w-k+1;
        if(ph<1 || pw<1) throw std::invalid_argument("PatchReconstruction: kernel exceeds input.");
        population_math::size(input,n*c*ph*pw); population_math::size(output,n*h*w);
        coverage.assign(h*w,0);
        for(int y=0;y<ph;++y) for(int x=0;x<pw;++x) for(int dy=0;dy<k;++dy) for(int dx=0;dx<k;++dx) ++coverage[(y+dy)*w+x+dx];
    }


    void
    Tick()
    {
        output=0;
        for(int b=0;b<n;++b) for(int y=0;y<ph;++y) for(int x=0;x<pw;++x)
            for(int j=0;j<c;++j) for(int dy=0;dy<k;++dy) for(int dx=0;dx<k;++dx)
                output(b,y+dy,x+dx)+=input.contiguous_data()[((b*c+j)*ph+y)*pw+x]*weights(j,dy*k+dx);
        for(int b=0;b<n;++b) for(int y=0;y<h;++y) for(int x=0;x<w;++x)
        {
            float v=output(b,y,x)/coverage[y*w+x];
            output(b,y,x)=clip.as_bool() ? std::clamp(v,0.0f,1.0f) : v;
        }
    }
};

INSTALL_CLASS(PatchReconstruction)
