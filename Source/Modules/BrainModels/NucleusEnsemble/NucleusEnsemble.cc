//
//	NucleusEnsemble.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2012 <Author Name>
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
//    See http://www.ikaros-project.org/ for more information.
//
//  This example is intended as a starting point for writing new Ikaros modules
//  The example includes most of the calls that you may want to use in a module.
//  If you prefer to start with a clean example, use he module MinimalModule instead.
//

#include "NucleusEnsemble.h"
#include <vector>
#include <iostream>
// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;

const float cDEFAULT_ADAPTRATE = 0.001f;
const float cDEFAULT_NA_THRESHOLD = 0.5f; // 2020-02-22: arbitarily set for now
const float cCLIPMIN = 0.01f; // min and max phivalues
const float cCLIPMAX = 100.f;

struct ring_buffer
{
    ring_buffer(std::size_t cap) : buffer(cap) {}
    bool empty() const { return sz == 0; }
    bool full() const { return sz == buffer.size(); }

    void push(float str)
    {
        if (last >= buffer.size())
            last = 0;
        buffer[last] = str;
        ++last;
        if (full())
            first = (first + 1) % buffer.size();
        else
            ++sz;
    }

    float operator[](std::size_t pos)
    {
        auto p = (first + pos) % buffer.size();
        return buffer[p];
    }

    float mean()
    {
        return ::mean(buffer.data(), buffer.size());
    }

    std::ostream &print(std::ostream &stm = std::cout) const
    {
        if (first < last)
            for (std::size_t i = first; i < last; ++i)
                std::cout << buffer[i] << ' ';
        else
        {
            for (std::size_t i = first; i < buffer.size(); ++i)
                std::cout << buffer[i] << ' ';
            for (std::size_t i = 0; i < last; ++i)
                std::cout << buffer[i] << ' ';
        }
        return stm;
    }

private:
    std::vector<float> buffer;
    std::size_t first = 0;
    std::size_t last = 0;
    std::size_t sz = 0;
};

void
NucleusEnsemble::Init()
{
    Bind(debug, "debug");
    Bind(ensemble_size, "size");
    Bind(alpha, "alpha");
    Bind(beta, "beta");

    Bind(phi, "phi");
    Bind(chi, "chi");
    Bind(psi, "psi");
    Bind(tau, "tau");
    
    Bind(scale, "scale");
    Bind(epsilon, "epsilon");
    Bind(sigma, "sigma");
    Bind(rho, "rho");
    Bind(default_threshold, "threshold");
    Bind(tetanus_factor, "tetanus_factor");
    Bind(tetanus_leak, "tetanus_leak");

    Bind(excitation_topology_name, "excitation_topology");
    Bind(inhibition_topology_name, "inhibition_topology");
    Bind(shunting_inhibition_topology_name, "shunting_inhibition_topology");
    Bind(recurrent_topology_name, "recurrent_topology");
    Bind(activation_function_name, "activation_function");
    Bind(avg_win_len, "moving_avg_window_size");

    io(excitation, excitation_size, "EXCITATION");
    io(inhibition, inhibition_size, "INHIBITION");
    io(shunting_inhibition, shunting_inhibition_size, "SHUNTING_INHIBITION");
    io(output, "OUTPUT");
    io(adenosine_out, "ADENO_OUTPUT");
    io(threshold, "THRESHOLD");


    int dummy_sz_x;
    int dummy_sz_y;
    int dummy_sz_z;
    io(ext_excitation_topology, dummy_sz_x, dummy_sz_y, "EX_TOPOLOGY");
    io(ext_inhibition_topology, dummy_sz_x, dummy_sz_y, "INH_TOPOLOGY");
    io(ext_shunting_topology, dummy_sz_x, dummy_sz_y, "SH_INH_TOPOLOGY");
    
    excitation_topology = create_matrix(excitation_size, ensemble_size);
    inhibition_topology = create_matrix(inhibition_size, ensemble_size);
    shunting_topology = create_matrix(shunting_inhibition_size, ensemble_size);
    recurrent_topology = create_matrix(ensemble_size, ensemble_size);

    io(setpoint, dummy_sz_x, "SETPOINT");
    io(adaptationrate, dummy_sz_x, "ADAPTATIONRATE");
    
    if(setpoint)
    {
        weights = create_array(ensemble_size);
        set_array(weights, 1.f, ensemble_size);
        if(!adaptationrate)
        {
            adaptationrate = create_array(ensemble_size);
            set_array(adaptationrate, cDEFAULT_ADAPTRATE, ensemble_size);
        }
    }
    if(!ext_excitation_topology && excitation)
        SetExcTopology(excitation_topology_name);
    if(!ext_inhibition_topology && inhibition)
        SetInhTopology(inhibition_topology_name);
    if(!ext_shunting_topology && shunting_inhibition)
        SetShInhTopology(shunting_inhibition_topology_name);
    SetRecTopology(recurrent_topology_name);

    io(dopamine, dummy_sz_x, "DOPAMINE");
    io(adenosine_in, dummy_sz_y, "ADENO_INPUT");
    io(noradrenaline, dummy_sz_z, "NORADRENALINE");
    io(na_threshold, dummy_sz_x, "NA_THRESHOLD");
    // threshold = create_array(ensemble_size);
    set_array(threshold, default_threshold, ensemble_size);
    //tetanus = create_array(ensemble_size);
    io(tetanus, "TETANUS");

    SetActivationFunc(activation_function_name);

    x = create_array(ensemble_size); // pre activation

    longtermavg = create_matrix(avg_win_len, ensemble_size);
    set_matrix(longtermavg, 1.f, avg_win_len, ensemble_size);
}



NucleusEnsemble::~NucleusEnsemble()
{
    // Destroy data structures that you allocated in Init.
    destroy_matrix(excitation_topology);
    destroy_matrix(inhibition_topology);
    destroy_matrix(shunting_topology);
    destroy_matrix(recurrent_topology);
    destroy_array(x);
    destroy_matrix(longtermavg);
    //destroy_array(tetanus);
}



void
NucleusEnsemble::Tick()
{
    // update topologies if external
    if(ext_excitation_topology)
        copy_matrix(excitation_topology, ext_excitation_topology, excitation_size, ensemble_size);
    if(ext_inhibition_topology)
        copy_matrix(inhibition_topology, ext_inhibition_topology, inhibition_size, ensemble_size);
    if(ext_shunting_topology)
        copy_matrix(shunting_topology, ext_shunting_topology, shunting_inhibition_size, ensemble_size);
    
    // array to hold sum of inputs dept on topology for each nucleus
    float *exc_sum = create_array(ensemble_size);
    float *inh_sum = create_array(ensemble_size);
    float *sh_inh_sum = create_array(ensemble_size);
    float *rec_sum = create_array(ensemble_size);
    float *dopa = create_array(ensemble_size);
    float *adeno = create_array(ensemble_size);
    float *norad = create_array(ensemble_size);
    float *na_thr = create_array(ensemble_size);

    if(excitation)
        multiply(exc_sum, excitation_topology, excitation, excitation_size, ensemble_size);
    if(inhibition)
        multiply(inh_sum, inhibition_topology, inhibition, inhibition_size, ensemble_size);
    if(shunting_inhibition)
        multiply(sh_inh_sum, shunting_topology, shunting_inhibition, shunting_inhibition_size, ensemble_size);
    multiply(rec_sum, recurrent_topology, output, ensemble_size, ensemble_size);

    if(scale)
    {
        if(excitation_size > 0)
            phi_scale = 1.0/float(excitation_size);

        if(inhibition_size > 0)
            chi_scale = 1.0/float(inhibition_size);

        if(shunting_inhibition_size > 0)
            psi_scale = 1.0/float(shunting_inhibition_size);
        tau_scale = 1.0/float(ensemble_size);
    }
    else
    {
        phi_scale = 1.0;
        chi_scale = 1.0;
        psi_scale = 1.0;
        tau_scale = 1.0;
    }

    float *phival = create_array(ensemble_size);
    set_array(phival, phi, ensemble_size);
    if(setpoint)
    {
        // TODO test this algo more: safe range of adaptation and buffer length
        // adapt weights so activation is closer to setpoint
        for(int i = 0; i < ensemble_size; i++)
        {
            // update excitation scale according to long term avg and
            // set point
            if(output[i] > 0.f) // only update when actually active
            {
                int ix = random(avg_win_len); // use rnd placement instead of queue
                longtermavg[i][ix] = output[i];
                float avg = mean(longtermavg[i], ensemble_size);
                weights[i] += (setpoint[i] - avg)*adaptationrate[i];
            }
        }
        
        copy_array(phival, clip(weights, cCLIPMIN, cCLIPMAX, ensemble_size), ensemble_size);
    }
    
    if(adenosine_in)
    {
        // TODO add caffeine inhibition
        for(int i = 0; i < ensemble_size; i++)
            adeno[i] = rho * adenosine_in[i]; // dopa inhibition
    }

    if(dopamine)
    {
        for(int i = 0; i < ensemble_size; i++)
            dopa[i] = sigma * dopamine[i]; // threshold reduction
    }

    if(noradrenaline)
    {
        for (int i = 0; i < ensemble_size; i++)
            norad[i] = noradrenaline[i]; // no modulation weight for now
        if(na_threshold)
            for (int i = 0; i < ensemble_size; i++)
                na_thr[i] = na_threshold[i];
        else
            set_array(na_thr, cDEFAULT_NA_THRESHOLD, ensemble_size);
        
    }


    // calculate threshold
    for(int i = 0; i < ensemble_size; i++)
    {
        float tmpthr = default_threshold + adeno[i];
        tmpthr -= dopa[i];
        threshold[i] = tmpthr;
        //float thr_modulation = dopa[i]-adeno[i];
        //thr_modulation = ( thr_modulation > 0) ? thr_modulation : 0;
        //threshold[i] = 1/(1 + thr_modulation) * default_threshold; // hyperbolic
        //threshold[i] = default_threshold - thr_modulation; // linear
        threshold[i] = (threshold[i] > 0) ? threshold[i] : 0;
    }
    
    for(int i = 0; i < ensemble_size; i++)
    {
        output[i] = 0.f;
        // calculate activation for each nucleus
        float s = 1/(1+psi*psi_scale*sh_inh_sum[i]); // shunting
        float a = phival[i] * phi_scale * s * exc_sum[i]; // excitation

        
        a += tau * tau_scale * rec_sum[i];      // recursion
        a -= chi * chi_scale * inh_sum[i];      // inhibition
        if (norad[i] >= na_thr[i])              // noradrenaline
            a += norad[i];
        else
            a -= norad[i];

        // TODO subthreshold integration to get
        // delays from threshold modulations (dopa, adeno)
        // note: shold probably affect rise time too
        tetanus[i] = (1-tetanus_leak)*tetanus[i] + tetanus_factor*a;
        
        x[i] += epsilon * (a - x[i]);       // rise time to max value

        
        float a_final = alpha + beta * (this->*Activate)(x[i]);
        if(tetanus[i] >= threshold[i])
            output[i] = a_final; //-threshold[i];
        else
            output[i] = alpha;
        adenosine_out[i] = output[i]; // TODO add multiplier here?
        

    }

    if(debug)
    {
        printf("\n\nInstance: %s\n", this->instance_name);
        print_matrix("long term avg", longtermavg, 10, ensemble_size, 2);
        printf("mean excitation: %f\n", mean(longtermavg[0], ensemble_size));
            print_array("phival", phival, ensemble_size, 2);
        print_array("x", x, ensemble_size, 2);
        //print_matrix("excitation_topology", excitation_topology, excitation_size, ensemble_size, 2);
        // print_array("dopa", dopa, ensemble_size);
        // print_array("adeno", adeno, ensemble_size);
        //print_array("threshold", threshold, ensemble_size);
        // print_array("norad", norad, ensemble_size);
        // print_array("na_thr", na_thr, ensemble_size);
        print_array("out", output, ensemble_size);
    }
    

    // clean up
    destroy_array(exc_sum);
    destroy_array(inh_sum);
    destroy_array(sh_inh_sum);
    destroy_array(rec_sum);
    destroy_array(phival);
    destroy_array(dopa);
    destroy_array(adeno);
}

float 
NucleusEnsemble::SecondOrder(float x)
{
    if(x < 0)
        return 0;
    else
        return 2*x*x/(1+x*x);
}

float
NucleusEnsemble::ScaledSigmoid(float x)
{
    if(x < 0)
        return 0;
    else
        return atan(x)/tan(1);    
}

float
NucleusEnsemble::ReLu(float x)
{
    if(x < 0)
        return 0;
    else
        return x;
}


/*
void 
NucleusEnsemble::NucleusTick()
{
    
    if(scale)
    {
        if(excitation_size > 0)
            phi_scale = 1.0/float(excitation_size);

        if(inhibition_size > 0)
            chi_scale = 1.0/float(excitation_size);

        if(shunting_inhibition_size > 0)
            psi_scale = 1.0/float(shunting_inhibition_size);
    }
    else
    {
        phi_scale = 1.0;
        chi_scale = 1.0;
        psi_scale = 1.0;
    }

    float a = 0;
    float s = 1;
    
    if(shunting_inhibition)
        s = 1/(1+psi*psi_scale*sum(shunting_inhibition, shunting_inhibition_size));

    if(excitation)
        a += phi * phi_scale * s * sum(excitation, excitation_size);

     if(inhibition)
        a -= chi * chi_scale * sum(inhibition, inhibition_size);

     x += epsilon * (a - x);
    
    // Activation function with
    // f(x) = 0 if x <= 0
    // f(x) = 1 if x = 1
    // f(x) -> 2 as x -> inf
    // S-shaped from 0+
    // derivative: 4x / (x^2+1)^2
    
    switch(1)
    {
        case 0:
            if(x < 0)
                *output = 0;
            else
                *output = 2*x*x/(1+x*x);
            break;
            
        case 1:
            if(x < 0)
                *output = 0;
            else
                *output = alpha + beta * atan(x)/tan(1);
            break;
    }
    // if external topologies, copy them in
	if(debugmode)
	{
		// print out debug info
	}
    
}
*/

void        
NucleusEnsemble::SetExcTopology(std::string atopology)
{
    // create topology matrix rows = ensemble size, cols = inputs
    
    if (atopology == "all_to_all" || ensemble_size==1)
    {
        set_matrix(excitation_topology, 1.f, excitation_size, ensemble_size);
    }
    else // one to one
    {
        // inputs must be same size as ensemble, otherwise crash
        // note 2020-02-10: if only single nucleus, only last
        // input will make any difference
        for(int i = 0; i < ensemble_size; i++)
            excitation_topology[i][i] = 1.f;
        
    }
    
}

void        
NucleusEnsemble::SetInhTopology(std::string atopology)
{
    if(atopology == "lateral")
    {
        // inhibit neighbors and roll around at the ends
    }
    else if(atopology == "wta")
    {
        // winner takes all: all to all inhibition
        set_matrix(inhibition_topology, 1.f, inhibition_size, ensemble_size);
    }
    else // one to one
        for(int i = 0; i < ensemble_size; i++)
            inhibition_topology[i][i] = 1.f;
        
}

void        
NucleusEnsemble::SetShInhTopology(std::string atopology)
{
    if(atopology == "lateral")
    {
        // inhibit neighbors and roll around at the ends
    }
    else if(atopology == "wta")
    {
        // winner takes all: all to all inhibition
        set_matrix(shunting_topology, 1.f, shunting_inhibition_size, ensemble_size);
    }
    else // one to one
        for(int i = 0; i < ensemble_size; i++)
            shunting_topology[i][i] = 1.f;
}


void        
NucleusEnsemble::SetRecTopology(std::string atopology)
{
    float val = 0;
    if(atopology=="excitatory")
        val = 1.f;
    if(atopology == "inhibitory")
        val = -1.f;
    // "none" means val = 0
    for(int i = 0; i < ensemble_size; i++)
            recurrent_topology[i][i] = val;

}

void        
NucleusEnsemble::SetActivationFunc(std::string afunc)
{
    if(afunc == "relu")
        Activate = &NucleusEnsemble::ReLu;
    else if(afunc == "secondorder")
        Activate = &NucleusEnsemble::SecondOrder;
    else // if(afunc == "scaledsigmoid")
        Activate = &NucleusEnsemble::ScaledSigmoid;
    

}



// Install the module. This code is executed during start-up.

static InitClass init("NucleusEnsemble", &NucleusEnsemble::Create, "Source/Modules/BrainModels/NucleusEnsemble/");


