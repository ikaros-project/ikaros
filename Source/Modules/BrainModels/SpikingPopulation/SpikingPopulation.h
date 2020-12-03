//
//	SpikingPopulation.h		This file is a part of the IKAROS project
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

#ifndef SpikingPopulation_
#define SpikingPopulation_

#include "IKAROS.h"
// #include <map>

class SpikingPopulation: public Module
{
public:
    static Module * Create(Parameter * p) { return new SpikingPopulation(p); }
    enum ModelType {eIzhikevich=0};
    enum NeuronType {eRegular_spiking=0, eIntrinsically_bursting, eChattering, eFast_spiking, eLow_threshold, eResonator};
    //typedef struct {
    //    float a[2]; // time scale of recovery
    //    float b[2]; // sensitivity of recovery to subthreshold oscillations
    //    float c[2]; // after-spike reset value of v
    //    float d[2]; // after-spike reset increment of u
    //} Iz_Params;

    SpikingPopulation(Parameter * p) : Module(p) {}
    virtual ~SpikingPopulation();
    //virtual void        SetSizes();
    void 		Init();
    void 		Tick();
    void        TimeStep_Iz(    float *a_a, 
                                float *a_b, 
                                float *a_c, 
                                float *a_d, 
                                float **e_syn,
                                int e_syn_x,
                                float **i_syn,
                                int i_syn_x,
                                float *a_i,
                                float *a_v, 
                                float *a_u 
                                );
    void        ComposeMatrices(float **retval, 
                                float **a, 
                                int ax, 
                                float **b, 
                                int bx, 
                                int rety);
    // pointers to inputs and outputs
    // and integers to represent their sizes

    float *     excitation_array;
    int         excitation_size;
    float *     inhibition_array;
    int         inhibition_size;
    float *     direct_input;
    int         direct_in_size;
    //float **    excitation_topology;
    float **    internal_topology;

    float *     output_array;
    int         output_array_size;
    float *     adenosine_out;



    // internal data storage
    float *     vlt;
    float *     u;
    float **    int_synapse;
    float **    exc_syn;
    float **    inh_syn;
    float       numfired;
    float *     taurecovery;
    float *     coupling;
    float *     resetvolt;
    float *     resetrecovery;
    
    //std::map<NeuronType, Iz_Params> iz_neuron_params;

    // parameter values
    ModelType modeltype;
    NeuronType neurontype;
    int population_size;
    int substeps;
    float threshold;
    float adenosine_factor;    
    float minrand;
    float maxrand;
	bool       	debugmode;
};

#endif
