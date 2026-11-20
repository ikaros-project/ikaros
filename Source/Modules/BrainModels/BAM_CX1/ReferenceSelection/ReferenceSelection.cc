#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "ikaros.h"
#include "../PopulationMath.h"

using namespace ikaros;

class ReferenceSelection: public Module
{
    parameter rows;
    parameter categories;
    parameter references;
    parameter latentSize;
    matrix input;
    matrix update;
    matrix output;
    matrix slots;
    matrix bank;
    matrix initialized;
    int n,c,r,d;
    void
    Init()
    {
        Bind(rows, "rows");
        Bind(categories, "categories");
        Bind(references, "references");
        Bind(latentSize, "latent_size");
        Bind(input, "INPUT");
        Bind(update, "UPDATE");
        Bind(output, "OUTPUT");
        Bind(slots, "SLOTS");
        Bind(bank, "BANK");
        Bind(initialized, "INITIALIZED");
        n=population_math::dimension(rows); c=population_math::dimension(categories); r=population_math::dimension(references); d=population_math::dimension(latentSize);
        population_math::size(input,n*d); population_math::size(update,1,true);
    }


    void
    Tick()
    {
        const float *x=input.contiguous_data();
        if(initialized(0)==0 || (!update.empty() && update(0)>0))
        {
            for(int b=0;b<n;++b) for(int j=0;j<c;++j)
            {
                double best=std::numeric_limits<double>::infinity(); int winner=0;
                for(int k=0;k<r;++k)
                {
                    double dist=0;
                    for(int l=0;l<d;++l) { const double e=x[b*d+l]-bank(j,k,l); dist+=e*e; }
                    if(dist<best) { best=dist; winner=k; }
                }
                slots(b,j)=winner;
            }
            initialized(0)=1;
        }
        for(int b=0;b<n;++b) for(int j=0;j<c;++j)
        {
            const int k=slots(b,j);
            if(k<0 || k>=r) throw std::invalid_argument("ReferenceSelection: stored slot out of range.");
            for(int l=0;l<d;++l) output(b,j,l)=bank(j,k,l);
        }
    }
};

INSTALL_CLASS(ReferenceSelection)
