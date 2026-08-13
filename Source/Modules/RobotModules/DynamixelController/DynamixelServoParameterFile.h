#pragma once

#include "DynamixelServoChain.h"
#include "ikaros.h"

#include <filesystem>
#include <functional>
#include <string>

class DynamixelServoParameterFile
{
public:
    using FileResolver = std::function<bool(const std::string &, std::filesystem::path &)>;

    DynamixelServoParameterFile(std::string robotType_, std::string controlMode_, FileResolver resolveFile_);

    bool
    Exists();
    bool
    Load();
    ikaros::matrix
    ParametersFor(DynamixelServoChain & chain, const ikaros::dictionary & controlTable);

    const std::string &
    last_error() const;

private:
    std::string robotType;
    std::string controlMode;
    std::string filename;
    FileResolver resolveFile;
    ikaros::dictionary jsonData;
    bool loaded = false;
    std::string error;

    bool
    Resolve(std::filesystem::path & resolvedPath);
};
