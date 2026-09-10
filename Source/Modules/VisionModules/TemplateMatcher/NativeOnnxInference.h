#pragma once

#include "ikaros.h"

#include <filesystem>
#include <string>
#include <vector>

#include <onnxruntime_cxx_api.h>

namespace ikaros
{
    struct OnnxInferenceOptions
    {
        bool useCoreML = false;
        bool requireStaticInputShapes = false;
        bool enableMemoryPattern = true;
        std::filesystem::path modelCacheDirectory;
    };

    struct OnnxTensorContract
    {
        std::string name;
        ONNXTensorElementDataType type;
        int rank;
    };

    class NativeOnnxInference
    {
    public:
        NativeOnnxInference(const std::filesystem::path & modelPath,
                            const std::string & expectedSha256,
                            const std::vector<OnnxTensorContract> & inputs,
                            const std::vector<OnnxTensorContract> & outputs,
                            const OnnxInferenceOptions & options = {});

        Ort::Value FloatTensor(matrix & value, bool addBatchDimension = true) const;
        std::vector<Ort::Value> Run(std::vector<Ort::Value> & inputs);
        bool UsingCoreML() const;

    private:
        static Ort::Env & Environment();
        static std::string Sha256(const std::filesystem::path & path);

        void ValidateContract(const std::vector<OnnxTensorContract> & expected,
                              bool input) const;

        Ort::SessionOptions options_;
        bool usingCoreML_ = false;
        Ort::Session session_;
        Ort::MemoryInfo memory_;
        std::vector<std::string> inputNames_;
        std::vector<std::string> outputNames_;
        std::vector<const char *> inputNamePointers_;
        std::vector<const char *> outputNamePointers_;
    };
}
