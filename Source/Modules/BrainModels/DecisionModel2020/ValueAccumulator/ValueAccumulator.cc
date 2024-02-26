//
//		ValueAccumulator.cc		This file is a part of the IKAROS project
//				
//
//    Copyright (C) 2016 Christian Balkenius
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//


#include "ValueAccumulator.h"

using namespace ikaros;

void
ValueAccumulator::Init()
{

    Bind(alpha, "alpha");
    Bind(beta, "beta");
    Bind(gamma, "gamma");
    Bind(delta, "delta");
    Bind(lambda, "lambda");
    Bind(mean, "mean");
    Bind(sigma, "sigma");
    io(input, input_size, "INPUT");
    io(state, size, "STATE");
    io(index, size, "INDEX");
    io(output, "OUTPUT");
    io(choice, "CHOICE");
    io(rt_histogram, rt_size_x, rt_size_y, "RT-HISTOGRAM");
    io(rt_mean, "RT-MEAN");
    rt_sum = create_array(size);
    io(choice_count, "CHOICE-COUNT");
    io(choice_probability, "CHOICE-PROBABILITY");
    io(choice_ix, "CHOICE_IX");
}



void
ValueAccumulator::Tick()
{
    reset_array(choice, size);

    // force no negative values
    threshold_gteq(input, input, 0.f, input_size);

    // Apply update rule
    float E = sum(input, input_size);

 //   printf("%f\t%f\n", E*index[0], E*index[1]);
    
    for(int i=0; i<size; i++)
    {
        state[i] = (1-lambda)*state[i] + alpha*index[i]*E - beta*(1-index[i])*E + gamma*state[i];
    }

    // recurrent inhibition
    for(int i=0; i<size; i++)
    {
        for(int j=0; j<size; j++)
            if(i!=j)
                state[i] -= delta*state[j];
    }

    // Noise
    for(int i=0; i<size; i++)
        state[i] += gaussian_noise(mean, sigma);

    clip(state, 0, 1, size);

    reset_array(output, size);
    if(max(state, size) == 1)
    {
        int c = arg_max(state, size);
        output[c] = 1;
        if(reaction_time/5 < 40)
            rt_histogram[reaction_time/5][c] += 1;
        choice_count[c] += 1;
        copy_array(choice_probability, choice_count, size);
        normalize1(choice_probability, size);
        rt_sum[c] += reaction_time;
        for(int i=0; i<size; i++)
            if(choice_count[i]> 0)
                rt_mean[i] = rt_sum[i] / choice_count[i];
            else
                rt_mean[i] = 0;
        reset_array(state, size);
        set_one(choice, c, size);
        choice_ix[0] = c;
        printf( "chosen: %i\n", c);
        reaction_time = 0;
    }
    else
        reaction_time += 1;
}


void
ValueAccumulator::Command(std::string s, float x, float y, std::string value)
{
    if(s == "reset")
    {
        reset_matrix(rt_histogram, rt_size_x, rt_size_y);
        reset_array(rt_sum, size);
        reset_array(rt_mean, size);
        reset_array(state, size);
        reset_array(output, size);
        reset_array(choice_count, size);
        reset_array(choice_probability, size);
    }
}

static InitClass init("ValueAccumulator", &ValueAccumulator::Create, "Source/Modules/BrainModels/DecisionModel2020/ValueAccumulator/");


