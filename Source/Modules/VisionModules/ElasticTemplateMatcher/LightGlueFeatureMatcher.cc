#include <algorithm>
#include <chrono>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <vector>

#include "NativeOnnxInference.h"

using namespace ikaros;

namespace
{
    constexpr const char * lightGlueSha256 =
        "80300b69842761d07ccc9a15b2097724f0319897d0ba89fc921cf37050bd3e68";
}

class LightGlueFeatureMatcher : public Module
{
    matrix keypoints_;
    matrix descriptors_;
    matrix templateKeypoints_;
    matrix templateDescriptors_;
    matrix templateRanges_;
    matrix enable_;
    matrix correspondences_;
    matrix performance_;
    parameter modelPath_;
    parameter useCoreML_;
    parameter imageWidth_;
    parameter imageHeight_;
    parameter scoreThreshold_;
    parameter maxFeatures_;
    parameter maxTemplates_;
    parameter maxMatchesPerTemplate_;
    matrix selectedKeypoints_;
    matrix selectedDescriptors_;
    matrix imageSize_;
    std::unique_ptr<NativeOnnxInference> inference_;
    bool inferenceWarningIssued_ = false;
    float smoothedInferenceMilliseconds_ = 0.0f;
    bool hasInferenceTiming_ = false;

public:
    void Init() override
    {
        Bind(keypoints_, "KEYPOINTS");
        Bind(descriptors_, "DESCRIPTORS");
        Bind(templateKeypoints_, "TEMPLATE_KEYPOINTS");
        Bind(templateDescriptors_, "TEMPLATE_DESCRIPTORS");
        Bind(templateRanges_, "TEMPLATE_RANGES");
        Bind(enable_, "ENABLE");
        Bind(correspondences_, "CORRESPONDENCES");
        Bind(performance_, "PERFORMANCE");
        Bind(modelPath_, "model_path");
        Bind(useCoreML_, "use_coreml");
        Bind(imageWidth_, "image_width");
        Bind(imageHeight_, "image_height");
        Bind(scoreThreshold_, "score_threshold");
        Bind(maxFeatures_, "max_features");
        Bind(maxTemplates_, "max_templates");
        Bind(maxMatchesPerTemplate_, "max_matches_per_template");

        const int correspondenceCapacity = static_cast<int>(maxTemplates_) *
                                           static_cast<int>(maxMatchesPerTemplate_);
        if(correspondences_.capacity() !=
           std::vector<int>({correspondenceCapacity, 4}))
            throw std::runtime_error(
                "LightGlueFeatureMatcher dynamic output capacity was not set up correctly");

        selectedKeypoints_.realloc(static_cast<int>(maxFeatures_), 2);
        selectedDescriptors_.realloc(static_cast<int>(maxFeatures_), 128);
        imageSize_.realloc(2);
        imageSize_(0) = static_cast<float>(imageWidth_);
        imageSize_(1) = static_cast<float>(imageHeight_);

        std::filesystem::path resolvedModelPath;
        if(!kernel().SanitizeReadPath(modelPath_.as_string(), resolvedModelPath))
            throw std::runtime_error(
                "LightGlueFeatureMatcher could not resolve model_path inside the project "
                "directory or UserData");

        std::filesystem::path modelCacheDirectory;
        if(static_cast<bool>(useCoreML_))
        {
            if(!kernel().SanitizeWritePath(
                   "models/ElasticTemplateMatcher/CoreMLCache/LightGlue",
                   modelCacheDirectory))
                throw std::runtime_error(
                    "LightGlueFeatureMatcher could not resolve its Core ML cache inside "
                    "UserData");
            std::filesystem::create_directories(modelCacheDirectory);
        }
        inference_ = std::make_unique<NativeOnnxInference>(
            resolvedModelPath,
            lightGlueSha256,
            std::vector<OnnxTensorContract>{
                {"keypoints0", ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, 3},
                {"descriptors0", ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, 3},
                {"size0", ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, 2},
                {"keypoints1", ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, 3},
                {"descriptors1", ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, 3},
                {"size1", ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, 2},
            },
            std::vector<OnnxTensorContract>{
                {"matches0", ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64, 2},
                {"scores0", ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, 2},
            },
            OnnxInferenceOptions{
                .useCoreML = static_cast<bool>(useCoreML_),
                .requireStaticInputShapes = false,
                .enableMemoryPattern = false,
                .modelCacheDirectory = modelCacheDirectory,
            });
        performance_(2) = inference_->UsingCoreML() ? 1.0f : 0.0f;
    }


    void Tick() override
    {
        correspondences_.clear();
        if(enable_.size() > 0 && enable_(0) <= 0.0f)
            return;
        if(keypoints_.rows() == 0 || templateRanges_.rows() == 0)
            return;

        if(keypoints_.rank() != 2 || keypoints_.cols() != 2 ||
           descriptors_.rank() != 2 || descriptors_.cols() != 128 ||
           descriptors_.rows() != keypoints_.rows() ||
           keypoints_.rows() > static_cast<int>(maxFeatures_) ||
           templateKeypoints_.rank() != 2 || templateKeypoints_.cols() != 2 ||
           templateDescriptors_.rank() != 2 || templateDescriptors_.cols() != 128 ||
           templateDescriptors_.rows() != templateKeypoints_.rows() ||
           templateRanges_.rank() != 2 || templateRanges_.cols() != 2)
        {
            Warning("LightGlueFeatureMatcher received incompatible feature matrices");
            return;
        }

        try
        {
            float inferenceMilliseconds = 0.0f;
            const int templateLimit = std::min(templateRanges_.rows(),
                                               static_cast<int>(maxTemplates_));
            for(int templateIndex = 0; templateIndex < templateLimit; ++templateIndex)
            {
                const int start = static_cast<int>(templateRanges_(templateIndex, 0));
                const int length = static_cast<int>(templateRanges_(templateIndex, 1));
                if(start < 0 || length < 1 || length > static_cast<int>(maxFeatures_) ||
                   start + length > templateKeypoints_.rows())
                    throw std::runtime_error("Template feature range is outside the bank");

                selectedKeypoints_.resize(length, 2);
                selectedDescriptors_.resize(length, 128);
                std::copy_n(templateKeypoints_.data() + 2 * start, 2 * length,
                            selectedKeypoints_.data());
                std::copy_n(templateDescriptors_.data() + 128 * start, 128 * length,
                            selectedDescriptors_.data());

                std::vector<Ort::Value> inputs;
                inputs.reserve(6);
                inputs.emplace_back(inference_->FloatTensor(selectedKeypoints_));
                inputs.emplace_back(inference_->FloatTensor(selectedDescriptors_));
                inputs.emplace_back(inference_->FloatTensor(imageSize_));
                inputs.emplace_back(inference_->FloatTensor(keypoints_));
                inputs.emplace_back(inference_->FloatTensor(descriptors_));
                inputs.emplace_back(inference_->FloatTensor(imageSize_));
                const auto inferenceStart = std::chrono::steady_clock::now();
                std::vector<Ort::Value> outputs = inference_->Run(inputs);
                inferenceMilliseconds += std::chrono::duration<float, std::milli>(
                    std::chrono::steady_clock::now() - inferenceStart).count();
                if(outputs.size() != 2)
                    throw std::runtime_error("LightGlue returned an unexpected output count");
                const auto matchShape = outputs[0].GetTensorTypeAndShapeInfo().GetShape();
                const auto scoreShape = outputs[1].GetTensorTypeAndShapeInfo().GetShape();
                if(matchShape != std::vector<int64_t>({1, length}) ||
                   scoreShape != std::vector<int64_t>({1, length}))
                    throw std::runtime_error("LightGlue returned unexpected output shapes");

                const int64_t * matches = outputs[0].GetTensorData<int64_t>();
                const float * scores = outputs[1].GetTensorData<float>();
                const float threshold = static_cast<float>(scoreThreshold_);
                int emitted = 0;
                for(int feature = 0; feature < length; ++feature)
                    if(matches[feature] >= 0 && matches[feature] < keypoints_.rows() &&
                       scores[feature] >= threshold &&
                       emitted < static_cast<int>(maxMatchesPerTemplate_))
                        ++emitted;
                const int firstRow = correspondences_.rows();
                correspondences_.resize(firstRow + emitted, 4);
                int target = firstRow;
                for(int feature = 0; feature < length && target < firstRow + emitted;
                    ++feature)
                {
                    if(matches[feature] < 0 || matches[feature] >= keypoints_.rows() ||
                       scores[feature] < threshold)
                        continue;
                    correspondences_(target, 0) = static_cast<float>(templateIndex);
                    correspondences_(target, 1) = static_cast<float>(feature);
                    correspondences_(target, 2) = static_cast<float>(matches[feature]);
                    correspondences_(target, 3) = scores[feature];
                    ++target;
                }
            }
            smoothedInferenceMilliseconds_ = hasInferenceTiming_
                ? 0.9f * smoothedInferenceMilliseconds_ + 0.1f * inferenceMilliseconds
                : inferenceMilliseconds;
            hasInferenceTiming_ = true;
            performance_(0) = inferenceMilliseconds;
            performance_(1) = smoothedInferenceMilliseconds_;
            inferenceWarningIssued_ = false;
        }
        catch(const std::exception & error)
        {
            correspondences_.clear();
            if(!inferenceWarningIssued_)
            {
                Warning(std::string("LightGlueFeatureMatcher inference failed: ") +
                        error.what());
                inferenceWarningIssued_ = true;
            }
        }
    }
};

INSTALL_CLASS(LightGlueFeatureMatcher)
