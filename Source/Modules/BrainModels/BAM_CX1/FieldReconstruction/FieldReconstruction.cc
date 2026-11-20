#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "ikaros.h"
#include "../PopulationMath.h"

using namespace ikaros;

class FieldReconstruction: public Module
{
    parameter height;
    parameter width;
    parameter fieldWidth;
    parameter padding;
    matrix input;
    matrix output;
    int h,w,f,pad,n;
    std::vector<int> coverage;
    void
    Init()
    {
        Bind(height, "height");
        Bind(width, "width");
        Bind(fieldWidth, "field_width");
        Bind(padding, "padding");
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");
        h=population_math::dimension(height); w=population_math::dimension(width); f=population_math::dimension(fieldWidth); population_math::range(padding.as_double(),0,10000); pad=padding.as_int(); n=w+2*pad-f+1;
        if(n<1) throw std::invalid_argument("FieldReconstruction: no fields fit.");
        population_math::size(input,n*h*f); coverage.assign(w,0);
        for(int i=0;i<n;++i) for(int x=0;x<f;++x) if(i-pad+x>=0 && i-pad+x<w) ++coverage[i-pad+x];
    }


    void
    Tick()
    {
        output=0; const float *v=input.contiguous_data();
        for(int i=0;i<n;++i) for(int y=0;y<h;++y) for(int x=0;x<f;++x)
        {
            const int col=i-pad+x; if(col>=0 && col<w) output(y,col)+=v[(i*h+y)*f+x]/coverage[col];
        }
    }
};

INSTALL_CLASS(FieldReconstruction)
