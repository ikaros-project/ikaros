#include "DynamixelServoParameterFile.h"

#include <algorithm>
#include <utility>

using namespace ikaros;

namespace
{
    bool
    RequireDictionary(dictionary & data, const std::string & key, const std::string & context, std::string & error)
    {
        if (data.contains(key) && data[key].is_dictionary())
            return true;

        error = context + " is missing " + key + ".";
        return false;
    }
}

DynamixelServoParameterFile::DynamixelServoParameterFile(std::string robotType_, std::string controlMode_, FileResolver resolveFile_):
    robotType(std::move(robotType_)),
    controlMode(std::move(controlMode_)),
    filename("ServoParameters/ServoParameters" + robotType + "_" + controlMode + ".json"),
    resolveFile(std::move(resolveFile_))
{
}

bool
DynamixelServoParameterFile::Resolve(std::filesystem::path & resolvedPath)
{
    if (resolveFile && resolveFile(filename, resolvedPath))
        return true;

    error = "Parameter file " + filename + " is outside the allowed read roots.";
    return false;
}

bool
DynamixelServoParameterFile::Exists()
{
    std::filesystem::path resolvedPath;
    if (!Resolve(resolvedPath))
        return false;

    return std::filesystem::exists(resolvedPath);
}

bool
DynamixelServoParameterFile::Load()
{
    if (loaded)
        return true;

    std::filesystem::path resolvedPath;
    if (!Resolve(resolvedPath))
        return false;

    try
    {
        jsonData.load_json(resolvedPath.string());
    }
    catch (const std::exception & e)
    {
        error = "Could not read parameter file " + resolvedPath.string() + ": " + e.what();
        return false;
    }

    loaded = true;
    return true;
}

matrix
DynamixelServoParameterFile::ParametersFor(DynamixelServoChain & chain, const dictionary & controlTable)
{
    std::string tunedParameters;

    if (!Load())
        return matrix();

    if (!RequireDictionary(jsonData, "robots", "Parameter file " + filename, error))
        return matrix();

    dictionary & robotsData = jsonData["robots"].as_dictionary();
    if (!RequireDictionary(robotsData, robotType, "Parameter file " + filename, error))
        return matrix();

    dictionary & robotData = robotsData[robotType].as_dictionary();
    if (!RequireDictionary(robotData, chain.name, "Parameter file " + filename + " robot " + robotType, error))
        return matrix();

    dictionary & chainData = robotData[chain.name].as_dictionary();
    if (!RequireDictionary(chainData, "servos", "Parameter file " + filename + " chain " + chain.name, error))
        return matrix();

    dictionary & servoData = chainData["servos"].as_dictionary();
    std::string firstIDKey = std::to_string(chain.idMin);
    if (!servoData.contains(firstIDKey))
    {
        error = "Parameter file " + filename + " is missing servo ID " + firstIDKey + " for chain " + chain.name + ".";
        return matrix();
    }

    chain.parameterNames.clear();
    if (!servoData[firstIDKey].is_dictionary())
    {
        error = "Parameter file " + filename + " has invalid parameter values for servo ID " + firstIDKey + " in chain " + chain.name + ".";
        return matrix();
    }
    dictionary & firstServoParameters = servoData[firstIDKey].as_dictionary();
    for (auto & parameter : firstServoParameters)
    {
        if (!controlTable.contains(parameter.first))
        {
            error = "Parameter file " + filename + " contains unknown parameter " + parameter.first + " for chain " + chain.name + ".";
            return matrix();
        }
        chain.parameterNames.push_back(parameter.first);
    }

    std::sort(chain.parameterNames.begin(), chain.parameterNames.end());
    for (auto & parameterName : chain.parameterNames)
        tunedParameters += "\"" + parameterName + "\", ";

    matrix result(chain.size(), static_cast<int>(chain.parameterNames.size()));

    for (int id = chain.idMin; id <= chain.idMax; id++)
    {
        std::string idKey = std::to_string(id);
        if (!servoData.contains(idKey))
        {
            error = "Parameter file " + filename + " is missing servo ID " + idKey + " for chain " + chain.name + ".";
            return matrix();
        }

        value & valuesValue = servoData[idKey];
        if (!valuesValue.is_dictionary())
        {
            error = "Parameter file " + filename + " has invalid parameter values for servo ID " + idKey + " in chain " + chain.name + ".";
            return matrix();
        }
        dictionary & values = valuesValue.as_dictionary();

        for (size_t param = 0; param < chain.parameterNames.size(); param++)
        {
            const std::string & parameterName = chain.parameterNames[param];
            if (!values.contains(parameterName))
            {
                error = "Parameter file " + filename + " is missing parameter " + parameterName + " for servo ID " + idKey + " in chain " + chain.name + ".";
                return matrix();
            }
            result(chain.row(id), static_cast<int>(param)) = values[parameterName];
        }
    }

    result.set_labels(0, tunedParameters);
    return result;
}

const std::string &
DynamixelServoParameterFile::last_error() const
{
    return error;
}
