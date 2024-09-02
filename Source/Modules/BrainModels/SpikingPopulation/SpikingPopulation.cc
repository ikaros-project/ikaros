#include "ikaros.h"
// #include "strstream"
using namespace ikaros;

const float cDirectCurrentScale = 1; //100.f;
const float cMinVolt = -65.f;

class SpikingPopulation: public Module
{
    // internals
    matrix taurecovery;
    matrix coupling;
    matrix resetvolt;
    matrix resetrecovery;
    matrix internal_topology;
    matrix internal_topology_inp;
    matrix excitation_topology;
    matrix inhibition_topology;

    matrix vlt;
    matrix u;
    
    // parameters
    
    parameter population_size;
    parameter neuron_type;
    parameter model_type;
    parameter substeps;
    parameter threshold;
    parameter debugmode;

    matrix output;
    matrix direct_input;
    matrix excitation;
    matrix inhibition;

    enum ModelType {eIzhikevich=0};
    enum NeuronType {eRegular_spiking=0, eIntrinsically_bursting, eChattering, eFast_spiking, eLow_threshold, eResonator};
    

    void Init()
    {
        std::cout << "SpikingPopulation::Init() 1)\n";
        // parameters
        
        Bind(population_size, "population_size");
        Bind(model_type, "model_type");
        Bind(neuron_type, "neuron_type");
        Bind(substeps, "substeps");
        Bind(threshold, "threshold");
        Bind(debugmode, "debugmode");
        
        // io
        Bind(excitation, "EXCITATION");
        Bind(inhibition, "INHIBITION");
        Bind(output, "OUTPUT"); 
        Bind(direct_input, "DIRECT_IN"); // note has to be same size as pop size
        Bind(excitation_topology, "EXCITATION_TOPOLOGY");
        Bind(internal_topology_inp, "INTERNAL_TOPOLOGY");

        internal_topology = matrix(population_size.as_int(), population_size.as_int());

        std::cout << "SpikingPopulation::Init() 2)\n";
        if(!direct_input.empty() && direct_input.size() != population_size.as_int()) {
            std::ostringstream oss;
            oss << "SpikingPopulation::Init(): Direct in size " 
                << direct_input.size()
                << " cannot be different than population size "
                << population_size.as_int()
                << ". Init aborting. \n";
            std::cout << oss.str();
            throw exception(oss.str()); // TODO: remove and allow fix?
        } else {
            std::cout << "SpikingPopulation::Init(): Direct in size " 
                << direct_input.size()
                << "; population size: "
                << population_size.as_int()
                << "\n";
        }
        std::cout << "SpikingPopulation::Init() 3)\n";
        
        // initialise matrices
        internal_topology = matrix(population_size.as_int(), population_size.as_int());
        internal_topology.set_name("internal topology");
        taurecovery = matrix(population_size.as_int());
        coupling = matrix(population_size.as_int());
        resetvolt = matrix(population_size.as_int());
        resetrecovery = matrix(population_size.as_int());
        
        u = matrix(population_size.as_int());
        u.set_name("u");
        vlt = matrix(population_size.as_int());
        vlt.set(cMinVolt);
        vlt.set_name("vlt");

        switch(neuron_type.as_int())
        {
            // values taken from Izhivich 2003
            case eIntrinsically_bursting:
                taurecovery.set(0.02f);
                coupling.set(0.2f);
                resetvolt.set(-55);
                resetrecovery.set(4);
                break;
            case eChattering:
                taurecovery.set(0.02f);
                coupling.set(0.2f);
                resetvolt.set(-50);
                resetrecovery.set(2);
                break;
            case eFast_spiking:
                taurecovery.set(0.1f);
                coupling.set(0.2f);
                resetvolt.set(-65);
                resetrecovery.set(2);

                break;
            case eLow_threshold:
                taurecovery.set(0.1f);
                coupling.set(0.25f);
                resetvolt.set(-65.f);
                resetrecovery.set(2.f);

                break;
            case eResonator:
                taurecovery.set(0.1f);
                coupling.set(0.26f);
                resetvolt.set(65);
                resetrecovery.set(2);

                break;
            default:
            case eRegular_spiking:
                taurecovery.set(0.02f);
                coupling.set(0.2f);
                resetvolt.set(-65.f);
                resetrecovery.set(8);
                break;
        }
        u.multiply(coupling, vlt);
        
        // printf("u 1)\n");
        // u.info();
        std::cout << "SpikingPopulation::Init() complete.\n";
    }


    void Tick()
    {
        // TODO implement topologies for excitation, inhibition, internal
        matrix composed_ext_t;
        matrix composed_inh_t;
        matrix dir_in_tmp (population_size.as_int()); 
        matrix exc_syn_tmp; // excitatory synapse tmp
        matrix inh_syn_tmp; // inhibitory synapse tmp
        matrix int_exc_syn_tmp; // internal excitatory synapse tmp
        matrix int_inh_syn_tmp; // internal inhibitory synapse tmp
        int composed_ext_syn_length = 0;
        int composed_inh_syn_length = 0;
        
        // excitation
        if(!excitation.empty()){
            exc_syn_tmp = matrix(population_size.as_int(), excitation.size());
            excitation.apply([this](float c) {return c >= (float) this->threshold ? (float) this->threshold : c;});
            tile(exc_syn_tmp, excitation, population_size.as_int());
            if(!excitation_topology.empty()){
                if(excitation_topology.size_y() == population_size.as_int()
                && excitation_topology.size_x() == excitation.size())
                    exc_syn_tmp.multiply(excitation_topology);
                else
                    std::cout << "Warning: SpikingPopulation::Tick(): excitation_topology wrong size, not used.\n";
            }
            composed_ext_syn_length += excitation.size();
        } 

        // inhibition

        // direct in

        // std::cout << "ok 1\n" ;
        if(!direct_input.empty()){
            std::cout << "populationsize: " << population_size.as_int()
                << "; direct input: " << direct_input.size()
                << "\n";
            dir_in_tmp.copy(direct_input);
            
        }
        // std::cout << "ok 2\n" ;
        {
            // copy_array(dir_in_tmp, direct_input, population_size);
            dir_in_tmp.scale(cDirectCurrentScale);
        }
        std::cout << "ok 3\n" ;
        internal_topology_inp.info();
        std::cout << "--\n";
        internal_topology.info();
        // internal topology and synapse composition
        if(!internal_topology_inp.empty())
            internal_topology.copy(internal_topology_inp);
        // always do internal topology even if its filled with 0
        // because use it to compose synapses for excitation and inh
        {
            std::cout << "ok 3.1\n" ;
            // excitation
            int_exc_syn_tmp = matrix(population_size.as_int(), population_size.as_int());
            int_exc_syn_tmp.copy(internal_topology);
            int_exc_syn_tmp.apply([](float c) {return c > 0.f ? c : 0.f;});
            std::cout << "ok 4\n" ;
            matrix output_spikes = matrix(population_size.as_int());
            output_spikes.copy(output);
            output_spikes.apply([this](float c){return c >= (float)this->threshold ? (float)this->threshold : 0.f;});
            std::cout << "ok 5\n" ;
            matrix output_tiled = matrix(population_size.as_int(), population_size.as_int());
            tile(output_tiled, output_spikes, population_size.as_int());
            int_exc_syn_tmp.multiply(output_tiled);
            composed_ext_syn_length += population_size.as_int();
            std::cout << "ok 6\n" ;
            // inhibition
            int_inh_syn_tmp = matrix(population_size.as_int(), population_size.as_int()); 
            int_inh_syn_tmp.copy(internal_topology);
            int_inh_syn_tmp.apply([](float c) {return c < 0.f ? c : 0.f;});
            int_inh_syn_tmp.multiply(output_tiled);
            composed_inh_syn_length += population_size.as_int();

        }
        std::cout << "ok 6\n" ;
        composed_ext_t = int_exc_syn_tmp;
        if(!exc_syn_tmp.empty()){
            composed_ext_t = matrix(population_size.as_int(), composed_ext_syn_length);
            hstack(composed_ext_t, exc_syn_tmp, int_exc_syn_tmp);
        }
        std::cout << "ok 7\n" ;
        composed_inh_t = int_inh_syn_tmp;
        if(!inh_syn_tmp.empty()){
            composed_inh_t = matrix(population_size.as_int(), composed_inh_syn_length);
            hstack(composed_inh_t, inh_syn_tmp, int_inh_syn_tmp);
        }
        
        switch(model_type.as_int())
        {
            default:
            case eIzhikevich:
            {   
                TimeStep_Iz(    taurecovery,
                                coupling,
                                resetvolt,
                                resetrecovery,
                                composed_ext_t, // excitation
                                composed_inh_t,
                                dir_in_tmp,
                                vlt, // in output
                                u
                                
                                );
            }
        }
        // vlt.info();
        // std::cout << "---\n";
        // output.info();
        // std::cout << "--- (before)\n";
        output.copy(vlt);
        // output.info();
        // std::cout << "--- (after)\n";

        if((bool)(debugmode))
        {
            // print out debug info
            printf("\n\n---Module %s\n", "TODO get name");
            if(!excitation.empty()){
                //print_array("exc_in: ", excitation_array, excitation_size);
                //print_matrix("exc_syn: ", exc_syn_tmp, excitation_size, population_size);
                excitation.info();
            }
            if(!excitation_topology.empty())
                excitation_topology.info();
                
            if(!inhibition.empty()){
                // print_array("inh_in: ", inhibition_array, inhibition_size);
                // print_matrix("inh_syn", inh_syn_tmp, inhibition_size, population_size);
                inhibition.info();
            }
            if(!inhibition_topology.empty())
                inhibition_topology.info();
            
            if(!direct_input.empty())
                direct_input.info();
            //print_matrix("internal synapse", int_synapse, population_size, population_size);
            if(!internal_topology.empty()){
                // print_matrix("internal topology: ", internal_topology_inp, population_size, population_size);
                // print_matrix("int_inh_top_syn: ", int_inh_syn_tmp, population_size, population_size);
                internal_topology.info();
            }
            
        }
        std::cout << "SpikingPopulation::Tick(): complete\n";
    }

    void TimeStep_Iz(           matrix a_a, // tau recovery
                                matrix a_b, // coupling
                                matrix a_c, // reset voltage
                                matrix a_d, // reset recovery
                                matrix e_syn, // synapse 2d - contains excitation scaled by synaptic vals, or zero if not spiked
                                matrix i_syn, // synapse 2d - contains excitation scaled by synaptic vals, or zero if not spiked
                                matrix a_i, // direct current
                                matrix &a_v, // in out excitation
                                matrix &a_u  // recovery
                                )
    {
        std::cout << "step 1)\n";
        matrix fired(population_size.as_int());
        fired.copy(a_v);
        fired.apply([this](float c){ return c >= (float) this->threshold ? 1.f : 0.f; });
        
        matrix v1 = matrix(population_size.as_int());
        v1.copy(a_v);
        matrix u1 = matrix(population_size.as_int());
        u1.copy(a_u);
        matrix i1 = matrix(population_size.as_int());
        matrix inputvlt = matrix(population_size.as_int());

        std::cout << "step 2)\n";

        for(int j = 0; j < population_size.as_int(); j++)
        {
            if(fired[j] == 1.f)
            {
                a_v[j] = (float)threshold;
                v1[j] = a_c[j]; // reset voltage
                u1[j] = a_u[j] + a_d[j];
            }
            std::cout << "step 2.1)\n";
            // sum up 

            for(int i = 0; !e_syn.empty() && i < e_syn.size_x(); i++)
            {
                //if(e_syn[j][i] >= threshold) // TAT 2022-09-26: removed since spikes already gated above; result is scaled by synaptic weights
                inputvlt[j] += e_syn[j][i];
            }
            std::cout << "step 2.2)\n";
            for(int i = 0; !i_syn.empty() && i< i_syn.size_x(); i++)
            {
                //if(i_syn[j][i] >= threshold)
                inputvlt[j]-= i_syn[j][i];
            }
        }
        std::cout << "step 3)\n";

        i1.add(inputvlt, a_i);
        float stepfact = 1.f/(float)substeps;

        for(int i = 0; i < population_size.as_int(); i++)
        {
            for(int step = 0; step < substeps.as_int(); step++)
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
        std::cout << "step 4)\n";
        
        v1.apply([this](float c) { return c >= (float) this->threshold? (float)this->threshold : c;});
        a_v.copy(v1);
        a_u.copy(u1);
        // a_v.info();
        std::cout << "step complete\n";

    }


    void tile(matrix &r, matrix &a, int n){
        // TODO check sizes ok
        if (r.size_y() != n)
            std::cout << "SpikingPopulation::tile(): n != r.size_y\n";
        if (r.size_x() != a.size())
            std::cout << "SpikingPopulation::tile(): a.size != r.size_x\n";

        for(int j=0; j<n; j++)
            for(int i=0; i<r.size_x(); i++)
                r[j][i] = a[i];
    }
    void hstack(matrix &target,
            matrix &a,
            matrix &b
            ) {
    
        for (int j = 0; j < a.size_y(); ++j) {
            for (int i = 0; i < a.size_x(); ++i) {
                target[j][i] = a[j][i];
            }
            for (int i = 0; i < b.size_x(); ++i) {
                target[j][i + a.size_x()] = b[j][i];
            }
        }
    }
};

INSTALL_CLASS(SpikingPopulation)

