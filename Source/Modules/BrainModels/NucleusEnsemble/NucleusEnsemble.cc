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

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;

const float cDEFAULT_ADAPTRATE = 0.001f;
void
NucleusEnsemble::Init()
{
    Bind(ensemble_size, "size");
    Bind(alpha, "alpha");
    Bind(beta, "beta");

    Bind(phi, "phi");
    Bind(chi, "chi");
    Bind(psi, "psi");
    Bind(tau, "tau");
    
    Bind(scale, "scale");
    Bind(epsilon, "epsilon");

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

    int dummy_sz_x;
    int dummy_sz_y;
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

    SetActivationFunc(activation_function_name);

    x = create_array(ensemble_size); // pre activation

    longtermavg = create_matrix(avg_win_len, ensemble_size);
    set_matrix(longtermavg, 0.5f, avg_win_len, ensemble_size);
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
        // adapt weights so activation is closer to setpoint
        for(int i = 0; i < ensemble_size; i++)
        {
            // update excitation scale according to long term avg and
            // set point
            int ix = random(avg_win_len); // use rnd placement instead of queue
            longtermavg[i][ix] = output[i];
            float avg = mean(longtermavg[i], ensemble_size);
            weights[i] += (avg - setpoint[i])*adaptationrate[i];
        }
        copy_array(phival, weights, ensemble_size);
    }

    for(int i = 0; i < ensemble_size; i++)
    {
        // calculate activation for each nucleus
        float s = 1/(1+psi*psi_scale*sh_inh_sum[i]); // shunting
        float a = phival[i] * phi_scale * s * exc_sum[i]; // excitation
        a += tau * tau_scale * rec_sum[i];      // recursion
        a -= chi * chi_scale * inh_sum[i];      // inhibition
        x[i] += epsilon * (a - x[i]);           // effect of previous
        output[i] = alpha + beta * (this->*Activate)(x[i]);

    }

    
    

    // clean up
    destroy_array(exc_sum);
    destroy_array(inh_sum);
    destroy_array(sh_inh_sum);
    destroy_array(rec_sum);
    destroy_array(phival);
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
    
    if (atopology == "all_to_all")
    {
        set_matrix(excitation_topology, 1.f, excitation_size, ensemble_size);
    }
    else // one to one
    {
        // inputs must be same size as ensemble, otherwise crash
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


