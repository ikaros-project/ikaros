/**
 * Generated with ChatGPT 4o 2024-07-29
 */

#include <vector>
#include <string>
//#include "matrix.h"
//#include "maths.h"
#include "ikaros.h"
#include "json.hpp"

using json = nlohmann::json;
using namespace ikaros;
class Layer {
public:
    virtual void forward(matrix &input) = 0;
    virtual void backward( matrix &d_output) = 0;
    virtual void update(float learning_rate) = 0;
    virtual void save(std::ofstream &ofs) = 0;
    virtual void load(std::ifstream &ifs) = 0;
    matrix output; // forwards
    matrix d_input; // backwards
    std::string name;
};

class FullyConnectedLayer : public Layer {
    public:
    FullyConnectedLayer(int input_size, int output_size);
    // Implementation of Fully Connected Layer
    virtual void forward( matrix &input);
    virtual void backward( matrix &d_output);
    virtual void update(float learning_rate);
    virtual void save(std::ofstream &ofs) ;
    virtual void load(std::ifstream &ifs);

    matrix weights;
    matrix biases;
    matrix input_cache;
    // derivations
    //matrix d_input;
    matrix d_weights;
    matrix d_biases;
    matrix d_output;
};

class ConvolutionalLayer : public Layer {
    // Implementation of Convolutional Layer
    public:
    ConvolutionalLayer(int num_filters, int kernel_size);
    virtual void forward(matrix &input);
    virtual void backward( matrix &d_output);
    virtual void update(float learning_rate);
    virtual void save(std::ofstream &ofs) ;
    virtual void load(std::ifstream &ifs);

    matrix input_cache;
    std::vector<int> output_shape; 

    std::vector<matrix> filters;
    matrix biases;
    // matrix d_input;
    std::vector<matrix> d_filters;
    matrix d_biases;
    
};

class PoolingLayer : public Layer {
public:
    enum PoolingType { MAX, AVERAGE };

    PoolingLayer(int pool_size, PoolingType type);
    virtual void forward( matrix &input) ;
    virtual void backward( matrix &d_output) ;
    virtual void update(float learning_rate) {};
    virtual void save(std::ofstream &ofs) {};
    virtual void load(std::ifstream &ifs) {};

private:
    int pool_size;
    PoolingType type;
    matrix input_cache;

};

class ActivationLayer : public Layer {
public:
    enum ActivationType { RELU, SIGMOID };

    ActivationLayer(ActivationType type);
    virtual void forward (matrix &input) ;
    virtual void backward(matrix &d_output) ;
    virtual void update(float learning_rate)  {}
    virtual void save(std::ofstream &ofs) {};
    virtual void load(std::ifstream &ifs) {};

private:
    ActivationType type;
    matrix input_cache;

};

class BatchNormalizationLayer : public Layer {
public:
    BatchNormalizationLayer(int size);
    virtual void forward( matrix &input);
    virtual void backward( matrix &d_output);
    virtual void update(float learning_rate);
    virtual void save(std::ofstream &ofs) ;
    virtual void load(std::ifstream &ifs);

    matrix gamma, beta, d_gamma, d_beta;
    matrix input_cache, mean_cache, var_cache;
private:
    int size;
    

};

class DropoutLayer : public Layer {
public:
    DropoutLayer(float dropout_rate);
    virtual void forward( matrix &input);
    virtual void backward( matrix &d_output);
    virtual void update(float learning_rate)  {};
    virtual void save(std::ofstream &ofs) {};
    virtual void load(std::ifstream &ifs) {};

private:
    float dropout_rate;
    matrix dropout_mask;
    matrix input_cache;

};

class NeuralNetwork {
public:
    NeuralNetwork(const std::string &json_spec);
    void forward(matrix &input);
    void backward( matrix &t_input,  matrix &t_target);
    void update(float learning_rate);
    matrix get_output() const;
    float compute_loss( matrix &t_target) const;
    void save_weights(const std::string &filename) ;
    void load_weights(const std::string &filename);
private:
    std::vector<Layer*> layers;
    matrix output;
};

// Utility functions to parse JSON and initialize layers
std::vector<Layer*> parse_json_spec(const std::string &json_spec);