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

#include "SpikingPopulation.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;

const float cMinRand = 0.001f;
const float cMaxRand = 0.999f;
const float cAPCost = 1; // arbitrary number representing million adenosine excuded per action potential

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
	Bind(debugmode, "debug");    

    excitation_array = GetInputArray("EXCITATION_IN");
    inhibition_array = GetInputArray("INHIBITION_IN");
    direct_input = GetInputArray("DIRECT_IN");
    excitation_size = GetInputSize("EXCITATION_IN");
    inhibition_size = GetInputSize("INHIBITION_IN");
    // TODO check that excitation and inhibition are same size

    output_array = GetOutputArray("OUTPUT");
    output_array_size = GetOutputSize("OUTPUT");
    adenosine_out = GetOutputArray("ADENOSINE");

    // create synapses
    synapse = create_matrix(excitation_size + inhibition_size, population_size);
    exc_syn = create_matrix(excitation_size, population_size);
    inh_syn = create_matrix(inhibition_size, population_size);
    random(exc_syn, cMinRand, cMaxRand, 
        excitation_size , population_size);
    random(inh_syn, -cMaxRand, -cMinRand, 
        inhibition_size, population_size);
    
    // set up neuron type params
    Iz_Params pyramidcell;

    pyramidcell.a[0] = 0.02f;
    pyramidcell.a[1] = 0.001f;
    pyramidcell.b[0] = 0.2f;
    pyramidcell.b[1] = 0.001f;
    pyramidcell.c[0] = -65.f;
    pyramidcell.c[1] = 5.f;
    pyramidcell.d[0] = 8.f;
    pyramidcell.d[1] =-6.f;

    Iz_Params interneuron;

    interneuron.a[0] = 0.02f;
    interneuron.a[1] = 0.02f;
    interneuron.b[0] = 0.25f;
    interneuron.b[1] = -0.05f;
    interneuron.c[0] = -65.f;
    interneuron.c[1] = 0.5f;
    interneuron.d[0] = 2.f;
    interneuron.d[1] = 0.5f;

    param_a = create_array(population_size);
    param_b = create_array(population_size);
    param_c = create_array(population_size);
    param_d = create_array(population_size);

    vlt = create_array(population_size);
    add(vlt, -65.f, population_size);
    

    // TODO initialize population abcd values
    float *rndfact = random(create_array(population_size), cMinRand, cMaxRand, 
        population_size);
    
    u = create_array(population_size);
    // TODO add switch for model type
    for(int i = 0; i < population_size; i++)
    {
        switch(neurontype)
        {
            case eInter:
                param_a[i] = interneuron.a[0] + interneuron.a[1]*rndfact[i];
                param_b[i] = interneuron.b[0] + interneuron.b[1]*rndfact[i];
                param_c[i] = interneuron.c[0] + interneuron.c[1]*rndfact[i]*rndfact[i];
                param_d[i] = interneuron.d[0] + interneuron.d[1]*rndfact[i]*rndfact[i];
                break;
            default:
            case ePyramidal:
                param_a[i] = pyramidcell.a[0] + pyramidcell.a[1]*rndfact[i];
                param_b[i] = pyramidcell.b[0] + pyramidcell.b[1]*rndfact[i];
                param_c[i] = pyramidcell.c[0] + pyramidcell.c[1]*rndfact[i]*rndfact[i];
                param_d[i] = pyramidcell.d[0] + pyramidcell.d[1]*rndfact[i]*rndfact[i];
                break;
        }
        u[i] = param_b[i] * vlt[i];
    }
    

    destroy_array(rndfact);

}



SpikingPopulation::~SpikingPopulation()
{
    // Destroy data structures that you allocated in Init.
    destroy_array(param_a);
    destroy_array(param_b);
    destroy_array(param_c);
    destroy_array(param_d);

    destroy_matrix(synapse);
    destroy_matrix(exc_syn);
    destroy_matrix(inh_syn);

    destroy_array(vlt);
    destroy_array(u);
    
}



void
SpikingPopulation::Tick()
{
    // update input synapses
    float **exc_syn_tmp = create_matrix(excitation_size, population_size);
    float **inh_syn_tmp = create_matrix(inhibition_size, population_size);
    tile(exc_syn_tmp[0], excitation_array, population_size, excitation_size);
    tile(inh_syn_tmp[0], inhibition_array, population_size, inhibition_size);
    multiply(exc_syn_tmp, exc_syn, excitation_size, population_size);
    multiply(inh_syn_tmp, inh_syn, inhibition_size, population_size);
    // compose complete synapse
    set_submatrix(synapse[0], excitation_size+inhibition_size, 
                    exc_syn_tmp[0], population_size, excitation_size, 0, 0);
    set_submatrix(synapse[0], excitation_size+inhibition_size,
                    inh_syn_tmp[0], population_size, inhibition_size, 
                    0, excitation_size);
    
    switch(modeltype)
    {
        default:
        case eIzhikevich:
        {   
            TimeStep_Iz(    param_a,
                            param_b,
                            param_c,
                            param_d,
                            synapse,
                            direct_input,
                            vlt,
                            u
                            
                            );
        }
    }
    // update adenosine
    adenosine_out[0] = numfired * cAPCost;
    copy_array(output_array, vlt, population_size);

	if(debugmode)
	{
		// print out debug info
	}
}

void
SpikingPopulation::TimeStep_Iz( float *a_a, 
                                float *a_b, 
                                float *a_c, 
                                float *a_d, 
                                float **a_syn,
                                float *a_i, 
                                float *a_v, 
                                float *a_u 
                                )
{
    float *fired = create_array(population_size);
    for(int i=0; i < population_size; ++i)
        if(a_v[i] >= threshold)
            fired[i] = 1.f;
    float *v1 = create_array(population_size);
    copy_array(v1, a_v, population_size);
    float *u1 = create_array(population_size);
    copy_array(u1, a_u, population_size);
    float *i1 = create_array(population_size);
    numfired = sum(fired, population_size);
    int synsize_x = excitation_size + inhibition_size;
    float **syn_fired = create_matrix(synsize_x, population_size);
    
    for(int j = 0; j < population_size; j++)
    {
        if(fired[j] == 1.f)
        {
            a_v[j] = threshold;
            v1[j] = a_c[j];
            u1[j] = a_u[j] + a_d[j];

        }
        
        for(size_t i = 0; i < excitation_size + inhibition_size; i++)
        {
            if(a_syn[j][i] >= threshold)
                syn_fired[j][i] = a_syn[j][i];
        }
    }
    float *tmp = create_array(population_size);
    sum(tmp, syn_fired, synsize_x, population_size, 1); // sum along x axis
    add(i1, tmp, a_i, population_size ); // get input current

    // calculate output voltage
    float stepfact = 1.f/substeps;
    for(int step = 0; step < substeps; step++)
    {
        for(int i = 0; i < population_size; i++)
        {
            // v1=v1+(1.0/substeps)*(0.04*(v1**2)+(5*v1)+140-u + i1) 
            v1[i] += stepfact*(0.04f*pow(v1[i], 2) + 5*v1[i] + 
                        140-a_u[i] + i1[i]);
            // u1=u1+(1.0/substeps)*a*(b*v1-u1)               
            u1[i] += stepfact*(a_a[i]*(a_b[i]*v1[i] - u1[i]));
        }
    }

    // clip at threshold
    for(int i = 0; i < population_size; i++)
    {
        if(v1[i] >= threshold)
            v1[i] = threshold;
    }
    
    copy_array(a_v, v1, population_size);
    copy_array(a_u, u1, population_size);

    destroy_array(v1);
    destroy_array(u1);
    destroy_array(i1);
    destroy_array(tmp);
    destroy_matrix(syn_fired);
        
}


// Install the module. This code is executed during start-up.

static InitClass init("SpikingPopulation", &SpikingPopulation::Create, "Source/Modules/BrainModels/SpikingPopulation/");


