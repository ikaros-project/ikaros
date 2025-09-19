#include "ikaros.h"

#include <math.h>

using namespace ikaros;

class Cerebellum: public Module
{
    parameter learning_rate;

    parameter isi_mu;
    parameter isi_sigma;
    parameter isi_tau;

    matrix input;
    matrix target;
    matrix output;
    matrix state;


    void Init()
    {
        Bind(learning_rate, "learning_rate");
        Bind(isi_mu, "isi_mu");
        Bind(isi_sigma, "isi_sigma");
        Bind(isi_sigma, "isi_tau");

        Bind(input, "INPUT");
        Bind(state, "STATE");
        Bind(target, "TARGET");
        Bind(output, "OUTPUT");
    }


    void Tick()
    {
        state = 0;
        double td = GetTickDuration();
        //for(int i=0; i<input.size(); i++)
        //    state += exgaussian(td*double(i), K, isi_mu, isi_sigma) * input[i];  // Convolve input with ex-gaussian kernel  g(x, K, mu, sigma)


        //output = x;
    }
};

INSTALL_CLASS(Cerebellum)

