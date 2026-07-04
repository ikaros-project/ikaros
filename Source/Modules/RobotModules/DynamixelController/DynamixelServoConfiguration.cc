#include "DynamixelServoConfiguration.h"

#include <cmath>
#include <set>
#include <utility>

using namespace ikaros;

namespace
{
    constexpr double SUPPORTED_PROTOCOL_VERSION = 2.0;


    bool
    RequireKey(const dictionary & data, const std::string & key, const std::string & context, std::string & error)
    {
        if (data.contains(key))
            return true;

        error = context + " is missing " + key + ".";
        return false;
    }

    bool
    ValidateOffset(const dictionary & data, const std::string & layoutName, const std::string & groupName, const std::string & parameterName, int byteCount, int dataLength, std::string & error)
    {
        const dictionary & offsets = data[groupName].as_dictionary();
        if (!offsets.contains(parameterName))
        {
            error = "Servo indirect layout " + layoutName + " " + groupName + " is missing required entry " + parameterName + ".";
            return false;
        }

        const int offset = offsets[parameterName].as_int();
        if (offset < 0 || offset + byteCount > dataLength)
        {
            error = "Servo indirect layout " + layoutName + " " + groupName + " entry " + parameterName +
                " has offset " + std::to_string(offset) +
                " and byte width " + std::to_string(byteCount) +
                " outside data length " + std::to_string(dataLength) + ".";
            return false;
        }

        return true;
    }
}

bool
DynamixelServoConfiguration::Load(FileResolver resolveFile_)
{
    resolveFile = std::move(resolveFile_);
    robots.clear();
    robotTypeChains.clear();
    robotTypePaths.clear();
    modelRegistry = dictionary();
    controlTables = dictionary();
    indirectLayouts = dictionary();
    error.clear();

    try
    {
        return LoadModelRegistry() && LoadRobotConfiguration() && LoadControlTable();
    }
    catch (const std::exception & e)
    {
        error = "Invalid servo configuration: " + std::string(e.what());
        return false;
    }
}

bool
DynamixelServoConfiguration::Resolve(const std::string & filename, std::filesystem::path & resolvedPath)
{
    if (resolveFile && resolveFile(filename, resolvedPath))
        return true;

    error = "Configuration file " + filename + " is outside the allowed read roots.";
    return false;
}


bool
DynamixelServoConfiguration::LoadModelRegistry()
{
    std::filesystem::path resolvedPath;
    if (!Resolve("DynamixelModels.json", resolvedPath))
        return false;

    dictionary registryIndex;
    try
    {
        registryIndex.load_json(resolvedPath.string());
    }
    catch (const std::exception & e)
    {
        error = "Could not read Dynamixel model registry file " + resolvedPath.string() + ": " + e.what();
        return false;
    }

    if (!registryIndex.contains("model_files") || !registryIndex["model_files"].is_dictionary())
    {
        error = "Dynamixel model registry is missing model_files.";
        return false;
    }

    modelRegistry["format"] = registryIndex["format"].as_string();
    modelRegistry["models"] = dictionary();

    const dictionary & modelFiles = registryIndex["model_files"].as_dictionary();
    for (const auto & modelFile : modelFiles)
    {
        std::filesystem::path modelPath;
        const std::string modelName = modelFile.first;
        const std::string modelFilename = modelFile.second.as_string();
        if (modelFilename.empty())
        {
            error = "Dynamixel model registry entry " + modelName + " has an empty filename.";
            return false;
        }

        if (!Resolve(modelFilename, modelPath))
            return false;

        dictionary model;
        try
        {
            model.load_json(modelPath.string());
        }
        catch (const std::exception & e)
        {
            error = "Could not read Dynamixel model file " + modelPath.string() + ": " + e.what();
            return false;
        }

        const std::string modelContext = "Dynamixel model file " + modelFilename;
        if (!RequireKey(model, "model", modelContext, error))
            return false;

        if (model["model"].as_string() != modelName)
        {
            error = "Dynamixel model file " + modelFilename + " identifies model " +
                model["model"].as_string() + " but the registry key is " + modelName + ".";
            return false;
        }

        if (!RequireKey(model, "manual_url", modelContext, error) ||
            !RequireKey(model, "protocol_version", modelContext, error) ||
            !RequireKey(model, "control_table", modelContext, error) ||
            !RequireKey(model, "position", modelContext, error))
            return false;

        const double protocolVersion = model["protocol_version"].as_float();
        if (protocolVersion != SUPPORTED_PROTOCOL_VERSION)
        {
            error = "Dynamixel model file " + modelFilename + " uses protocol version " +
                std::to_string(protocolVersion) +
                ", but DynamixelController currently supports only protocol version 2.0.";
            return false;
        }

        if (!model["position"].is_dictionary())
        {
            error = "Dynamixel model file " + modelFilename + " position must be a dictionary.";
            return false;
        }

        const dictionary & position = model["position"].as_dictionary();
        const std::string positionContext = "Dynamixel model file " + modelFilename + " position";
        if (!RequireKey(position, "unit_degrees", positionContext, error) ||
            !RequireKey(position, "min", positionContext, error) ||
            !RequireKey(position, "max", positionContext, error))
            return false;

        modelRegistry["models"][modelName] = model;
    }

    return true;
}


bool
DynamixelServoConfiguration::LoadRobotConfiguration()
{
    std::filesystem::path resolvedPath;
    if (!Resolve("ServoConfiguration.json", resolvedPath))
        return false;

    dictionary config;
    try
    {
        config.load_json(resolvedPath.string());
    }
    catch (const std::exception & e)
    {
        error = "Could not read servo configuration file " + resolvedPath.string() + ": " + e.what();
        return false;
    }

    if (config.contains("robot_type_files"))
    {
        if (!config["robot_type_files"].is_dictionary())
        {
            error = "Servo configuration file robot_type_files must be a dictionary.";
            return false;
        }

        const dictionary robotTypeFiles = config["robot_type_files"].as_dictionary();
        config["robot_types"] = dictionary();

        for (const auto & robotTypeFile : robotTypeFiles)
        {
            const std::string robotType = robotTypeFile.first;
            const std::string filename = robotTypeFile.second.as_string();
            if (filename.empty())
            {
                error = "Servo configuration robot type " + robotType + " has an empty filename.";
                return false;
            }

            std::filesystem::path robotTypePath;
            if (!Resolve(filename, robotTypePath))
                return false;

            dictionary robotTypeConfig;
            try
            {
                robotTypeConfig.load_json(robotTypePath.string());
            }
            catch (const std::exception & e)
            {
                error = "Could not read robot type configuration file " + robotTypePath.string() + ": " + e.what();
                return false;
            }

            const std::string robotTypeContext = "Robot type configuration file " + filename;
            if (!RequireKey(robotTypeConfig, "type", robotTypeContext, error))
                return false;

            if (robotTypeConfig["type"].as_string() != robotType)
            {
                error = "Robot type configuration file " + filename + " identifies type " +
                    robotTypeConfig["type"].as_string() + " but the registry key is " + robotType + ".";
                return false;
            }

            config["robot_types"][robotType] = robotTypeConfig;
            robotTypePaths[robotType] = robotTypePath;
        }
    }

    if (config.contains("robot_files"))
    {
        if (!config["robot_files"].is_dictionary())
        {
            error = "Servo configuration file robot_files must be a dictionary.";
            return false;
        }

        const dictionary robotFiles = config["robot_files"].as_dictionary();
        config["robots"] = dictionary();

        for (const auto & robotFile : robotFiles)
        {
            const std::string robotName = robotFile.first;
            const std::string filename = robotFile.second.as_string();
            if (filename.empty())
            {
                error = "Servo configuration robot " + robotName + " has an empty filename.";
                return false;
            }

            std::filesystem::path robotPath;
            if (!Resolve(filename, robotPath))
                return false;

            dictionary robotConfig;
            try
            {
                robotConfig.load_json(robotPath.string());
            }
            catch (const std::exception & e)
            {
                error = "Could not read robot configuration file " + robotPath.string() + ": " + e.what();
                return false;
            }

            const std::string robotContext = "Robot configuration file " + filename;
            if (!RequireKey(robotConfig, "robot", robotContext, error))
                return false;

            if (robotConfig["robot"].as_string() != robotName)
            {
                error = "Robot configuration file " + filename + " identifies robot " +
                    robotConfig["robot"].as_string() + " but the registry key is " + robotName + ".";
                return false;
            }

            config["robots"][robotName] = robotConfig;
        }
    }

    if (!config.contains("robot_types") || !config["robot_types"].is_dictionary())
    {
        error = "Servo configuration file is missing robot_types.";
        return false;
    }

    const dictionary & typeData = config["robot_types"].as_dictionary();
    for (const auto & typeEntry : typeData)
    {
        if (!typeEntry.second.is_dictionary())
        {
            error = "Servo configuration for robot type " + typeEntry.first + " must be a dictionary.";
            return false;
        }

        const dictionary & typeConfig = typeEntry.second.as_dictionary();
        if (!typeConfig.contains("chains") || !typeConfig["chains"].is_list())
        {
            error = "Servo configuration for robot type " + typeEntry.first + " is missing chains.";
            return false;
        }

        std::vector<DynamixelChainConfiguration> chains;
        const list & chainList = typeConfig["chains"].as_list();
        for (const value & chainValue : chainList)
        {
            if (!chainValue.is_dictionary())
            {
                error = "Servo chain configuration for robot type " + typeEntry.first + " must be a dictionary.";
                return false;
            }

            const dictionary & chainData = chainValue.as_dictionary();
            const std::string chainContext = "Servo chain configuration for robot type " + typeEntry.first;
            if (!RequireKey(chainData, "name", chainContext, error) ||
                !RequireKey(chainData, "id_min", chainContext, error) ||
                !RequireKey(chainData, "id_max", chainContext, error) ||
                !RequireKey(chainData, "io_index", chainContext, error) ||
                !RequireKey(chainData, "baud_rate", chainContext, error) ||
                !RequireKey(chainData, "servos", chainContext, error))
                return false;

            DynamixelChainConfiguration chain;
            chain.name = chainData["name"].as_string();
            chain.idMin = chainData["id_min"].as_int();
            chain.idMax = chainData["id_max"].as_int();
            chain.ioIndex = chainData["io_index"].as_int();
            chain.baudRate = chainData["baud_rate"].as_int();
            chain.parameterChain = !chainData.contains("parameter_chain") || chainData["parameter_chain"].is_true();
            chain.communication = chainData.contains("communication") ? chainData["communication"].as_string() : (chain.parameterChain ? "sync_indirect" : "direct_position");

            if (!chainData["servos"].is_dictionary())
            {
                error = "Servo configuration for chain " + chain.name + " must be a dictionary.";
                return false;
            }

            const dictionary & servos = chainData["servos"].as_dictionary();
            for (int id = chain.idMin; id <= chain.idMax; id++)
            {
                std::string idKey = std::to_string(id);
                if (!servos.contains(idKey) || !servos[idKey].is_dictionary())
                {
                    error = "Servo configuration for chain " + chain.name + " is missing servo ID " + idKey + ".";
                    return false;
                }

                const dictionary & servoData = servos[idKey].as_dictionary();
                const std::string servoContext = "Servo configuration for chain " + chain.name + " servo ID " + idKey;
                if (!RequireKey(servoData, "model", servoContext, error) ||
                    !RequireKey(servoData, "unit", servoContext, error) ||
                    !RequireKey(servoData, "label", servoContext, error))
                    return false;

                chain.models[id] = servoData["model"].as_string();
                chain.units[id] = servoData["unit"].as_string();
                chain.labels[id] = servoData["label"].as_string();
                if (chain.models[id].empty())
                {
                    error = "Servo model configuration for chain " + chain.name + " servo ID " + idKey + " is empty.";
                    return false;
                }
                if (chain.units[id].empty())
                {
                    error = "Servo unit configuration for chain " + chain.name + " servo ID " + idKey + " is empty.";
                    return false;
                }
                if (chain.labels[id].empty())
                {
                    error = "Servo label configuration for chain " + chain.name + " servo ID " + idKey + " is empty.";
                    return false;
                }

                const dictionary & registeredModels = modelRegistry["models"].as_dictionary();
                if (!registeredModels.contains(chain.models[id]))
                {
                    error = "Servo model configuration for chain " + chain.name + " servo ID " + idKey +
                        " references unknown Dynamixel model " + chain.models[id] + ".";
                    return false;
                }

                const dictionary & model = registeredModels[chain.models[id]].as_dictionary();
                chain.positionUnitDegrees[id] = model["position"]["unit_degrees"].as_float();
                if (chain.positionUnitDegrees[id] == 0)
                {
                    error = "Dynamixel model " + chain.models[id] + " has zero position unit_degrees.";
                    return false;
                }

                if (servoData.contains("conversion_factor"))
                    chain.conversionFactors[id] = servoData["conversion_factor"].as_float();
                else if (chain.units[id] == "degree")
                    chain.conversionFactors[id] = 1.0;
                else
                {
                    error = "Servo configuration for chain " + chain.name + " servo ID " + idKey +
                        " must set conversion_factor for unit " + chain.units[id] + ".";
                    return false;
                }

                if (chain.conversionFactors[id] == 0)
                {
                    error = "Servo conversion factor for chain " + chain.name + " servo ID " + idKey + " must be non-zero.";
                    return false;
                }

                chain.goalPreInverted[id] = servoData.contains("goal_pre_inverted") && servoData["goal_pre_inverted"].is_true();
                chain.goalOffset[id] = servoData.contains("goal_offset") ? servoData["goal_offset"].as_float() : 0.0;
                chain.goalPostInverted[id] = servoData.contains("goal_post_inverted") && servoData["goal_post_inverted"].is_true();
                chain.presentPositionPreInverted[id] = servoData.contains("present_position_pre_inverted") && servoData["present_position_pre_inverted"].is_true();
                chain.presentPositionOffset[id] = servoData.contains("present_position_offset") ? servoData["present_position_offset"].as_float() : 0.0;
                chain.presentPositionPostInverted[id] = servoData.contains("present_position_post_inverted") && servoData["present_position_post_inverted"].is_true();
                chain.presentCurrentPreInverted[id] = servoData.contains("present_current_pre_inverted") && servoData["present_current_pre_inverted"].is_true();
                chain.presentCurrentOffset[id] = servoData.contains("present_current_offset") ? servoData["present_current_offset"].as_float() : 0.0;
                chain.presentCurrentPostInverted[id] = servoData.contains("present_current_post_inverted") && servoData["present_current_post_inverted"].is_true();
                if (servoData.contains("startup_position"))
                    chain.startupPosition[id] = servoData["startup_position"].as_float();
                if (servoData.contains("shutdown_position"))
                    chain.shutdownPosition[id] = servoData["shutdown_position"].as_float();

                if (chain.communication == "direct_position")
                {
                    if (!RequireKey(servoData, "direct_position", servoContext, error))
                        return false;

                    if (!servoData["direct_position"].is_dictionary())
                    {
                        error = servoContext + " direct_position must be a dictionary.";
                        return false;
                    }

                    const dictionary & directPosition = servoData["direct_position"].as_dictionary();
                    const std::string directPositionContext = servoContext + " direct_position";
                    if (!RequireKey(directPosition, "software_min", directPositionContext, error) ||
                        !RequireKey(directPosition, "software_max", directPositionContext, error) ||
                        !RequireKey(directPosition, "input_min", directPositionContext, error) ||
                        !RequireKey(directPosition, "input_max", directPositionContext, error) ||
                        !RequireKey(directPosition, "position_min", directPositionContext, error) ||
                        !RequireKey(directPosition, "position_max", directPositionContext, error))
                        return false;

                    chain.softwareMin[id] = directPosition["software_min"].as_float();
                    chain.softwareMax[id] = directPosition["software_max"].as_float();
                    chain.inputMin[id] = directPosition["input_min"].as_float();
                    chain.inputMax[id] = directPosition["input_max"].as_float();
                    chain.positionMin[id] = directPosition["position_min"].as_float();
                    chain.positionMax[id] = directPosition["position_max"].as_float();

                    if (chain.softwareMax[id] <= chain.softwareMin[id] ||
                        chain.inputMax[id] <= chain.inputMin[id] ||
                        chain.positionMax[id] <= chain.positionMin[id])
                    {
                        error = "Direct-position ranges for chain " + chain.name + " servo ID " + idKey + " are invalid.";
                        return false;
                    }
                }

                if (chain.communication == "sync_indirect" || servoData.contains("min") || servoData.contains("max"))
                {
                    const std::string limitContext = "Default position limit for chain " + chain.name + " servo ID " + idKey;
                    if (!RequireKey(servoData, "min", limitContext, error) ||
                        !RequireKey(servoData, "max", limitContext, error))
                        return false;

                    DynamixelServoPositionLimit limit;
                    limit.chainName = chain.name;
                    limit.id = id;
                    limit.minPosition = servoData["min"].as_int();
                    limit.maxPosition = servoData["max"].as_int();
                    chain.defaultPositionLimits.push_back(limit);
                }
            }

            if (chainData.contains("defaults") && chainData["defaults"].is_dictionary())
            {
                const dictionary & defaults = chainData["defaults"].as_dictionary();
                const std::string defaultsContext = "Default servo settings for chain " + chain.name;
                if (!RequireKey(defaults, "p_gain", defaultsContext, error) ||
                    !RequireKey(defaults, "i_gain", defaultsContext, error) ||
                    !RequireKey(defaults, "d_gain", defaultsContext, error))
                    return false;

                chain.defaults.pGain = defaults["p_gain"].as_int();
                chain.defaults.iGain = defaults["i_gain"].as_int();
                chain.defaults.dGain = defaults["d_gain"].as_int();
            }

            if (chain.communication == "direct_position")
            {
                const std::string directContext = "Direct-position configuration for chain " + chain.name;
                if (!RequireKey(chainData, "startup_writes", directContext, error))
                    return false;

                if (!chainData["startup_writes"].is_dictionary())
                {
                    error = directContext + " startup_writes must be a dictionary.";
                    return false;
                }

                const dictionary & startupWrites = chainData["startup_writes"].as_dictionary();
                if (startupWrites.empty())
                {
                    error = directContext + " startup_writes must not be empty.";
                    return false;
                }

                for (const auto & startupWrite : startupWrites)
                {
                    if (startupWrite.first.empty())
                    {
                        error = directContext + " startup write name must not be empty.";
                        return false;
                    }

                    chain.startupWrites[startupWrite.first] = startupWrite.second.as_float();
                }
            }

            if (chainData.contains("detect_range"))
            {
                const std::string detectRangeContext = "Detect-range configuration for chain " + chain.name;
                if (!chainData["detect_range"].is_dictionary())
                {
                    error = detectRangeContext + " must be a dictionary.";
                    return false;
                }

                const dictionary & detectRange = chainData["detect_range"].as_dictionary();
                if (!RequireKey(detectRange, "first_goal_position", detectRangeContext, error) ||
                    !RequireKey(detectRange, "second_goal_position", detectRangeContext, error) ||
                    !RequireKey(detectRange, "position_min_offset", detectRangeContext, error) ||
                    !RequireKey(detectRange, "position_max_offset", detectRangeContext, error) ||
                    !RequireKey(detectRange, "moving_speed", detectRangeContext, error) ||
                    !RequireKey(detectRange, "torque_limit", detectRangeContext, error) ||
                    !RequireKey(detectRange, "max_torque_limit", detectRangeContext, error) ||
                    !RequireKey(detectRange, "full_range_position", detectRangeContext, error) ||
                    !RequireKey(detectRange, "write_delay", detectRangeContext, error) ||
                    !RequireKey(detectRange, "move_delay", detectRangeContext, error))
                    return false;

                chain.detectRange = true;
                chain.detectRangeFirstGoalPosition = detectRange["first_goal_position"].as_int();
                chain.detectRangeSecondGoalPosition = detectRange["second_goal_position"].as_int();
                chain.detectRangePositionMinOffset = detectRange["position_min_offset"].as_int();
                chain.detectRangePositionMaxOffset = detectRange["position_max_offset"].as_int();
                chain.detectRangeMovingSpeed = detectRange["moving_speed"].as_int();
                chain.detectRangeTorqueLimit = detectRange["torque_limit"].as_int();
                chain.detectRangeMaxTorqueLimit = detectRange["max_torque_limit"].as_int();
                chain.detectRangeFullRangePosition = detectRange["full_range_position"].as_int();
                chain.writeDelay = detectRange["write_delay"].as_float();
                chain.detectRangeMoveDelay = detectRange["move_delay"].as_float();

                if (chain.detectRangePositionMinOffset < 0 ||
                    chain.detectRangePositionMaxOffset <= chain.detectRangePositionMinOffset ||
                    chain.detectRangeFirstGoalPosition == chain.detectRangeSecondGoalPosition ||
                    chain.detectRangeMovingSpeed <= 0 ||
                    chain.detectRangeTorqueLimit <= 0 ||
                    chain.detectRangeMaxTorqueLimit <= 0 ||
                    chain.detectRangeFullRangePosition <= 0 ||
                    chain.writeDelay < 0 ||
                    chain.detectRangeMoveDelay < 0)
                {
                    error = detectRangeContext + " values are invalid.";
                    return false;
                }
            }

            if (chain.name.empty() || chain.idMax < chain.idMin || chain.baudRate <= 0)
            {
                error = "Invalid servo chain configuration for robot type " + typeEntry.first + ".";
                return false;
            }

            chains.push_back(chain);
        }

        if (!ValidateRobotTypeChains(typeEntry.first, chains))
            return false;

        robotTypeChains[typeEntry.first] = chains;
    }

    if (!config.contains("robots") || !config["robots"].is_dictionary())
    {
        error = "Servo configuration file is missing robots.";
        return false;
    }

    const dictionary & robotData = config["robots"].as_dictionary();
    for (const auto & robotEntry : robotData)
    {
        if (!robotEntry.second.is_dictionary())
        {
            error = "Servo configuration for robot " + robotEntry.first + " must be a dictionary.";
            return false;
        }

        const dictionary & robotConfig = robotEntry.second.as_dictionary();
        if (!RequireKey(robotConfig, "type", "Servo configuration for robot " + robotEntry.first, error))
            return false;

        DynamixelRobotConfiguration configuredRobot;
        configuredRobot.type = robotConfig["type"].as_string();

        if (configuredRobot.type.empty() || robotTypeChains.find(configuredRobot.type) == robotTypeChains.end())
        {
            error = "Servo configuration for robot " + robotEntry.first + " has an unknown type.";
            return false;
        }

        if (!robotConfig.contains("serial_ports") || !robotConfig["serial_ports"].is_dictionary())
        {
            error = "Servo configuration for robot " + robotEntry.first + " is missing serial_ports.";
            return false;
        }

        const dictionary & serialPorts = robotConfig["serial_ports"].as_dictionary();
        for (const auto & serialPort : serialPorts)
            configuredRobot.serialPorts[serialPort.first] = serialPort.second.as_string();

        if (!ValidateRobotSerialPorts(robotEntry.first, configuredRobot))
            return false;

        robots[robotEntry.first] = configuredRobot;
    }

    return true;
}

bool
DynamixelServoConfiguration::ValidateRobotTypeChains(const std::string & robotType, const std::vector<DynamixelChainConfiguration> & chains)
{
    std::set<std::string> chainNames;
    std::set<int> parameterIndexes;

    for (const DynamixelChainConfiguration & chain : chains)
    {
        if (!chainNames.insert(chain.name).second)
        {
            error = "Servo configuration for robot type " + robotType + " has duplicate chain name " + chain.name + ".";
            return false;
        }

        if (!chain.parameterChain)
            continue;

        for (int ioIndex = chain.ioIndex; ioIndex < chain.ioIndex + chain.idMax - chain.idMin + 1; ioIndex++)
            if (!parameterIndexes.insert(ioIndex).second)
            {
                error = "Servo configuration for robot type " + robotType + " has overlapping parameter-chain IO index " + std::to_string(ioIndex) + ".";
                return false;
            }
    }

    return true;
}

bool
DynamixelServoConfiguration::ValidateRobotSerialPorts(const std::string & robotName, const DynamixelRobotConfiguration & robot)
{
    const auto typeEntry = robotTypeChains.find(robot.type);
    if (typeEntry == robotTypeChains.end())
    {
        error = "Servo configuration for robot " + robotName + " has an unknown type.";
        return false;
    }

    for (const DynamixelChainConfiguration & chain : typeEntry->second)
    {
        const auto serialPort = robot.serialPorts.find(chain.name);
        if (serialPort == robot.serialPorts.end() || serialPort->second.empty())
        {
            error = "Servo configuration for robot " + robotName + " is missing serial port for chain " + chain.name + ".";
            return false;
        }
    }

    return true;
}

bool
DynamixelServoConfiguration::LoadControlTable()
{
    std::filesystem::path resolvedPath;
    if (!Resolve("ServoControlTable.json", resolvedPath))
        return false;

    dictionary controlTableFile;
    try
    {
        controlTableFile.load_json(resolvedPath.string());
    }
    catch (const std::exception & e)
    {
        error = "Could not read servo control table file " + resolvedPath.string() + ": " + e.what();
        return false;
    }

    if (!controlTableFile.contains("tables") || !controlTableFile["tables"].is_dictionary())
    {
        error = "Servo control table file is missing tables.";
        return false;
    }

    if (controlTableFile.contains("indirect_layouts"))
    {
        if (!controlTableFile["indirect_layouts"].is_dictionary())
        {
            error = "Servo control table file indirect_layouts must be a dictionary.";
            return false;
        }

        indirectLayouts = controlTableFile["indirect_layouts"].as_dictionary();
        for (const auto & layoutEntry : indirectLayouts)
        {
            if (!layoutEntry.second.is_dictionary())
            {
                error = "Servo indirect layout " + layoutEntry.first + " must be a dictionary.";
                return false;
            }

            if (!ValidateIndirectLayout(layoutEntry.first, layoutEntry.second.as_dictionary()))
                return false;
        }
    }

    controlTables = controlTableFile["tables"].as_dictionary();
    for (const auto & tableEntry : controlTables)
    {
        if (!tableEntry.second.is_dictionary())
        {
            error = "Servo control table " + tableEntry.first + " must be a dictionary.";
            return false;
        }

        if (!ValidateControlTable(tableEntry.first, tableEntry.second.as_dictionary()))
            return false;
    }

    const dictionary & models = modelRegistry["models"].as_dictionary();
    for (const auto & modelEntry : models)
    {
        const dictionary & model = modelEntry.second.as_dictionary();
        const std::string tableName = model["control_table"].as_string();
        if (!controlTables.contains(tableName))
        {
            error = "Dynamixel model " + modelEntry.first + " references unknown control table " + tableName + ".";
            return false;
        }

        if (model.contains("indirect_layout"))
        {
            const std::string layoutName = model["indirect_layout"].as_string();
            if (!indirectLayouts.contains(layoutName) || !indirectLayouts[layoutName].is_dictionary())
            {
                error = "Dynamixel model " + modelEntry.first + " references unknown indirect layout " + layoutName + ".";
                return false;
            }
        }
    }

    return ValidateDirectPositionStartupWrites();
}


bool
DynamixelServoConfiguration::ValidateDirectPositionStartupWrites()
{
    const dictionary & registeredModels = modelRegistry["models"].as_dictionary();

    for (const auto & robotTypeEntry : robotTypeChains)
        for (const DynamixelChainConfiguration & chain : robotTypeEntry.second)
        {
            if (chain.communication != "direct_position")
                continue;

            for (const auto & startupWrite : chain.startupWrites)
            {
                const double value = startupWrite.second;
                if (value < 0 || std::floor(value) != value)
                {
                    error = "Startup write " + startupWrite.first + " for direct-position chain " + chain.name +
                        " in robot type " + robotTypeEntry.first + " must be a non-negative integer.";
                    return false;
                }

                for (const auto & modelEntry : chain.models)
                {
                    const std::string & modelName = modelEntry.second;
                    const dictionary & model = registeredModels[modelName].as_dictionary();
                    const std::string tableName = model["control_table"].as_string();
                    const dictionary & controlTable = controlTables[tableName].as_dictionary();

                    if (!controlTable.contains(startupWrite.first) || !controlTable[startupWrite.first].is_dictionary())
                    {
                        error = "Startup write " + startupWrite.first + " for direct-position chain " + chain.name +
                            " in robot type " + robotTypeEntry.first + " is missing from control table " + tableName + ".";
                        return false;
                    }

                    const dictionary & parameter = controlTable[startupWrite.first].as_dictionary();
                    const int bytes = parameter["Bytes"].as_int();
                    if (bytes != 1 && bytes != 2)
                    {
                        error = "Startup write " + startupWrite.first + " for direct-position chain " + chain.name +
                            " in robot type " + robotTypeEntry.first + " uses unsupported byte width " + std::to_string(bytes) + ".";
                        return false;
                    }

                    const double maxValue = bytes == 1 ? 255 : 65535;
                    if (value > maxValue)
                    {
                        error = "Startup write " + startupWrite.first + " for direct-position chain " + chain.name +
                            " in robot type " + robotTypeEntry.first + " exceeds " + std::to_string(static_cast<int>(maxValue)) + ".";
                        return false;
                    }
                }
            }
        }

    return true;
}


bool
DynamixelServoConfiguration::ValidateIndirectLayout(const std::string & layoutName, const dictionary & layout)
{
    static const std::vector<std::string> requiredEntries =
    {
        "Write Data Start",
        "Write Data Length",
        "Read Data Start",
        "Read Data Length",
        "Write Offsets",
        "Read Offsets",
        "Torque Enable",
        "Goal Position",
        "Goal Current",
        "Goal PWM",
        "Present Position",
        "Present Current",
        "Present Temperature"
    };

    for (const std::string & requiredEntry : requiredEntries)
    {
        if (!layout.contains(requiredEntry))
        {
            error = "Servo indirect layout " + layoutName + " is missing required entry " + requiredEntry + ".";
            return false;
        }
    }

    if (!layout["Write Offsets"].is_dictionary())
    {
        error = "Servo indirect layout " + layoutName + " Write Offsets must be a dictionary.";
        return false;
    }

    if (!layout["Read Offsets"].is_dictionary())
    {
        error = "Servo indirect layout " + layoutName + " Read Offsets must be a dictionary.";
        return false;
    }

    const int writeDataLength = layout["Write Data Length"].as_int();
    const int readDataLength = layout["Read Data Length"].as_int();

    if (!ValidateOffset(layout, layoutName, "Write Offsets", "Torque Enable", 1, writeDataLength, error) ||
        !ValidateOffset(layout, layoutName, "Write Offsets", "Goal Position", 4, writeDataLength, error) ||
        !ValidateOffset(layout, layoutName, "Write Offsets", "Goal Current", 2, writeDataLength, error) ||
        !ValidateOffset(layout, layoutName, "Write Offsets", "Goal PWM", 2, writeDataLength, error) ||
        !ValidateOffset(layout, layoutName, "Read Offsets", "Present Position", 4, readDataLength, error) ||
        !ValidateOffset(layout, layoutName, "Read Offsets", "Present Current", 2, readDataLength, error) ||
        !ValidateOffset(layout, layoutName, "Read Offsets", "Present Temperature", 1, readDataLength, error))
        return false;

    return true;
}


bool
DynamixelServoConfiguration::ValidateControlTable(const std::string & tableName, const dictionary & table)
{
    static const std::vector<std::string> requiredEntries =
    {
        "Torque Enable",
        "Goal Position",
        "Present Position"
    };

    for (const std::string & requiredEntry : requiredEntries)
    {
        if (!table.contains(requiredEntry))
        {
            error = "Servo control table " + tableName + " is missing required entry " + requiredEntry + ".";
            return false;
        }
    }

    for (const auto & entry : table)
    {
        if (!entry.second.is_dictionary())
        {
            error = "Servo control table " + tableName + " entry " + entry.first + " must be a dictionary.";
            return false;
        }

        const dictionary & item = entry.second.as_dictionary();
        if (!item.contains("Address") || !item.contains("Bytes"))
        {
            error = "Servo control table " + tableName + " entry " + entry.first + " must include Address and Bytes.";
            return false;
        }

        int bytes = item["Bytes"].as_int();
        if (bytes != 1 && bytes != 2 && bytes != 4)
        {
            error = "Servo control table " + tableName + " entry " + entry.first +
                " has unsupported byte width " + std::to_string(bytes) + ".";
            return false;
        }
    }

    return true;
}

const std::string &
DynamixelServoConfiguration::last_error() const
{
    return error;
}
