#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "ikaros.h"
#include "../PopulationMath.h"

using namespace ikaros;

class FieldExtraction: public Module
{
    parameter height;
    parameter width;
    parameter fieldWidth;
    parameter padding;
    matrix input;
    matrix output;
    int h, w, f, pad, n;
    void
    Init()
    {
        Bind(height, "height");
        Bind(width, "width");
        Bind(fieldWidth, "field_width");
        Bind(padding, "padding");
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");
        h = population_math::dimension(height); w = population_math::dimension(width); f = population_math::dimension(fieldWidth);
        population_math::range(padding.as_double(), 0, 10000); pad = padding.as_int();
        n = w + 2*pad - f + 1;
        if(n < 1) throw std::invalid_argument("FieldExtraction: no fields fit the padded display.");
        population_math::size(input, h*w); population_math::size(output, n*h*f);
    }


    void
    Tick()
    {
        for(int i=0; i<n; ++i)
            for(int y=0; y<h; ++y)
                for(int x=0; x<f; ++x)
                {
                    const int sx = i-pad+x;
                    output(i,y,x) = sx>=0 && sx<w ? input.contiguous_data()[y*w+sx] : 0.0f;
                }
    }
};

INSTALL_CLASS(FieldExtraction)
