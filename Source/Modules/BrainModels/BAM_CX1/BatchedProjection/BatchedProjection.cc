#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "ikaros.h"
#include "../PopulationMath.h"

using namespace ikaros;

class BatchedProjection: public Module
{
    parameter rows;
    parameter inputSize;
    parameter outputSize;
    parameter softmaxChannels;
    matrix input;
    matrix output;
    matrix weights;
    matrix bias;
    int n,d,q,c;
    matrix values;
    void
    Init()
    {
        Bind(rows, "rows");
        Bind(inputSize, "input_size");
        Bind(outputSize, "output_size");
        Bind(softmaxChannels, "softmax_channels");
        Bind(input, "INPUT");
        Bind(output, "OUTPUT");
        Bind(weights, "WEIGHTS");
        Bind(bias, "BIAS");
        n=population_math::dimension(rows); d=population_math::dimension(inputSize); q=population_math::dimension(outputSize); c=population_math::dimension(softmaxChannels);
        if(q%c) throw std::invalid_argument("BatchedProjection: channels must divide output_size.");
        population_math::size(input,n*d); population_math::size(output,n*q);
        values.realloc(n,d);
    }


    void
    Tick()
    {
        std::copy(input.contiguous_data(),input.contiguous_data()+n*d,values.contiguous_data());
        output.matmul(values,weights);
        float *out = output.contiguous_data();
        const float *offset = bias.contiguous_data();
        for(int i=0;i<n;++i) for(int j=0;j<q;++j) out[i*q+j]+=offset[j];
        if(c>1) for(int i=0;i<n;++i) for(int s=0;s<q/c;++s)
        {
            float maximum=-std::numeric_limits<float>::infinity();
            for(int j=0;j<c;++j) maximum=std::max(maximum,out[i*q+j*(q/c)+s]);
            double total=0;
            for(int j=0;j<c;++j) { const int k=j*(q/c)+s; out[i*q+k]=std::exp(out[i*q+k]-maximum); total+=out[i*q+k]; }
            for(int j=0;j<c;++j) out[i*q+j*(q/c)+s]/=total;
        }
    }
};

INSTALL_CLASS(BatchedProjection)
