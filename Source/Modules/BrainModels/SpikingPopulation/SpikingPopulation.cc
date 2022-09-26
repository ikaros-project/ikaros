//
//	SpikingPopulation.cc		This file is a part of the IKAROS project
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

// TODO 2020-11-01:
// 1) add support for connection topology
// 2) fix excitation, inhibition

#include "SpikingPopulation.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;

//const float minrand = 0.5001f;
//const float maxrand = 1.499f;
const float cDirectCurrentScale = 1; //100.f;
const float cMinVolt = -65.f;

// void
// SpikingPopulation::SetSizes()
// {
//     SetOutputSize("ADENOSINE", 1);
//     population_size = GetIntValue("population_size");
//     SetOutputSize("OUTPUT", population_size);
// }

void
SpikingPopulation::Init()
{
    int model = GetIntValue("model_type");
    int ntype = GetIntValue("neuron_type");
    modeltype = (ModelType) model;
    neurontype = (NeuronType) ntype;
    population_size = GetIntValue("population_size");
    Bind(substeps, "substeps");
    Bind(threshold, "threshold");
    Bind(adenosine_factor, "adenosine_factor");
	Bind(debugmode, "debug");    
    Bind(minrand, "synapse_min");
    Bind(maxrand, "synapse_max");


    io(excitation_array, excitation_size, "EXCITATION");
    io(inhibition_array, inhibition_size, "INHIBITION");
    io(direct_input, direct_in_size, "DIRECT_IN");
    if(direct_input && direct_in_size > population_size)
        Notify(msg_fatal_error, "Direct in size cannot be larger than population size");

    
    
    // TODO add ext inh external topologies
    internal_topology = create_matrix(population_size, population_size);
    int dummy_x, dummy_y;
    io(internal_topology_inp, dummy_x, dummy_y, "INTERNAL_TOPOLOGY");
    if(internal_topology_inp && (dummy_x != population_size || dummy_y!=population_size)){
        printf("Topology size x = %i, y = %i; population: %i\n", dummy_x, dummy_y, population_size);
        Notify(msg_fatal_error, "Internal topology must be square equal to population size");
    }
    io(excitation_topology, dummy_x, dummy_y, "EXCITATION_TOPOLOGY");
    if(excitation_topology && (dummy_x != excitation_size || dummy_y != population_size)){
        printf("Exc topology (%i, %i); exc size = %i, pop size = %i\n", dummy_x, dummy_y, excitation_size, population_size);
        Notify(msg_fatal_error, "Excitation topology must be excitation size x population size");
    }
    io(inhibition_topology, dummy_x, dummy_y, "INHIBITION_TOPOLOGY");
    if(inhibition_topology && (dummy_x != inhibition_size || dummy_y != population_size)){
        printf("Inh topology (%i, %i); inhibition size = %i, pop size = %i\n", dummy_x, dummy_y, inhibition_size, population_size);
        Notify(msg_fatal_error, "Inhibition topology must be inhibition size x population size");
    }

    io(output_array, output_array_size, "OUTPUT");
    io(adenosine_out, "ADENO_OUTPUT");

    // create synapses/topologies
    int_synapse = create_matrix(population_size, population_size);
    if(excitation_array)
    {
        exc_syn = create_matrix(excitation_size, population_size);
        random(exc_syn, minrand, maxrand, 
            excitation_size , population_size);
    }
    if(inhibition_array)
    {
        inh_syn = create_matrix(inhibition_size, population_size);
        random(inh_syn, maxrand, minrand, 
            inhibition_size, population_size);
    }

    taurecovery = create_array(population_size); // tau_recovery
    coupling = create_array(population_size); // coupling
    resetvolt = create_array(population_size); // reset voltage
    resetrecovery = create_array(population_size); // reset recovery

    vlt = create_array(population_size);
    add(vlt, cMinVolt, population_size); 
    

    // TODO initialize population abcd values
    // float *rndfact = random(create_array(population_size), minrand, maxrand, 
    //     population_size);
    
    u = create_array(population_size);
    // TODO add switch for model type
    for(int i = 0; i < population_size; i++)
    {
        switch(neurontype)
        {
            // values taken from Izhivich 2003
            case eIntrinsically_bursting:
                taurecovery[i] = 0.02f;
                coupling[i] = 0.2f;
                resetvolt[i] = -55;
                resetrecovery[i] = 4;
                break;
            case eChattering:
                taurecovery[i] = 0.02f;
                coupling[i] = 0.2f;
                resetvolt[i] = -50;
                resetrecovery[i] = 2;
                break;
            case eFast_spiking:
                taurecovery[i] = 0.1f;
                coupling[i] = 0.2f;
                resetvolt[i] = -65;
                resetrecovery[i] = 2;

                break;
            case eLow_threshold:
                taurecovery[i] = 0.1f;
                coupling[i] = 0.25f;
                resetvolt[i] = -65.f;
                resetrecovery[i] = 2.f;

                break;
            case eResonator:
                taurecovery[i] = 0.1f;
                coupling[i] = 0.26f;
                resetvolt[i] = 65;
                resetrecovery[i] = 2;

                break;
            default:
            case eRegular_spiking:
                taurecovery[i] = 0.02f;
                coupling[i] = 0.2f;
                resetvolt[i] = -65.f;
                resetrecovery[i] = 8;
                break;
        }
        u[i] = coupling[i] * vlt[i];
    }
    

    // destroy_array(rndfact);

}



SpikingPopulation::~SpikingPopulation()
{
    // Destroy data structures that you allocated in Init.
    destroy_array(taurecovery);
    destroy_array(coupling);
    destroy_array(resetvolt);
    destroy_array(resetrecovery);

    destroy_matrix(int_synapse);
    destroy_matrix(exc_syn);
    destroy_matrix(inh_syn);
    destroy_matrix(internal_topology);

    destroy_array(vlt);
    destroy_array(u);
    
}



void
SpikingPopulation::Tick()
{
    // update input synapses
    float **exc_syn_tmp = NULL; 
    float **inh_syn_tmp = NULL; 
    float *dir_in_tmp = create_array(population_size); 
    float **int_exc_syn_tmp = NULL;
    float **int_inh_syn_tmp = NULL;
    int composed_ext_syn_length = 0;
    int composed_inh_syn_length = 0;

    
    if(excitation_array != NULL)
    {
        exc_syn_tmp = create_matrix(excitation_size, population_size);
        // weed out nonspiking input and scale by synapse
        threshold_gteq(excitation_array, excitation_array, threshold, excitation_size);
        tile(exc_syn_tmp[0], excitation_array, population_size, excitation_size);
        if(excitation_topology != NULL)
            multiply(exc_syn_tmp, excitation_topology, excitation_size, population_size);
        multiply(exc_syn_tmp, exc_syn, excitation_size, population_size);
        composed_ext_syn_length += excitation_size;
    }
    if(inhibition_array != NULL)
    {
        inh_syn_tmp = create_matrix(inhibition_size, population_size);
        // weed out nonspiking input and scale by synapse
        threshold_gteq(inhibition_array, inhibition_array, threshold, inhibition_size);
        tile(inh_syn_tmp[0], inhibition_array, population_size, inhibition_size);
        if(inhibition_topology != NULL)
            multiply(inh_syn_tmp, inhibition_topology, inhibition_size, population_size);
        multiply(inh_syn_tmp, inh_syn, inhibition_size, population_size);
        composed_inh_syn_length += inhibition_size;

    }
    if(direct_input != NULL)
        copy_array(dir_in_tmp, direct_input, direct_in_size);
    {

        // copy_array(dir_in_tmp, direct_input, population_size);
        multiply(dir_in_tmp, cDirectCurrentScale, population_size);

    }
    if(internal_topology_inp != NULL)
        copy_matrix(internal_topology, internal_topology_inp, population_size, population_size);
    // always do internal topology even if its filled with 0
    {
        // excitation
        int_exc_syn_tmp = create_matrix(population_size, population_size);
        threshold_gt(int_exc_syn_tmp[0], internal_topology[0], 0.f, population_size*population_size);
        float *output_spikes = create_array(population_size);
        threshold_gteq(output_spikes, output_array, threshold, population_size);
        float **output_tiled = create_matrix(population_size, population_size);
        tile(output_tiled[0], output_spikes, population_size, population_size);
        // if this scales down spike amplitude below threshold, will be missed below
        multiply(int_exc_syn_tmp, output_tiled, population_size, population_size);
        composed_ext_syn_length += population_size;

        // inhibition
        int_inh_syn_tmp = create_matrix(population_size, population_size);
        threshold_lt(int_inh_syn_tmp[0], internal_topology[0], 0.f, population_size*population_size);
        multiply(int_inh_syn_tmp, output_tiled, population_size, population_size);
        composed_inh_syn_length += population_size;
        
        destroy_array(output_spikes);
        destroy_matrix(output_tiled);

    }
    float ** composed_ext_t = int_exc_syn_tmp;
    if(exc_syn_tmp)
    {
        composed_ext_t = create_matrix(composed_ext_syn_length, population_size);
        hstack(composed_ext_t, 
                    exc_syn_tmp, 
                    excitation_size,
                    int_exc_syn_tmp,
                    population_size,
                    population_size);
    }
    

    float **composed_inh_t = int_inh_syn_tmp;
    if(inh_syn_tmp)
    {
        composed_inh_t = create_matrix(composed_inh_syn_length, population_size);
        hstack(composed_inh_t,
                    inh_syn_tmp,
                    inhibition_size,
                    int_inh_syn_tmp,
                    population_size,
                    population_size);
    }

    switch(modeltype)
    {
        default:
        case eIzhikevich:
        {   
            TimeStep_Iz(    taurecovery,
                            coupling,
                            resetvolt,
                            resetrecovery,
                            composed_ext_t, // excitation
                            composed_ext_syn_length,
                            composed_inh_t,
                            composed_inh_syn_length,
                            dir_in_tmp,
                            vlt, // in output
                            u
                            
                            );
        }
    }
    // update adenosine
    adenosine_out[0] = numfired * adenosine_factor;
    copy_array(output_array, vlt, population_size);

	if(debugmode)
	{
		// print out debug info
        printf("\n\n---Module %s\n", GetName());
        if(excitation_array){
            print_array("exc_in: ", excitation_array, excitation_size);
            print_matrix("exc_syn: ", exc_syn_tmp, excitation_size, population_size);
        }
        if(excitation_topology)
            print_matrix("exc topology: ", excitation_topology, excitation_size, population_size);
        if(inhibition_array){
            print_array("inh_in: ", inhibition_array, inhibition_size);
            print_matrix("inh_syn", inh_syn_tmp, inhibition_size, population_size);
        }
        if(inhibition_topology)
            print_matrix("inh topology: ", inhibition_topology, inhibition_size, population_size);
        
        if(direct_input)
            print_array("direct_in: ", dir_in_tmp, population_size);
        //print_matrix("internal synapse", int_synapse, population_size, population_size);
        if(internal_topology_inp){
            print_matrix("internal topology: ", internal_topology_inp, population_size, population_size);
            print_matrix("int_inh_top_syn: ", int_inh_syn_tmp, population_size, population_size);
        }
        
	}

    destroy_array(dir_in_tmp);
    if(exc_syn_tmp)
        destroy_matrix(exc_syn_tmp);
    if(inh_syn_tmp)
        destroy_matrix(inh_syn_tmp);
    if(int_exc_syn_tmp != composed_ext_t)
        destroy_matrix(composed_ext_t);
    if(int_inh_syn_tmp != composed_inh_t)
        destroy_matrix(composed_inh_t);
    destroy_matrix(int_exc_syn_tmp);
    destroy_matrix(int_inh_syn_tmp);
    
}

void
SpikingPopulation::TimeStep_Iz( float *a_a, // tau recovery
                                float *a_b, // coupling
                                float *a_c, // reset voltage
                                float *a_d, // reset recovery
                                float **e_syn, // synapse - contains excitation scaled by synaptic vals, or zero if not spiked
                                int e_syn_x, // x length of exc synapse
                                float **i_syn, // synapse - contains excitation scaled by synaptic vals, or zero if not spiked
                                int i_syn_x, // x length of inh synapse
                                float *a_i, // direct current
                                float *a_v, // in out excitation
                                float *a_u  // recovery
                                )
{
    float *fired = create_array(population_size);
    for(int i=0; i < population_size; ++i) 
        if(a_v[i] >= threshold)
        {
            fired[i] = 1.f;
            //numfired += 1;
        }
    numfired = sum(fired, population_size); // used for adenosine calc
    
    float *v1 = create_array(population_size);
    copy_array(v1, a_v, population_size);
    
    float *u1 = create_array(population_size);
    copy_array(u1, a_u, population_size);
    
    float *i1 = create_array(population_size);
    // int synsize_x = excitation_size + inhibition_size;
    // float **syn_fired = create_matrix(synsize_x, population_size);

    //float *tmp = create_array(population_size);
    //sum(tmp, a_syn, synsize_x, population_size, 1); // sum along x axis

    // TODO convert to matrix by weeding out (gteq) and summing along x
    float *inputvlt = create_array(population_size);
    for(int j = 0; j < population_size; j++)
    {
        if(fired[j] == 1.f)
        {
            a_v[j] = threshold;
            v1[j] = a_c[j];
            u1[j] = a_u[j] + a_d[j];
        }
        // sum up 
        for(int i = 0; i < e_syn_x; i++)
        {
            //if(e_syn[j][i] >= threshold) // TAT 2022-09-26: removed since spikes already gated above; result is scaled by synaptic weights
            inputvlt[j] += e_syn[j][i];
        }
        for(int i = 0; i< i_syn_x; i++)
        {
            //if(i_syn[j][i] >= threshold)
            inputvlt[j]-= i_syn[j][i];
        }
    }
    //float *tmp = create_array(population_size);
    //sum(tmp, syn_fired, synsize_x, population_size, 1); // sum along x axis
    //sum(tmp, a_syn, synsize_x, population_size, 1); // sum along x axis
    
    add(i1, inputvlt, a_i, population_size ); // add direct current
  // calculate output voltage
    float stepfact = 1.f/substeps;
    for(int i = 0; i < population_size; i++)
    {
        for(int step = 0; step < substeps; step++)
        {
            // v1=v1+(1.0/substeps)*(0.04*(v1**2)+(5*v1)+140-u + i1) 
            // 
            v1[i] += stepfact*(0.04f*pow(v1[i], 2) + 5*v1[i] + 
                        140-a_u[i] + i1[i]);
            // u1=u1+(1.0/substeps)*a*(b*v1-u1)                  
        }
        // u1[i] += stepfact*(a_a[i]*(a_b[i]*v1[i] - u1[i]));
        u1[i] += (a_a[i]*(a_b[i]*v1[i] - u1[i]));
    }
    // multiply(v1, 100.f, population_size);
    // clip at threshold
    for(int i = 0; i < population_size; i++)
    {
        if(v1[i] >= threshold)
            v1[i] = threshold;
    }
    if(debugmode)
	{
      //print_matrix("syn_fired: ", a_syn, synsize_x, population_size);
      print_array("i1: ", i1, population_size);
      print_array("inputvlt: ", inputvlt, population_size);
    }
    
    copy_array(a_v, v1, population_size); // output voltage
    copy_array(a_u, u1, population_size); // output recovery

    destroy_array(v1);
    destroy_array(u1);
    destroy_array(i1);
    //destroy_array(tmp);
    destroy_array(fired);
    // destroy_matrix(syn_fired);
        
}



// Install the module. This code is executed during start-up.

static InitClass init("SpikingPopulation", &SpikingPopulation::Create, "Source/Modules/BrainModels/SpikingPopulation/");


