#pragma once

#include "ikaros.h"

#include <filesystem>
#include <string>
#include <vector>

#include <onnxruntime_cxx_api.h>

namespace ikaros
{
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
                            const std::vector<OnnxTensorContract> & outputs);

        Ort::Value FloatTensor(matrix & value, bool addBatchDimension = true) const;
        std::vector<Ort::Value> Run(std::vector<Ort::Value> & inputs);

    private:
        static Ort::Env & Environment();
        static std::string Sha256(const std::filesystem::path & path);

        void ValidateContract(const std::vector<OnnxTensorContract> & expected,
                              bool input) const;

        Ort::SessionOptions options_;
        Ort::Session session_;
        Ort::MemoryInfo memory_;
        std::vector<std::string> inputNames_;
        std::vector<std::string> outputNames_;
        std::vector<const char *> inputNamePointers_;
        std::vector<const char *> outputNamePointers_;
    };
}
