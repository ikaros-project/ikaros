#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "ikaros.h"

using namespace ikaros;


class NeuralTissue: public Module
{
    static constexpr int inputCount = 26;
    static constexpr int populationCount = 7;
    static constexpr int shuntingInput = 25;
    const std::array<std::string, inputCount> inputNames = {
        "AMPA", "NMDA", "MGLU_I", "MGLU_II", "MGLU_III",
        "GABA_A", "GABA_B", "GLYR", "NACHR", "MACHR_M135", "MACHR_M24",
        "D1_LIKE", "D2_LIKE", "ADRENERGIC_ALPHA1", "ADRENERGIC_ALPHA2", "ADRENERGIC_BETA",
        "HT1", "HT2", "HT3", "HT4", "HT5", "HT6", "HT7",
        "EXCITATION", "INHIBITION", "SHUNTING_INHIBITION",
    };
    const std::array<std::string, populationCount> outputNames = {
        "GLUTAMATE", "GABA", "GLYCINE", "ACETYLCHOLINE", "DOPAMINE", "NORADRENALINE", "SEROTONIN",
    };
    const std::array<std::string, populationCount> prefixes = {
        "glutamate", "gaba", "glycine", "acetylcholine", "dopamine", "noradrenaline", "serotonin",
    };

    struct Population
    {
        parameter enabled;
        parameter tau;
        parameter baseline;
        parameter psi;
        matrix gains;
        matrix weights;
        matrix output;
        double alpha = 0;
        double baselineValue = 0;
        double psiValue = 0;
        bool active = false;
        bool identity = true;
    };

    std::array<matrix, inputCount> inputs;
    std::array<int, inputCount> offsets{};
    std::array<int, inputCount> sizes{};
    std::array<std::vector<int>, inputCount> inputShapes;
    std::array<Population, populationCount> populations;
    std::vector<float> inputValues;

    int
    SetOutputShape(dictionary definition, input_map connections) override
    {
        const std::string name = definition["name"];
        for(int p = 0; p < populationCount; ++p)
            if(name == outputNames[p])
            {
                parameter shape;
                Bind(shape, prefixes[p] + "_shape");
                if(!shape.is_resolved())
                    return 0;
                // Expand the configured expression before the kernel resolves input dependencies.
                definition["shape"] = shape.as_string();
                break;
            }
        return Module::SetOutputShape(definition, connections);
    }


    void
    Init() override
    {
        int totalSize = 0;
        for(int k = 0; k < inputCount; ++k)
        {
            Bind(inputs[k], inputNames[k]);
            offsets[k] = totalSize;
            sizes[k] = inputs[k].size();
            inputShapes[k] = inputs[k].shape();
            if(inputs[k].is_dynamic())
                throw std::invalid_argument("NeuralTissue requires fixed input shapes at startup.");
            if(sizes[k] > std::numeric_limits<int>::max() - totalSize)
                throw std::invalid_argument("NeuralTissue combined input size exceeds the supported range.");
            totalSize += sizes[k];
        }
        inputValues.resize(totalSize);

        const double dt = GetTickDuration();
        if(!std::isfinite(dt) || dt <= 0)
            throw std::invalid_argument("NeuralTissue requires a positive finite tick duration.");

        for(int p = 0; p < populationCount; ++p)
        {
            auto & population = populations[p];
            const auto & prefix = prefixes[p];
            Bind(population.enabled, prefix + "_enabled");
            Bind(population.tau, prefix + "_tau");
            Bind(population.baseline, prefix + "_baseline");
            Bind(population.psi, prefix + "_psi");
            Bind(population.gains, prefix + "_gains");
            Bind(population.weights, prefix + "_weights");
            Bind(population.output, outputNames[p]);
            population.output = 0;
            population.active = population.enabled.as_bool();
            population.identity = population.weights.empty();

            const double tau = population.tau.as_double();
            const double baseline = population.baseline.as_double();
            const double psi = population.psi.as_double();
            if(!std::isfinite(tau) || tau <= 0 || !std::isfinite(baseline) ||
               baseline < 0 || baseline > 1 || !std::isfinite(psi) || psi < 0)
                throw std::invalid_argument(prefix + ": require tau > 0, baseline in [0,1], and psi >= 0, all finite.");
            population.alpha = -std::expm1(-dt / tau);
            population.baselineValue = baseline;
            population.psiValue = psi;
            population.gains = population.gains.clone();
            population.weights = population.weights.clone();
            if(population.output.empty() || !population.output.is_contiguous())
                throw std::invalid_argument(prefix + ": output must have a nonempty contiguous startup shape.");
            if(population.gains.rank() != 1 || population.gains.size() != inputCount)
                throw std::invalid_argument(prefix + "_gains must contain exactly 26 values.");
            for(int k = 0; k < inputCount; ++k)
                if(!std::isfinite(population.gains(k)))
                    throw std::invalid_argument(prefix + "_gains must be finite.");
            if(population.gains(23) < 0 || population.gains(24) > 0 || population.gains(25) < 0)
                throw std::invalid_argument(prefix + ": generic excitation/shunting gains must be nonnegative; inhibition gain must be nonpositive.");

            if(!population.identity)
            {
                if(population.weights.rank() != 2 || population.weights.rows() != population.output.size() ||
                   population.weights.cols() != totalSize)
                    throw std::invalid_argument(prefix + "_weights must have output.size rows and sum of input sizes columns.");
                for(int i = 0; i < population.weights.rows(); ++i)
                    for(int j = 0; j < population.weights.cols(); ++j)
                        if(!std::isfinite(population.weights(i, j)) || population.weights(i, j) < 0)
                            throw std::invalid_argument(prefix + "_weights must be finite and nonnegative; use receptor gains for sign.");
            }
            else if(population.active)
                for(int k = 0; k < inputCount; ++k)
                    if(sizes[k] && population.gains(k) != 0 && inputs[k].shape() != population.output.shape())
                        throw std::invalid_argument(prefix + ": " + inputNames[k] + " shape differs from output; supply explicit internal weights.");
        }
    }


    void
    Tick() override
    {
        for(int k = 0; k < inputCount; ++k)
        {
            if(inputs[k].shape() != inputShapes[k])
            {
                Notify(msg_fatal_error, "NeuralTissue input shape changed after startup.");
                return;
            }
            int index = offsets[k];
            for(int block = 0; block < inputs[k].logical_block_count(); ++block)
            {
                const float * values = inputs[k].logical_block_data(block);
                for(int j = 0; j < inputs[k].logical_block_size(); ++j)
                {
                    const float value = values[j];
                    if(!std::isfinite(value) || value < 0)
                    {
                        Notify(msg_fatal_error, "NeuralTissue inputs must be finite and nonnegative.");
                        return;
                    }
                    inputValues[index++] = value;
                }
            }
        }

        for(auto & population : populations)
        {
            if(!population.active)
                continue;
            float * output = population.output.contiguous_data();
            for(int i = 0; i < population.output.size(); ++i)
            {
                double excitation = 0;
                double inhibition = 0;
                double shunting = 0;
                for(int k = 0; k < inputCount; ++k)
                {
                    const double gain = population.gains(k);
                    if(sizes[k] == 0 || gain == 0)
                        continue;
                    double drive = 0;
                    if(population.identity)
                        drive = inputValues[offsets[k] + i];
                    else
                        for(int j = 0; j < sizes[k]; ++j)
                            drive += double(population.weights(i, offsets[k] + j)) * inputValues[offsets[k] + j];
                    drive *= gain;
                    if(k == shuntingInput)
                        shunting += drive;
                    else if(drive >= 0)
                        excitation += drive;
                    else
                        inhibition -= drive;
                }
                const double target = std::clamp(population.baselineValue +
                    excitation / (1 + population.psiValue * shunting) - inhibition, 0.0, 1.0);
                output[i] += population.alpha * (target - output[i]);
            }
        }
    }
};

INSTALL_CLASS(NeuralTissue)
