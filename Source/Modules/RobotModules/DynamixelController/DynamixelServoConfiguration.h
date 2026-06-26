#pragma once

#include "ikaros.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <vector>

struct DynamixelChainDefaultSettings
{
    uint16_t pGain = 0;
    uint16_t iGain = 0;
    uint16_t dGain = 0;
};

struct DynamixelServoPositionLimit
{
    std::string chainName;
    int id = 0;
    uint32_t maxPosition = 0;
    uint32_t minPosition = 0;
};

struct DynamixelChainConfiguration
{
    std::string name;
    int idMin = 0;
    int idMax = 0;
    int ioIndex = 0;
    int baudRate = 0;
    bool parameterChain = true;
    std::string communication = "sync_indirect";
    std::map<int, std::string> models;
    std::map<int, std::string> units;
    std::map<int, std::string> labels;
    std::map<int, double> positionUnitDegrees;
    std::map<int, double> conversionFactors;
    std::map<int, double> softwareMin;
    std::map<int, double> softwareMax;
    std::map<int, double> inputMin;
    std::map<int, double> inputMax;
    std::map<int, double> positionMin;
    std::map<int, double> positionMax;
    std::map<int, bool> goalPreInverted;
    std::map<int, double> goalOffset;
    std::map<int, bool> goalPostInverted;
    std::map<int, bool> presentPositionPreInverted;
    std::map<int, double> presentPositionOffset;
    std::map<int, bool> presentPositionPostInverted;
    std::map<int, bool> presentCurrentPreInverted;
    std::map<int, double> presentCurrentOffset;
    std::map<int, bool> presentCurrentPostInverted;
    std::map<int, double> startupPosition;
    std::map<int, double> shutdownPosition;
    std::map<std::string, double> startupWrites;
    int calibrationGoalPosition = 0;
    int calibratedPositionOffset = 0;
    int calibratedPositionRange = 0;
    int calibrationTorqueLimit = 0;
    int calibrationMaxTorqueLimit = 0;
    int calibrationFullRangePosition = 0;
    double writeDelay = 0;
    double calibrationMoveDelay = 0;
    DynamixelChainDefaultSettings defaults;
    std::vector<DynamixelServoPositionLimit> defaultPositionLimits;
};

struct DynamixelRobotConfiguration
{
    std::string type;
    std::map<std::string, std::string> serialPorts;
};

class DynamixelServoConfiguration
{
public:
    using FileResolver = std::function<bool(const std::string &, std::filesystem::path &)>;

    bool
    Load(FileResolver resolveFile);

    const std::string &
    last_error() const;

    std::map<std::string, DynamixelRobotConfiguration> robots;
    std::map<std::string, std::vector<DynamixelChainConfiguration>> robotTypeChains;
    std::map<std::string, std::filesystem::path> robotTypePaths;
    ikaros::dictionary modelRegistry;
    ikaros::dictionary controlTables;
    ikaros::dictionary indirectLayouts;

private:
    FileResolver resolveFile;
    std::string error;

    bool
    LoadModelRegistry();
    bool
    LoadRobotConfiguration();
    bool
    LoadControlTable();
    bool
    ValidateIndirectLayout(const std::string & layoutName, const ikaros::dictionary & layout);
    bool
    ValidateControlTable(const std::string & tableName, const ikaros::dictionary & table);
    bool
    ValidateDirectPositionStartupWrites();
    bool
    ValidateRobotTypeChains(const std::string & robotType, const std::vector<DynamixelChainConfiguration> & chains);
    bool
    ValidateRobotSerialPorts(const std::string & robotName, const DynamixelRobotConfiguration & robot);
    bool
    Resolve(const std::string & filename, std::filesystem::path & resolvedPath);
};
