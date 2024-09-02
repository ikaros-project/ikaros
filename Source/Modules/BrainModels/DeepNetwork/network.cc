#include <random>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <limits>
#include "network.h"


NeuralNetwork::NeuralNetwork(const std::string &json_spec) {
    layers = parse_json_spec(json_spec);
}

void NeuralNetwork::forward( matrix &input) {
    matrix current_input = input;
    // current_input.reshape(1, input.size());
    for (auto &layer : layers) {
        layer->forward(current_input);
        current_input = layer->output;
    }
}

void NeuralNetwork::backward( matrix &t_input, matrix &t_target) {
    // Compute loss and start backward propagation
    matrix d_output = get_output().subtract(t_target);
    d_output.reshape(1, d_output.size());
    for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
        (*it)->backward(d_output);
        d_output = (*it)->d_input; // Propagate the gradients backward; check correct
    }
}

void NeuralNetwork::update(float learning_rate) {
    for (auto &layer : layers) {
        layer->update(learning_rate);
    }
}

matrix NeuralNetwork::get_output() const {
    return layers.back()->output;
}

float NeuralNetwork::compute_loss( matrix &t_target) const {
    //std::cout << "NeuralNetwork::compute_loss 0\n";
    matrix &predictions = layers.back()->output;
    //predictions.info("===predictions===");
    //t_target.info("===t_target===");
    //std::cout << "NeuralNetwork::compute_loss 0.1\n";
    //if (predictions.rows() != t_target.rows() || predictions.cols() != t_target.cols()) {
    if(predictions.size() != t_target.size())
        throw std::invalid_argument("NeuralNetwork::compute_loss: Predictions and target sizes do not match.");
    
    
    float loss = 0.0f;
    for (int i = 0; i < predictions.size(); ++i) {
        //for (int j = 0; j < predictions.cols(); ++j) {
            float prediction = predictions.data()[i];
            float target = t_target.data()[i];

            // Apply a small value to avoid log(0)
            float epsilon = 1e-8;
            loss -= target * std::log(prediction + epsilon);
        //}
    }
    //std::cout << "NeuralNetwork::compute_loss 2\n";
    return loss / predictions.size();
}


#include <stdexcept>

// Save weights to a file
void NeuralNetwork::save_weights(const std::string &filename)  {
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs.is_open()) {
        throw std::runtime_error("Could not open file to save weights");
    }

    for ( Layer* layer : layers) {
        layer->save(ofs);
    }

    ofs.close();
}

// Load weights from a file
void NeuralNetwork::load_weights(const std::string &filename) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs.is_open()) {
        throw std::runtime_error("Could not open file to load weights");
    }

    for (Layer* layer : layers) {
        layer->load(ifs);
    }

    ifs.close();
}

// Fully connected

// Constructor for FullyConnectedLayer
FullyConnectedLayer::FullyConnectedLayer(int input_size, int output_size) {
    // Initialize weights and biases with small random values
    //std::cout << "FullyConnectedLayer::FullyConnectedLayer begin\n";
    weights = matrix (input_size, output_size);
    d_weights = matrix (input_size, output_size);
    biases  = matrix (1, output_size);
    d_biases = matrix (1, output_size);
    output = matrix(1, output_size);
    d_input = matrix(1, input_size);
    for (int i = 0; i < weights.size(); ++i) {
        weights.data()[i] = static_cast<float>(rand()) / RAND_MAX * 0.01f;
    }
    
    // set names on the matrices
    weights.set_name("FullyConnectedLayer:weights");
    biases.set_name("FullyConnectedLayer:biases");
    d_weights.set_name("FullyConnectedLayer:d_weights");
    d_biases.set_name("FullyConnectedLayer:d_biases");
    d_input.set_name("FullyConnectedLayer:d_input");
    input_cache.set_name("FullyConnectedLayer:input_cache");
    output.set_name("FullyConnectedLayer:output");
    
}

// Forward method for FullyConnectedLayer
void FullyConnectedLayer::forward( matrix &input) {
    // Compute the dot product of input and weights
    //output.matmul(input, weights);
    input.reshape(1, input.size());
    input_cache = input; // Cache the input for use in backward pass

    output = matrix(1, weights.cols());
    output.matmul(input, weights);
    //output.info();

    // Add the bias
    for (int i = 0; i < output.rows(); ++i) {
        for (int j = 0; j < output.cols(); ++j) {
            output(i, j) += biases(0, j);
        }
    }

    // Apply an activation function (e.g., ReLU)
    for (int i = 0; i < output.size(); ++i) {
        output.data()[i] = std::max(0.0f, output.data()[i]); // ReLU activation
    }
    output.reshape(output.size()); // to array
}

// Backward method for FullyConnectedLayer
void FullyConnectedLayer::backward( matrix &d_output) {
    // Compute d_biases
    d_biases.reset();
    //d_output.info("===d_output===");
    for (int i = 0; i < d_output.rows(); ++i) {
        for (int j = 0; j < d_output.cols(); ++j) {
            d_biases(0, j) += d_output(i, j);
        }
    }

    // Compute d_weights
    d_weights.reset();
    for (int i = 0; i < input_cache.rows(); ++i) {
        for (int j = 0; j < d_output.cols(); ++j) {
            for (int k = 0; k < input_cache.cols(); ++k) {
                d_weights(k, j) += input_cache(i, k) * d_output(i, j);
            }
        }
    }
    matrix transp;
    weights.transpose(transp);
    
    // Compute d_input
    d_input = matrix(1, input_cache.size());
    d_input.matmul(d_output, transp);

    // Apply the derivative of the activation function (ReLU in this case)
    for (int i = 0; i < d_input.size(); ++i) {
        if (input_cache(0, i) <= 0) {
            d_input(0, i) = 0.0f;
        }
    }
    //d_input.info("===fullyconnected.d_input");
}

// Update method for FullyConnectedLayer
void FullyConnectedLayer::update(float learning_rate) {
    // Update weights and biases using the gradients
    for (int i = 0; i < weights.size(); ++i) {
        weights.data()[i] -= learning_rate * d_weights.data()[i];
    }
    for (int i = 0; i < biases.size(); ++i) {
        biases.data()[i] -= learning_rate * d_biases.data()[i];
    }
}

//
// convolutional layer
//
// Constructor for ConvolutionalLayer
ConvolutionalLayer::ConvolutionalLayer(int num_filters, int kernel_size){
    biases = matrix(num_filters);
    d_biases = matrix(num_filters); 

    // Initialize filters and biases with small random values
    filters.resize(num_filters); 
    for (auto &filter : filters) {
        filter = matrix(kernel_size, kernel_size);
        
        for (int i = 0; i < filter.size(); ++i) {
            filter.data()[i] = static_cast<float>(rand()) / RAND_MAX * 0.01f;
        }
    }

    d_filters.resize(num_filters);
    for (auto &filter : d_filters) 
        filter = matrix(kernel_size, kernel_size);

    // name matrices
    input_cache.set_name("ConvolutionalLayer:input_cache");
    output.set_name("ConvolutionalLayer:output");
    for(auto &filter : filters) filter.set_name("ConvolutionalLayer:filter");
    biases.set_name("ConvolutionalLayer:biases");
    d_input.set_name("ConvolutionalLayer:d_input");
    for(auto &dfilter : d_filters) dfilter.set_name("ConvolutionalLayer:d_filter");
    d_biases.set_name("ConvolutionalLayer:d_biases");
    output_shape.resize(3);
}

void ConvolutionalLayer::forward( matrix &input) {
    input_cache = input; // Cache the input for use in backward pass
    int num_filters = filters.size();
    int kernel_size = filters[0].rows();
    int output_size = input.rows() - kernel_size + 1;
    output = matrix(output_size, output_size, num_filters);
    output_shape[0] = output_size;
    output_shape[1] = output_size;
    output_shape[2] = num_filters;
    output.set_name("ConvolutionalLayer:output");
    // TODO add effort
    for (int f = 0; f < num_filters; ++f) {
        for (int i = 0; i < output_size; ++i) {
            for (int j = 0; j < output_size; ++j) {
                float sum = 0.0f;
                for (int ki = 0; ki < kernel_size; ++ki) {
                    for (int kj = 0; kj < kernel_size; ++kj) {     
                        sum += input(i + ki, j + kj) * filters[f](ki, kj);
                    }
                }
                output(i, j, f) = sum + biases[f];
            }
        }
    }

    // Apply an activation function (e.g., ReLU)
    for (int i = 0; i < output.size(); ++i) {
        output.data()[i] = std::max(0.0f, output.data()[i]); // ReLU activation
    }
}

void ConvolutionalLayer::backward( matrix &d_output) {
    //d_output.info("d_output");
    int num_filters = filters.size();
    int kernel_size = filters[0].rows();
    int output_size = output_shape[0];
    d_output.reshape(output_shape);

    d_input = matrix(input_cache.rows(), input_cache.cols());
    d_input.reset();

    // Compute d_biases
    // std::fill(d_biases.begin(), d_biases.end(), 0.0f);
    d_biases.reset();
    for (int f = 0; f < num_filters; ++f) {
        for (int i = 0; i < output_size; ++i) {
            for (int j = 0; j < output_size; ++j) {
                d_biases[f] += d_output(i, j, f);
            }
        }
    }

    // Compute d_filters
    for (auto &d_filter : d_filters) {
        d_filter.reset();
    }

    for (int f = 0; f < num_filters; ++f) {
        for (int i = 0; i < output_size; ++i) {
            for (int j = 0; j < output_size; ++j) {
                for (int ki = 0; ki < kernel_size; ++ki) {
                    for (int kj = 0; kj < kernel_size; ++kj) {
                        d_filters[f](ki, kj) += input_cache(i + ki, j + kj) * d_output(i, j, f);
                        d_input(i + ki, j + kj) += filters[f](ki, kj) * d_output(i, j, f);
                    }
                }
            }
        }
    }

    // Apply the derivative of the activation function (ReLU in this case)
    for (int i = 0; i < d_input.size(); ++i) {
        if (input_cache.data()[i] <= 0) {
            d_input.data()[i] = 0.0f;
        }
    }
}

void ConvolutionalLayer::update(float learning_rate) {
    // Update filters and biases using the gradients
    for (int f = 0; f < filters.size(); ++f) {
        for (int i = 0; i < filters[f].size(); ++i) {
            filters[f].data()[i] -= learning_rate * d_filters[f].data()[i];
        }
        biases[f] -= learning_rate * d_biases[f];
    }
}

//
// pooling layer
//
// Constructor for PoolingLayer
PoolingLayer::PoolingLayer(int pool_size, PoolingType type)
    : pool_size(pool_size), type(type) {}

// Forward method for PoolingLayer
void PoolingLayer::forward( matrix &input) {
    input_cache = input; // Cache the input for use in backward pass

    int depth = input.shape()[2]; 
    int input_height = input.shape()[0];
    int input_width = input.shape()[1];
    int output_height = input_height / pool_size;
    int output_width = input_width / pool_size;

    output = matrix(output_height, output_width, depth);

    for (int d = 0; d < depth; ++d) {
        for (int i = 0; i < output_height; ++i) {
            for (int j = 0; j < output_width; ++j) {
                float pooled_value = (type == MAX) ? -std::numeric_limits<float>::infinity() : 0.0f;
                for (int ki = 0; ki < pool_size; ++ki) {
                    for (int kj = 0; kj < pool_size; ++kj) {
                        int input_i = i * pool_size + ki;
                        int input_j = j * pool_size + kj;
                        if (type == MAX) {
                            pooled_value = std::max(pooled_value, input(input_i, input_j, d));
                        } else if (type == AVERAGE) {
                            pooled_value += input(input_i, input_j, d);
                        }
                    }
                }
                if (type == AVERAGE) {
                    pooled_value /= (pool_size * pool_size);
                }
                output(i, j, d) = pooled_value;
            }
        }
    }
}

// Backward method for PoolingLayer
void PoolingLayer::backward( matrix &d_output) {
    int depth = input_cache.shape()[2];
    int input_height = input_cache.rows();
    int input_width = input_cache.cols();
    int output_height = d_output.shape()[0];
    int output_width = d_output.shape()[1];

    d_input = matrix(input_height, input_width, depth);
    d_input.reset();

    for (int d = 0; d < depth; ++d) {
        for (int i = 0; i < output_height; ++i) {
            for (int j = 0; j < output_width; ++j) {
                for (int ki = 0; ki < pool_size; ++ki) {
                    for (int kj = 0; kj < pool_size; ++kj) {
                        int input_i = i * pool_size + ki;
                        int input_j = j * pool_size + kj;
                        if (type == MAX && input_cache(input_i, input_j, d) == output(i, j, d)) {
                            d_input(input_i, input_j, d) += d_output(i, j, d);
                        } else if (type == AVERAGE) {
                            d_input(input_i, input_j, d) += d_output(i, j, d) / (pool_size * pool_size);
                        }
                    }
                }
            }
        }
    }
}

//
// Activation layer
//



// Constructor for ActivationLayer
ActivationLayer::ActivationLayer(ActivationType type)
    : type(type) {}

// Forward method for ActivationLayer
void ActivationLayer::forward( matrix &input) {
    input_cache = input; // Cache the input for use in backward pass

    output = input; // Initialize output matrix with the same size as input
    for (int i = 0; i < input.size(); ++i) {
        if (type == RELU) {
            output.data()[i] = std::max(0.0f, input.data()[i]); // ReLU activation
        } else if (type == SIGMOID) {
            output.data()[i] = 1.0 / (1.0 + std::exp(-input.data()[i])); // Sigmoid activation
        }
    }
}

// Backward method for ActivationLayer
void ActivationLayer::backward( matrix &d_output) {
    d_input = d_output; // Initialize d_input matrix with the same size as d_output

    for (int i = 0; i < input_cache.size(); ++i) {
        if (type == RELU) {
            d_input.data()[i] = (input_cache.data()[i] > 0) ? d_output.data()[i] : 0.0f; // ReLU derivative
        } else if (type == SIGMOID) {
            float sigmoid = 1.0 / (1.0 + std::exp(-input_cache.data()[i]));
            d_input.data()[i] = d_output.data()[i] * sigmoid * (1 - sigmoid); // Sigmoid derivative
        }
    }
}

//
// Batch normalization
//
// Constructor for BatchNormalizationLayer
BatchNormalizationLayer::BatchNormalizationLayer(int size)
    : size(size), gamma(1, size), beta(1, size),
      d_gamma(1, size), d_beta(1, size) {}

// Forward method for BatchNormalizationLayer
void BatchNormalizationLayer::forward( matrix &input) {
    input_cache = input; // Cache the input for use in backward pass

    int N = input.rows();
    mean_cache = matrix(1, size);
    var_cache = matrix(1, size);

    // Compute mean
    for (int j = 0; j < size; ++j) {
        float sum = 0.0f;
        for (int i = 0; i < N; ++i) {
            sum += input(i, j);
        }
        mean_cache(0, j) = sum / N;
    }

    // Compute variance
    for (int j = 0; j < size; ++j) {
        float sum = 0.0f;
        for (int i = 0; i < N; ++i) {
            sum += std::pow(input(i, j) - mean_cache(0, j), 2);
        }
        var_cache(0, j) = sum / N;
    }

    // Normalize
    output = matrix(input.rows(), input.cols());
    for (int i = 0; i < input.rows(); ++i) {
        for (int j = 0; j < input.cols(); ++j) {
            output(i, j) = (input(i, j) - mean_cache(0, j)) / std::sqrt(var_cache(0, j) + 1e-8);
            output(i, j) = gamma(0, j) * output(i, j) + beta(0, j);
        }
    }
}

// Backward method for BatchNormalizationLayer
void BatchNormalizationLayer::backward( matrix &d_output) {
    int N = input_cache.rows();

    d_gamma = matrix(1, size);
    d_beta = matrix(1, size);

    for (int j = 0; j < size; ++j) {
        float d_gamma_sum = 0.0f;
        float d_beta_sum = 0.0f;
        for (int i = 0; i < N; ++i) {
            d_gamma_sum += d_output(i, j) * output(i, j);
            d_beta_sum += d_output(i, j);
        }
        d_gamma(0, j) = d_gamma_sum;
        d_beta(0, j) = d_beta_sum;
    }

    d_input = matrix(N, size);
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < size; ++j) {
            float norm_x = (input_cache(i, j) - mean_cache(0, j)) / std::sqrt(var_cache(0, j) + 1e-8);
            d_input(i, j) = (1.0 / N) * gamma(0, j) / std::sqrt(var_cache(0, j) + 1e-8) *
                (N * d_output(i, j) - d_beta(0, j) - norm_x * d_gamma(0, j));
        }
    }
}

// Update method for BatchNormalizationLayer
void BatchNormalizationLayer::update(float learning_rate) {
    for (int i = 0; i < gamma.size(); ++i) {
        gamma.data()[i] -= learning_rate * d_gamma.data()[i];
        beta.data()[i] -= learning_rate * d_beta.data()[i];
    }
}

//
// dropout
//
// Constructor for DropoutLayer
DropoutLayer::DropoutLayer(float dropout_rate)
    : dropout_rate(dropout_rate) {}

// Forward method for DropoutLayer
void DropoutLayer::forward( matrix &input) {
    input_cache = input; // Cache the input for use in backward pass

    std::default_random_engine generator;
    std::bernoulli_distribution distribution(1.0 - dropout_rate);

    dropout_mask = matrix(input.rows(), input.cols());
    for (int i = 0; i < dropout_mask.size(); ++i) {
        dropout_mask.data()[i] = distribution(generator);
    }

    output = input;
    for (int i = 0; i < output.size(); ++i) {
        output.data()[i] *= dropout_mask.data()[i];
    }
}

// Backward method for DropoutLayer
void DropoutLayer::backward( matrix &d_output) {
    d_input = d_output;
    for (int i = 0; i < d_input.size(); ++i) {
        d_input.data()[i] *= dropout_mask.data()[i];
    }
}

// FullyConnectedLayer methods
void FullyConnectedLayer::save(std::ofstream &ofs)  {
    for (int i = 0; i < weights.size(); ++i) {
        ofs.write(reinterpret_cast<const char*>(&weights.data()[i]), sizeof(float));
    }
    for (int i = 0; i < biases.size(); ++i) {
        ofs.write(reinterpret_cast<const char*>(&biases.data()[i]), sizeof(float));
    }
}

void FullyConnectedLayer::load(std::ifstream &ifs) {
    for (int i = 0; i < weights.size(); ++i) {
        ifs.read(reinterpret_cast<char*>(&weights.data()[i]), sizeof(float));
    }
    for (int i = 0; i < biases.size(); ++i) {
        ifs.read(reinterpret_cast<char*>(&biases.data()[i]), sizeof(float));
    }
}

// ConvolutionalLayer methods
void ConvolutionalLayer::save(std::ofstream &ofs)  {
    for ( matrix& filter : filters) {
        for (int i = 0; i < filter.size(); ++i) {
            ofs.write(reinterpret_cast<const char*>(&filter.data()[i]), sizeof(float));
        }
    }
    for (float bias : biases) {
        ofs.write(reinterpret_cast<const char*>(&bias), sizeof(float));
    }
}

void ConvolutionalLayer::load(std::ifstream &ifs) {
    for (matrix& filter : filters) {
        for (int i = 0; i < filter.size(); ++i) {
            ifs.read(reinterpret_cast<char*>(&filter.data()[i]), sizeof(float));
        }
    }
    for (float& bias : biases) {
        ifs.read(reinterpret_cast<char*>(&bias), sizeof(float));
    }
}

// BatchNormalizationLayer methods
void BatchNormalizationLayer::save(std::ofstream &ofs)  {
    for (int i = 0; i < gamma.size(); ++i) {
        ofs.write(reinterpret_cast<const char*>(&gamma.data()[i]), sizeof(float));
    }
    for (int i = 0; i < beta.size(); ++i) {
        ofs.write(reinterpret_cast<const char*>(&beta.data()[i]), sizeof(float));
    }
}

void BatchNormalizationLayer::load(std::ifstream &ifs) {
    for (int i = 0; i < gamma.size(); ++i) {
        ifs.read(reinterpret_cast<char*>(&gamma.data()[i]), sizeof(float));
    }
    for (int i = 0; i < beta.size(); ++i) {
        ifs.read(reinterpret_cast<char*>(&beta.data()[i]), sizeof(float));
    }
}

// Utility functions to parse JSON and initialize layers
std::vector<Layer*> parse_json_spec(const std::string &json_spec) {
    std::vector<Layer*> layers;
    auto spec = json::parse(json_spec);

    for (const auto &layer_spec : spec["layers"]) {
        std::string type = layer_spec["type"];
        if (type == "fully_connected") {
            int input_size = layer_spec["input_size"];
            int output_size = layer_spec["output_size"];
            layers.push_back(new FullyConnectedLayer(input_size, output_size));
        } else if (type == "convolutional") {
            int num_filters = layer_spec["num_filters"];
            int kernel_size = layer_spec["kernel_size"];
            layers.push_back(new ConvolutionalLayer(num_filters, kernel_size));
        } else if (type == "pooling") {
            int pool_size =layer_spec["pool_size"];
            PoolingLayer::PoolingType pooling_type = (layer_spec["pooling_type"] == "max") ? PoolingLayer::MAX : PoolingLayer::AVERAGE;
            layers.push_back(new PoolingLayer(pool_size, pooling_type));
        } else if (type == "activation") {
            ActivationLayer::ActivationType activation_type = (layer_spec["activation_type"] == "relu") ? ActivationLayer::RELU : ActivationLayer::SIGMOID;
            layers.push_back(new ActivationLayer(activation_type));
        } else if (type == "batch_normalization") {
            int size = layer_spec["size"];
            layers.push_back(new BatchNormalizationLayer(size));
        } else if (type == "dropout") {
            float dropout_rate = layer_spec["dropout_rate"];
            layers.push_back(new DropoutLayer(dropout_rate));
        }
    }
    return layers;
}