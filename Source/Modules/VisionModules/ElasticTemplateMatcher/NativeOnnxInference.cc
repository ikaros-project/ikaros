#include "NativeOnnxInference.h"

#include <array>
#include <fstream>
#include <stdexcept>
#include <unordered_map>

#include <CommonCrypto/CommonDigest.h>

namespace ikaros
{
    namespace
    {
        std::string
        hexEncode(const unsigned char * data, size_t size)
        {
            static constexpr char hex[] = "0123456789abcdef";
            std::string result;
            result.reserve(size * 2);
            for(size_t i = 0; i < size; ++i)
            {
                result.push_back(hex[(data[i] >> 4) & 0x0f]);
                result.push_back(hex[data[i] & 0x0f]);
            }
            return result;
        }


        void
        configureCommonOptions(Ort::SessionOptions & options,
                               const OnnxInferenceOptions & configuration)
        {
            options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
            if(!configuration.enableMemoryPattern)
                options.DisableMemPattern();
        }


        Ort::Session
        createSession(Ort::Env & environment, Ort::SessionOptions & options,
                      const std::filesystem::path & modelPath,
                      const OnnxInferenceOptions & configuration, bool & usingCoreML)
        {
            configureCommonOptions(options, configuration);
            if(configuration.useCoreML)
            {
                std::unordered_map<std::string, std::string> providerOptions{
                    {"ModelFormat", "MLProgram"},
                    {"MLComputeUnits", "ALL"},
                    {"RequireStaticInputShapes",
                     configuration.requireStaticInputShapes ? "1" : "0"},
                    {"EnableOnSubgraphs", "0"},
                    {"SpecializationStrategy", "FastPrediction"},
                };
                if(!configuration.modelCacheDirectory.empty())
                    providerOptions["ModelCacheDirectory"] =
                        configuration.modelCacheDirectory.string();

                try
                {
                    options.AppendExecutionProvider("CoreML", providerOptions);
                    Ort::Session session(environment, modelPath.c_str(), options);
                    usingCoreML = true;
                    return session;
                }
                catch(const Ort::Exception &)
                {
                    options = Ort::SessionOptions{};
                    configureCommonOptions(options, configuration);
                }
            }

            usingCoreML = false;
            return Ort::Session(environment, modelPath.c_str(), options);
        }
    }


    NativeOnnxInference::NativeOnnxInference(
        const std::filesystem::path & modelPath,
        const std::string & expectedSha256,
        const std::vector<OnnxTensorContract> & inputs,
        const std::vector<OnnxTensorContract> & outputs,
        const OnnxInferenceOptions & configuration)
        : options_(),
          session_([&]() -> Ort::Session
          {
              if(!std::filesystem::is_regular_file(modelPath))
                  throw std::runtime_error("ONNX model is not a regular file: " +
                                           modelPath.string());
              if(modelPath.extension() != ".onnx")
                  throw std::runtime_error("ONNX model must have an .onnx extension: " +
                                           modelPath.string());
              if(!expectedSha256.empty() && Sha256(modelPath) != expectedSha256)
                  throw std::runtime_error("ONNX model checksum does not match: " +
                                           modelPath.string());

              return createSession(Environment(), options_, modelPath, configuration,
                                   usingCoreML_);
          }()),
          memory_(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault))
    {
        ValidateContract(inputs, true);
        ValidateContract(outputs, false);

        for(const auto & contract : inputs)
            inputNames_.push_back(contract.name);
        for(const auto & contract : outputs)
            outputNames_.push_back(contract.name);
        for(const std::string & name : inputNames_)
            inputNamePointers_.push_back(name.c_str());
        for(const std::string & name : outputNames_)
            outputNamePointers_.push_back(name.c_str());
    }


    bool
    NativeOnnxInference::UsingCoreML() const
    {
        return usingCoreML_;
    }


    Ort::Env &
    NativeOnnxInference::Environment()
    {
        static Ort::Env environment(ORT_LOGGING_LEVEL_WARNING, "ikaros-native-onnx");
        return environment;
    }


    std::string
    NativeOnnxInference::Sha256(const std::filesystem::path & path)
    {
        std::ifstream stream(path, std::ios::binary);
        if(!stream)
            throw std::runtime_error("Could not read ONNX model: " + path.string());

        CC_SHA256_CTX context;
        CC_SHA256_Init(&context);
        std::array<char, 64 * 1024> buffer;
        while(stream)
        {
            stream.read(buffer.data(), buffer.size());
            const std::streamsize count = stream.gcount();
            if(count > 0)
                CC_SHA256_Update(&context, buffer.data(), static_cast<CC_LONG>(count));
        }
        if(!stream.eof())
            throw std::runtime_error("Could not read complete ONNX model: " + path.string());

        unsigned char digest[CC_SHA256_DIGEST_LENGTH];
        CC_SHA256_Final(digest, &context);
        return hexEncode(digest, sizeof(digest));
    }


    void
    NativeOnnxInference::ValidateContract(
        const std::vector<OnnxTensorContract> & expected, bool input) const
    {
        const size_t count = input ? session_.GetInputCount() : session_.GetOutputCount();
        if(count != expected.size())
            throw std::runtime_error("ONNX model tensor count does not match its contract");

        Ort::AllocatorWithDefaultOptions allocator;
        for(size_t i = 0; i < count; ++i)
        {
            Ort::AllocatedStringPtr name = input ? session_.GetInputNameAllocated(i, allocator)
                                                  : session_.GetOutputNameAllocated(i, allocator);
            Ort::TypeInfo info = input ? session_.GetInputTypeInfo(i)
                                       : session_.GetOutputTypeInfo(i);
            const auto tensorInfo = info.GetTensorTypeAndShapeInfo();
            if(expected[i].name != name.get() ||
               expected[i].type != tensorInfo.GetElementType() ||
               expected[i].rank != static_cast<int>(tensorInfo.GetShape().size()))
                throw std::runtime_error("ONNX model tensor contract mismatch at " +
                                         std::string(input ? "input " : "output ") +
                                         std::to_string(i));
        }
    }


    Ort::Value
    NativeOnnxInference::FloatTensor(matrix & value, bool addBatchDimension) const
    {
        std::vector<int64_t> dimensions;
        dimensions.reserve(value.rank() + (addBatchDimension ? 1 : 0));
        if(addBatchDimension)
            dimensions.push_back(1);
        for(int i = 0; i < value.rank(); ++i)
            dimensions.push_back(value.shape(i));

        return Ort::Value::CreateTensor<float>(memory_, value.data(), value.size(),
                                               dimensions.data(), dimensions.size());
    }


    std::vector<Ort::Value>
    NativeOnnxInference::Run(std::vector<Ort::Value> & inputs)
    {
        if(inputs.size() != inputNamePointers_.size())
            throw std::runtime_error("ONNX inference input count does not match the model");
        return session_.Run(Ort::RunOptions{nullptr}, inputNamePointers_.data(),
                            inputs.data(), inputs.size(), outputNamePointers_.data(),
                            outputNamePointers_.size());
    }
}
