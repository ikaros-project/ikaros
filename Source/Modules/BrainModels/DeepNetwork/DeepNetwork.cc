#include "ikaros.h"
#include "network.h"
//#include "Kernel/json.cc"
#include <sstream>
#include <filesystem>
using namespace ikaros;
//using json = nlohmann::json;
class DeepNetwork: public Module
{
    parameter output_size;
    parameter spec_filename;
    parameter weights_filename;
    parameter do_load_weights;
    parameter do_save_weights;
    parameter learning_rate;
    matrix input;
    matrix t_input;
    matrix t_target;
    matrix loss;
    matrix output;
    matrix effort;

    NeuralNetwork *network;
public:
    DeepNetwork()
    {
        
        //int size  = stoi(GetValue("output_size"));
        
        //info_["outputs"] = list(); // Temporary fix; check that it does not already exist in AddOutput later
        //auto spec = json::parse(spec_filename);
        //AddOutput("OUTPUT", size, "Inference output of the deep network");
        

    }

    void Init()
    {
        // Bind(output_size, "output_size");
        Bind(spec_filename, "spec_filename");
        Bind(weights_filename, "weights_filename");
        Bind(do_load_weights, "load_weights");
        Bind(do_save_weights, "save_weights");
        Bind(learning_rate, "learning_rate");
        Bind(input, "INPUT");
        Bind(t_input, "T_INPUT");
        Bind(t_target, "T_TARGET");
        Bind(effort, "EFFORT");
        Bind(loss, "LOSS");
        Bind(output, "OUTPUT");

        // TODO make input to network dynamic 
        std::string spec = readFileIntoString(spec_filename);
        network = new NeuralNetwork(spec);
        if(do_load_weights) network->load_weights(weights_filename);
    }


    void Tick()
    {
        if(effort.empty() || effort > 0){
            // convert to rank 2
            //matrix inp, t_inp, t_trgt;
            //inp.copy(input); // does not copy shape
            //t_inp.copy(t_input); 
            //t_trgt.copy(t_target);
            //inp.reshape(1, input.size());
            //t_inp.reshape(1, t_input.size()); 
            //t_trgt.reshape(1, t_target.size());


            network->forward(input);
            matrix out = network->get_output(); 
            output.copy(out);
            if(!t_input.empty() && !t_target.empty()){
                network->backward(t_input, t_target);
                network->update(learning_rate);
                loss(0) = network->compute_loss(t_target);
            }
        }
    }
    std::string readFileIntoString(const std::string& fileName) {
        std::ifstream fileStream(fileName);
        if (!fileStream.is_open()) {
            std::filesystem::path cwd = std::filesystem::current_path();
            std::cout << "Current working directory: " << cwd << std::endl;

            throw std::runtime_error("Could not open file " + fileName);
        }

        std::stringstream buffer;
        buffer << fileStream.rdbuf();
        return buffer.str();
    }
};

INSTALL_CLASS(DeepNetwork)