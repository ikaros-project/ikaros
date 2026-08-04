#include <algorithm>
#include <filesystem>
#include <memory>
#include <stdexcept>

#include "NativeOnnxInference.h"

using namespace ikaros;

namespace
{
    constexpr const char * alikedSha256 =
        "3f05fbc23007143c3000abf377b74ea92f3d072e74a8be9361304c3015e520ae";

    bool
    hasShape(const Ort::Value & value, const std::vector<int64_t> & expected)
    {
        return value.IsTensor() &&
               value.GetTensorTypeAndShapeInfo().GetShape() == expected;
    }
}

class ALIKEDFeatureExtractor : public Module
{
    matrix input_;
    matrix enable_;
    matrix keypoints_;
    matrix descriptors_;
    matrix scores_;
    parameter modelPath_;
    parameter useCoreML_;
    parameter scoreThreshold_;
    parameter maxFeatures_;
    std::unique_ptr<NativeOnnxInference> inference_;
    bool inferenceWarningIssued_ = false;

public:
    void Init() override
    {
        Bind(input_, "INPUT");
        Bind(enable_, "ENABLE");
        Bind(keypoints_, "KEYPOINTS");
        Bind(descriptors_, "DESCRIPTORS");
        Bind(scores_, "SCORES");
        Bind(modelPath_, "model_path");
        Bind(useCoreML_, "use_coreml");
        Bind(scoreThreshold_, "score_threshold");
        Bind(maxFeatures_, "max_features");

        if(input_.shape() != std::vector<int>({3, 240, 320}))
            throw std::runtime_error(
                "ALIKEDFeatureExtractor INPUT must have shape 3,240,320");
        if(keypoints_.capacity() !=
               std::vector<int>({static_cast<int>(maxFeatures_), 2}) ||
           descriptors_.capacity() !=
               std::vector<int>({static_cast<int>(maxFeatures_), 128}) ||
           scores_.capacity() !=
               std::vector<int>({static_cast<int>(maxFeatures_), 1}))
            throw std::runtime_error(
                "ALIKEDFeatureExtractor dynamic output capacities were not set up correctly");

        std::filesystem::path resolvedModelPath;
        if(!kernel().SanitizeReadPath(modelPath_.as_string(), resolvedModelPath))
            throw std::runtime_error(
                "ALIKEDFeatureExtractor could not resolve model_path inside the project "
                "directory or UserData");

        std::filesystem::path modelCacheDirectory;
        if(static_cast<bool>(useCoreML_))
        {
            if(!kernel().SanitizeWritePath(
                   "models/ElasticTemplateMatcher/CoreMLCache/ALIKED",
                   modelCacheDirectory))
                throw std::runtime_error(
                    "ALIKEDFeatureExtractor could not resolve its Core ML cache inside "
                    "UserData");
            std::filesystem::create_directories(modelCacheDirectory);
        }

        inference_ = std::make_unique<NativeOnnxInference>(
            resolvedModelPath,
            alikedSha256,
            std::vector<OnnxTensorContract>{{"image", ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, 4}},
            std::vector<OnnxTensorContract>{
                {"keypoints", ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, 3},
                {"descriptors", ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, 3},
                {"scores", ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, 2},
            },
            OnnxInferenceOptions{
                .useCoreML = static_cast<bool>(useCoreML_),
                .requireStaticInputShapes = true,
                .enableMemoryPattern = true,
                .modelCacheDirectory = modelCacheDirectory,
            });
    }


    void Tick() override
    {
        keypoints_.clear();
        descriptors_.clear();
        scores_.clear();
        if(enable_.size() > 0 && enable_(0) <= 0.0f)
            return;

        try
        {
            std::vector<Ort::Value> inputs;
            inputs.emplace_back(inference_->FloatTensor(input_));
            std::vector<Ort::Value> outputs = inference_->Run(inputs);
            if(outputs.size() != 3 ||
               !hasShape(outputs[0], {1, 512, 2}) ||
               !hasShape(outputs[1], {1, 512, 128}) ||
               !hasShape(outputs[2], {1, 512}))
                throw std::runtime_error("ALIKED model returned unexpected output shapes");

            const float * sourceKeypoints = outputs[0].GetTensorData<float>();
            const float * sourceDescriptors = outputs[1].GetTensorData<float>();
            const float * sourceScores = outputs[2].GetTensorData<float>();
            const int limit = std::min(512, static_cast<int>(maxFeatures_));
            const float threshold = static_cast<float>(scoreThreshold_);
            int featureCount = 0;
            for(int i = 0; i < limit; ++i)
                if(sourceScores[i] >= threshold)
                    ++featureCount;

            keypoints_.resize(featureCount, 2);
            descriptors_.resize(featureCount, 128);
            scores_.resize(featureCount, 1);
            int target = 0;
            for(int source = 0; source < limit; ++source)
            {
                if(sourceScores[source] < threshold)
                    continue;
                keypoints_(target, 0) = sourceKeypoints[2 * source];
                keypoints_(target, 1) = sourceKeypoints[2 * source + 1];
                std::copy_n(sourceDescriptors + 128 * source, 128,
                            descriptors_.data() + 128 * target);
                scores_(target, 0) = sourceScores[source];
                ++target;
            }
            inferenceWarningIssued_ = false;
        }
        catch(const std::exception & error)
        {
            if(!inferenceWarningIssued_)
            {
                Warning(std::string("ALIKEDFeatureExtractor inference failed: ") +
                        error.what());
                inferenceWarningIssued_ = true;
            }
        }
    }
};

INSTALL_CLASS(ALIKEDFeatureExtractor)
