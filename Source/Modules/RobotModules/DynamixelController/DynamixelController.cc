//
//	DynamixelController.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2026 Birger Johansson, Pierre Klintefors, and Christian Balkenius

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "ikaros.h"
#include "DynamixelServoChain.h"
#include "DynamixelServoConfiguration.h"
#include "DynamixelServoParameterFile.h"

using namespace ikaros;

namespace
{
    std::string
    PrettyJson(const std::string & json)
    {
        std::string formatted;
        int indent = 0;
        bool inString = false;
        bool escaped = false;

        auto appendIndent = [&formatted, &indent]
        {
            formatted.append(static_cast<size_t>(indent) * 4, ' ');
        };

        for (char c : json)
        {
            if (inString)
            {
                formatted += c;
                if (escaped)
                    escaped = false;
                else if (c == '\\')
                    escaped = true;
                else if (c == '"')
                    inString = false;
                continue;
            }

            if (std::isspace(static_cast<unsigned char>(c)))
                continue;

            switch (c)
            {
                case '"':
                    inString = true;
                    formatted += c;
                    break;

                case '{':
                case '[':
                    formatted += c;
                    formatted += '\n';
                    indent++;
                    appendIndent();
                    break;

                case '}':
                case ']':
                    formatted += '\n';
                    indent = std::max(0, indent - 1);
                    appendIndent();
                    formatted += c;
                    break;

                case ',':
                    formatted += c;
                    formatted += '\n';
                    appendIndent();
                    break;

                case ':':
                    formatted += ": ";
                    break;

                default:
                    formatted += c;
                    break;
            }
        }

        formatted += '\n';
        return formatted;
    }

    bool
    ParsePositiveInt(const std::string & value, int & parsedValue)
    {
        try
        {
            size_t parsedLength = 0;
            parsedValue = std::stoi(value, &parsedLength);
            return parsedLength == value.size() && parsedValue > 0;
        }
        catch (...)
        {
            parsedValue = 0;
            return false;
        }
    }

    bool
    HasControlTableEntry(const dictionary & controlTable, const std::string & parameterName, int expectedBytes)
    {
        if (!controlTable.contains(parameterName) || !controlTable[parameterName].is_dictionary())
            return false;

        const dictionary & parameter = controlTable[parameterName].as_dictionary();
        return parameter.contains("Address") && parameter.contains("Bytes") && parameter["Bytes"].as_int() == expectedBytes;
    }
}

class DynamixelController : public Module
{
    enum class ControllerMode
    {
        idle,
        initialization,
        rampUp,
        startUp,
        shutDown,
        rampDown,
        operation,
        torqueOff
    };

    enum class InitializationStep
    {
        openChains,
        readTorqueState,
        createSyncObjects,
        loadParameters,
        applySettings,
        applyPositionLimits,
        applyDefaultSettings,
        detectRangeBegin,
        detectRangeWait,
        detectRangeFinish,
        detectRange,
        complete
    };

    enum class DetectRangeStep
    {
        disableTorque,
        writeCWAngleLimit,
        writeCCWAngleLimit,
        writeDetectRangeTorqueLimit,
        enableTorque,
        writeDetectRangeGoalPosition,
        readPresentPosition,
        disableTorqueAfterRead,
        restoreTorqueLimit,
        done
    };

    parameter simulate;
    parameter robot;
    parameter servoCount;
    parameter usePositionLimitParameters;
    matrix minLimitPosition;
    matrix maxLimitPosition;
    parameter servoControlMode;
    parameter simulationRate;

    matrix goalPosition;
    matrix goalCurrent;
    matrix torqueEnable = true;
    matrix goalPWM;

    matrix presentPosition;
    matrix presentCurrent;
    matrix state;
    matrix servoGoalPosition;
    matrix servoPresentPosition;
    matrix servoPresentCurrent;
    matrix startUpStartPosition;
    matrix shutDownStartPosition;

    std::string controlMode;
    ControllerMode controllerMode = ControllerMode::idle;
    InitializationStep initializationStep = InitializationStep::openChains;
    DetectRangeStep detectRangeStep = DetectRangeStep::disableTorque;
    size_t initializationChainIndex = 0;
    size_t initializationPositionLimitIndex = 0;
    int initializationServoID = 0;
    int initializationWaitTicks = 0;
    int startUpPoseStep = 0;
    int shutDownPoseStep = 0;
    int rampUpStep = 0;
    int rampDownStep = 0;
    bool shutDownComplete = false;
    bool hardwareInitialized = false;
    bool initializationPositionLimitsBuilt = false;

    std::vector<DynamixelServoChain> servoChains;
    std::vector<DynamixelServoPositionLimit> initializationPositionLimits;
    std::vector<bool> torqueWasEnabledAtInitialization;
    std::vector<bool> torqueStateKnownAtInitialization;

    std::string robotName;
    DynamixelServoConfiguration configuration;
    int configuredServoCount = 0;
    bool hardwareOpened = false;
    std::chrono::steady_clock::time_point initializationTickStart;

    static constexpr double INITIALIZATION_TIME_BUDGET = 0.010;
    static constexpr double INITIALIZATION_START_OPERATION_LIMIT = 0.007;

    bool
    ResolveModuleFile(const std::string & filename, std::filesystem::path & resolvedPath)
    {
        std::string path = __FILE__;
        path = path.substr(0, path.find_last_of("/\\"));
        return kernel().SanitizeReadPath(path + "/" + filename, resolvedPath);
    }

    bool
    LoadConfiguration()
    {
        if (!configuration.robots.empty())
            return true;

        if (configuration.Load([this](const std::string & filename, std::filesystem::path & resolvedPath)
        {
            return ResolveModuleFile(filename, resolvedPath);
        }))
            return true;

        Error(configuration.last_error());
        return false;
    }

    int
    ConfiguredServoCountForRobot(const std::string & configuredRobotName) const
    {
        const auto robotEntry = configuration.robots.find(configuredRobotName);
        if (robotEntry == configuration.robots.end())
            return 0;

        const auto typeEntry = configuration.robotTypeChains.find(robotEntry->second.type);
        if (typeEntry == configuration.robotTypeChains.end())
            return 0;

        int size = 0;
        for (const DynamixelChainConfiguration & chain : typeEntry->second)
            size = std::max(size, chain.ioIndex + chain.idMax - chain.idMin + 1);

        return size;
    }

    int
    RequestedServoCount() const
    {
        int requestedServoCount = 0;
        ParsePositiveInt(GetValue("ServoCount"), requestedServoCount);
        return requestedServoCount;
    }

    std::string
    ResolveParameterReference(const std::string & value) const
    {
        if (value.size() > 1 && value.front() == '@')
            return GetValue(value.substr(1));

        return value;
    }

    std::string
    RequestedRobotName() const
    {
        return ResolveParameterReference(GetValue("robot"));
    }

    int
    ExpectedGraphServoCount()
    {
        if (!KeyExists("nrServosTotal"))
            return 0;

        int expectedServoCount = 0;
        ParsePositiveInt(ComputeValueOf("nrServosTotal"), expectedServoCount);
        return expectedServoCount;
    }

    bool
    ValidateGraphServoCount()
    {
        const int expectedServoCount = ExpectedGraphServoCount();
        if (expectedServoCount <= 0 || expectedServoCount == configuredServoCount)
            return true;

        Notify(
            msg_fatal_error,
            "DynamixelController robot " + robotName + " configuration has " +
            std::to_string(configuredServoCount) + " servos, but the graph expects " +
            std::to_string(expectedServoCount) + " servos from nrServosTotal.");
        return false;
    }

    bool
    BoolMapValue(const std::map<int, bool> & values, int id) const
    {
        const auto value = values.find(id);
        return value != values.end() && value->second;
    }

    double
    DoubleMapValue(const std::map<int, double> & values, int id) const
    {
        const auto value = values.find(id);
        if (value == values.end())
            return 0.0;

        return value->second;
    }

    double
    ApplyTransform(double value, bool preInverted, double offset, bool postInverted) const
    {
        if (preInverted)
            value = -value;
        value += offset;
        if (postInverted)
            value = -value;
        return value;
    }

    double
    InvertTransform(double value, bool preInverted, double offset, bool postInverted) const
    {
        if (postInverted)
            value = -value;
        value -= offset;
        if (preInverted)
            value = -value;
        return value;
    }

    void
    CopyGoalPositionToServoSpace()
    {
        if (!goalPosition.connected())
            return;

        for (DynamixelServoChain & chain : servoChains)
            for (int id = chain.idMin; id <= chain.idMax; id++)
            {
                const int index = chain.ioIndex + chain.row(id);
                servoGoalPosition(index) = ApplyTransform(
                    goalPosition(index),
                    BoolMapValue(chain.goalPreInverted, id),
                    DoubleMapValue(chain.goalOffset, id),
                    BoolMapValue(chain.goalPostInverted, id));
            }
    }

    void
    PublishFeedbackInGraphSpace()
    {
        for (DynamixelServoChain & chain : servoChains)
            for (int id = chain.idMin; id <= chain.idMax; id++)
            {
                const int index = chain.ioIndex + chain.row(id);
                presentPosition(index) = ApplyTransform(
                    servoPresentPosition(index),
                    BoolMapValue(chain.presentPositionPreInverted, id),
                    DoubleMapValue(chain.presentPositionOffset, id),
                    BoolMapValue(chain.presentPositionPostInverted, id));
                presentCurrent(index) = ApplyTransform(
                    servoPresentCurrent(index),
                    BoolMapValue(chain.presentCurrentPreInverted, id),
                    DoubleMapValue(chain.presentCurrentOffset, id),
                    BoolMapValue(chain.presentCurrentPostInverted, id));
            }
    }

    const std::map<int, double> &
    ConfiguredPose(const DynamixelServoChain & chain, bool startup) const
    {
        return startup ? chain.startupPosition : chain.shutdownPosition;
    }

    std::map<int, double> &
    ConfiguredPose(DynamixelServoChain & chain, bool startup)
    {
        return startup ? chain.startupPosition : chain.shutdownPosition;
    }

    bool
    HasCompleteConfiguredPose(bool startup) const
    {
        bool hasPose = false;

        for (const DynamixelServoChain & chain : servoChains)
        {
            const std::map<int, double> & pose = ConfiguredPose(chain, startup);
            for (int id = chain.idMin; id <= chain.idMax; id++)
            {
                if (pose.find(id) == pose.end())
                    return false;
                hasPose = true;
            }
        }

        return hasPose;
    }

    double
    ConfiguredPoseValue(const DynamixelServoChain & chain, int id, bool startup) const
    {
        return ConfiguredPose(chain, startup).at(id);
    }

    bool
    SendPoseGoal(bool reportWarnings, matrix & poseTorqueEnable)
    {
        for (DynamixelServoChain & chain : servoChains)
            chain.PrepareFeedbackForCommunicationMode(servoGoalPosition, servoPresentPosition);

        ClipGoalPositionsToLimits();

        for (DynamixelServoChain & chain : servoChains)
            chain.ConvertGoalsForCommunicationMode(servoGoalPosition);

        bool success = true;
        matrix poseGoalPWM;
        for (DynamixelServoChain & chain : servoChains)
        {
            const dictionary * controlTable = reportWarnings ?
                ControlTableFor(chain) :
                chain.ControlTable(configuration.modelRegistry, configuration.controlTables);
            if (!controlTable)
            {
                success = false;
                continue;
            }

            if (!chain.WriteGoalForCommunicationMode(
                    servoGoalPosition,
                    poseGoalPWM,
                    poseTorqueEnable,
                    *controlTable))
            {
                if (reportWarnings)
                    Warning("Cannot move " + chain.name + " to configured pose");
                chain.ClearPort();
                success = false;
            }
        }

        PublishFeedbackInGraphSpace();
        return success;
    }

    bool
    WritePreparedServoGoals(bool reportWarnings, matrix & poseTorqueEnable)
    {
        ClipGoalPositionsToLimits();

        for (DynamixelServoChain & chain : servoChains)
            chain.ConvertGoalsForCommunicationMode(servoGoalPosition);

        bool success = true;
        matrix poseGoalPWM;
        for (DynamixelServoChain & chain : servoChains)
        {
            const dictionary * controlTable = reportWarnings ?
                ControlTableFor(chain) :
                chain.ControlTable(configuration.modelRegistry, configuration.controlTables);
            if (!controlTable)
            {
                success = false;
                continue;
            }

            if (!chain.WriteGoalForCommunicationMode(
                    servoGoalPosition,
                    poseGoalPWM,
                    poseTorqueEnable,
                    *controlTable))
            {
                if (reportWarnings)
                    Warning("Cannot write goal for " + chain.name);
                chain.ClearPort();
                success = false;
            }
        }

        PublishFeedbackInGraphSpace();
        return success;
    }

    void
    CopyInterpolatedConfiguredPoseToServoSpace(bool startup, double phase)
    {
        for (DynamixelServoChain & chain : servoChains)
            for (int id = chain.idMin; id <= chain.idMax; id++)
            {
                const int index = chain.ioIndex + chain.row(id);
                const double target = ConfiguredPoseValue(chain, id, startup);
                double start = target;
                if (startup && startUpStartPosition.size() == configuredServoCount)
                    start = startUpStartPosition(index);
                else if (!startup && shutDownStartPosition.size() == configuredServoCount)
                    start = shutDownStartPosition(index);
                else if (presentPosition.size() == configuredServoCount)
                    start = presentPosition(index);
                const double graphPosition = start + phase * (target - start);
                servoGoalPosition(index) = ApplyTransform(
                    graphPosition,
                    BoolMapValue(chain.goalPreInverted, id),
                    DoubleMapValue(chain.goalOffset, id),
                    BoolMapValue(chain.goalPostInverted, id));
            }
    }

    void
    BeginStartUpMode()
    {
        Notify(msg_print, "DynamixelController entering start_up mode.");
        startUpPoseStep = 0;
        startUpStartPosition.realloc(configuredServoCount);
        if (hardwareInitialized)
            ReadPresentPositionForConfiguredChains();
        for (DynamixelServoChain & chain : servoChains)
            for (int id = chain.idMin; id <= chain.idMax; id++)
            {
                const int index = chain.ioIndex + chain.row(id);
                startUpStartPosition(index) = 0;
                if (presentPosition.size() == configuredServoCount)
                    startUpStartPosition(index) = presentPosition(index);
            }

        EnterState(ControllerMode::startUp);
    }

    bool
    ReadPresentPositionForConfiguredChains()
    {
        bool success = true;
        for (DynamixelServoChain & chain : servoChains)
        {
            if (!chain.ready())
                continue;

            const dictionary * controlTable = ControlTableFor(chain);
            if (!controlTable)
            {
                success = false;
                continue;
            }

            if (!chain.ReadFeedbackForCommunicationMode(servoPresentPosition, servoPresentCurrent, *controlTable))
            {
                Warning("Cannot read startup position from " + chain.name);
                chain.ClearPort();
                success = false;
            }
        }

        PublishFeedbackInGraphSpace();
        return success;
    }

    void
    BeginShutDownMode()
    {
        Notify(msg_print, "DynamixelController entering shut_down mode.");
        shutDownPoseStep = 0;
        shutDownComplete = false;
        shutDownStartPosition.realloc(configuredServoCount);
        if (hardwareInitialized)
            ReadPresentPositionForConfiguredChains();
        for (int i = 0; i < configuredServoCount; i++)
        {
            shutDownStartPosition(i) = 0;
            if (presentPosition.size() == configuredServoCount)
                shutDownStartPosition(i) = presentPosition(i);
        }

        EnterState(ControllerMode::shutDown);
    }

    void
    BeginInitializationMode()
    {
        initializationStep = InitializationStep::openChains;
        detectRangeStep = DetectRangeStep::disableTorque;
        initializationChainIndex = 0;
        initializationPositionLimitIndex = 0;
        initializationServoID = 0;
        initializationWaitTicks = 0;
        initializationPositionLimitsBuilt = false;
        initializationPositionLimits.clear();
        torqueWasEnabledAtInitialization.assign(configuredServoCount, true);
        torqueStateKnownAtInitialization.assign(configuredServoCount, false);
        hardwareInitialized = false;
        EnterState(ControllerMode::initialization);
    }

    void
    BeginRampUpMode()
    {
        rampUpStep = 0;
        EnterState(ControllerMode::rampUp);
    }

    void
    BeginRampDownMode()
    {
        rampDownStep = 0;
        EnterState(ControllerMode::rampDown);
    }

    bool
    RunPowerOffRampAction(bool (DynamixelServoChain::*action)(const dictionary &))
    {
        bool success = true;
        for (DynamixelServoChain & chain : servoChains)
        {
            if (!chain.ready())
                continue;

            const dictionary * controlTable = ControlTableFor(chain);
            if (!controlTable)
            {
                success = false;
                continue;
            }

            if (!(chain.*action)(*controlTable))
                success = false;
        }

        return success;
    }

    bool
    RampPowerOffRobot(double phase)
    {
        bool success = true;
        for (DynamixelServoChain & chain : servoChains)
        {
            const dictionary * controlTable = ControlTableFor(chain);
            if (!controlTable)
            {
                success = false;
                continue;
            }

            if (!chain.RampPowerOffForCommunicationMode(*controlTable, phase))
                success = false;
        }

        return success;
    }

    bool
    PreparePowerOnRampRobot()
    {
        bool success = true;
        for (DynamixelServoChain & chain : servoChains)
        {
            const dictionary * controlTable = ControlTableFor(chain);
            if (!controlTable)
            {
                success = false;
                continue;
            }

            if (!chain.PreparePowerOnRampForCommunicationMode(*controlTable, torqueWasEnabledAtInitialization))
                success = false;
        }

        return success;
    }

    bool
    RampPowerOnRobot(double phase)
    {
        bool success = true;
        for (DynamixelServoChain & chain : servoChains)
        {
            const dictionary * controlTable = ControlTableFor(chain);
            if (!controlTable)
            {
                success = false;
                continue;
            }

            if (!chain.RampPowerOnForCommunicationMode(*controlTable, phase, torqueWasEnabledAtInitialization))
                success = false;
        }

        return success;
    }

    bool
    RestorePowerOnRampRobot()
    {
        bool success = true;
        for (DynamixelServoChain & chain : servoChains)
        {
            const dictionary * controlTable = ControlTableFor(chain);
            if (!controlTable)
            {
                success = false;
                continue;
            }

            if (!chain.RestorePowerOnRampForCommunicationMode(*controlTable, torqueWasEnabledAtInitialization))
                success = false;
        }

        return success;
    }

    int
    TickStepsForDuration(double duration) const
    {
        const double tickDuration = GetTickDuration();
        if (tickDuration <= 0)
            return 1;

        return std::max(1, static_cast<int>(std::ceil(duration / tickDuration)));
    }

    double
    InitializationElapsedTime() const
    {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - initializationTickStart).count();
    }

    bool
    CanStartInitializationOperation() const
    {
        return InitializationElapsedTime() < INITIALIZATION_START_OPERATION_LIMIT;
    }

    void
    DebugSlowInitializationOperation(const std::string & description, double startTime)
    {
        const double elapsed = InitializationElapsedTime() - startTime;
        if (elapsed > INITIALIZATION_TIME_BUDGET)
            Warning("DynamixelController initialization operation exceeded 10 ms: " + description +
                " took " + std::to_string(1000.0 * elapsed) + " ms.");
    }

    void
    WarnSlowInitializationTick()
    {
        const double elapsed = InitializationElapsedTime();
        if (elapsed > INITIALIZATION_TIME_BUDGET)
            Warning("DynamixelController initialization tick exceeded 10 ms in step " +
                std::to_string(static_cast<int>(initializationStep)) +
                " and took " + std::to_string(1000.0 * elapsed) + " ms.");
    }

    bool
    WriteInitialization1Byte(DynamixelServoChain & chain, const dictionary & controlTable, int id, const std::string & parameterName, uint8_t value)
    {
        int address = 0;
        if (!chain.ControlTableAddress(controlTable, parameterName, 1, address))
        {
            EnterState(ControllerMode::idle);
            return false;
        }

        uint8_t dxl_error = 0;
        const double startTime = InitializationElapsedTime();
        if (!chain.Write1Byte(id, address, value, dxl_error))
        {
            Warning("Failed to write " + parameterName + " for " + chain.name + " servo ID " + std::to_string(id) + ".");
            chain.ClearPort();
            EnterState(ControllerMode::idle);
            return false;
        }

        DebugSlowInitializationOperation("write " + parameterName + " for " + chain.name + " servo ID " + std::to_string(id), startTime);
        return true;
    }

    bool
    ReadInitialization1Byte(DynamixelServoChain & chain, const dictionary & controlTable, int id, const std::string & parameterName, uint8_t & value)
    {
        int address = 0;
        if (!chain.ControlTableAddress(controlTable, parameterName, 1, address))
            return false;

        uint8_t dxl_error = 0;
        const double startTime = InitializationElapsedTime();
        if (!chain.Read1Byte(id, address, value, dxl_error))
        {
            Warning("Could not read " + parameterName + " for " + chain.name + " servo ID " +
                std::to_string(id) + ". Assuming torque may already be enabled.");
            return false;
        }

        DebugSlowInitializationOperation("read " + parameterName + " for " + chain.name + " servo ID " + std::to_string(id), startTime);
        return true;
    }

    bool
    WriteInitialization2Byte(DynamixelServoChain & chain, const dictionary & controlTable, int id, const std::string & parameterName, uint16_t value)
    {
        int address = 0;
        if (!chain.ControlTableAddress(controlTable, parameterName, 2, address))
        {
            EnterState(ControllerMode::idle);
            return false;
        }

        uint8_t dxl_error = 0;
        const double startTime = InitializationElapsedTime();
        if (!chain.Write2Byte(id, address, value, dxl_error))
        {
            Warning("Failed to write " + parameterName + " for " + chain.name + " servo ID " + std::to_string(id) + ".");
            chain.ClearPort();
            EnterState(ControllerMode::idle);
            return false;
        }

        DebugSlowInitializationOperation("write " + parameterName + " for " + chain.name + " servo ID " + std::to_string(id), startTime);
        return true;
    }

    bool
    ReadInitialization2Byte(DynamixelServoChain & chain, const dictionary & controlTable, int id, const std::string & parameterName, uint16_t & value)
    {
        int address = 0;
        if (!chain.ControlTableAddress(controlTable, parameterName, 2, address))
        {
            EnterState(ControllerMode::idle);
            return false;
        }

        uint8_t dxl_error = 0;
        const double startTime = InitializationElapsedTime();
        if (!chain.Read2Byte(id, address, value, dxl_error))
        {
            Warning("Failed to read " + parameterName + " for " + chain.name + " servo ID " + std::to_string(id) + ".");
            chain.ClearPort();
            EnterState(ControllerMode::idle);
            return false;
        }

        DebugSlowInitializationOperation("read " + parameterName + " for " + chain.name + " servo ID " + std::to_string(id), startTime);
        return true;
    }

    void
    RecordUnknownTorqueState(DynamixelServoChain & chain, int id)
    {
        const int index = chain.ioIndex + chain.row(id);
        if (index >= 0 && index < configuredServoCount)
        {
            torqueWasEnabledAtInitialization[index] = true;
            torqueStateKnownAtInitialization[index] = false;
        }
    }

    bool
    ReadInitializationTorqueState(DynamixelServoChain & chain)
    {
        if (initializationServoID < chain.idMin || initializationServoID > chain.idMax)
            initializationServoID = chain.idMin;

        if (!CanStartInitializationOperation())
            return false;

        const dictionary * controlTable = ControlTableFor(chain);
        if (!controlTable)
        {
            RecordUnknownTorqueState(chain, initializationServoID);
            initializationServoID++;
            if (initializationServoID > chain.idMax)
            {
                initializationChainIndex++;
                if (initializationChainIndex < servoChains.size())
                    initializationServoID = servoChains[initializationChainIndex].idMin;
            }
            return true;
        }

        uint8_t torqueEnabled = 1;
        const int index = chain.ioIndex + chain.row(initializationServoID);
        if (index >= 0 && index < configuredServoCount &&
            ReadInitialization1Byte(chain, *controlTable, initializationServoID, "Torque Enable", torqueEnabled))
        {
            torqueWasEnabledAtInitialization[index] = torqueEnabled != 0;
            torqueStateKnownAtInitialization[index] = true;
        }
        else
            RecordUnknownTorqueState(chain, initializationServoID);

        initializationServoID++;
        if (initializationServoID > chain.idMax)
        {
            initializationChainIndex++;
            if (initializationChainIndex < servoChains.size())
                initializationServoID = servoChains[initializationChainIndex].idMin;
        }

        return true;
    }

    bool
    TorqueWasEnabledAtInitialization(const DynamixelServoChain & chain, int id) const
    {
        const int index = chain.ioIndex + chain.row(id);
        return index >= 0 &&
            index < static_cast<int>(torqueWasEnabledAtInitialization.size()) &&
            torqueWasEnabledAtInitialization[index];
    }

    void
    BeginDetectRangeStep(DynamixelServoChain & chain, DetectRangeStep step)
    {
        detectRangeStep = step;
        initializationServoID = chain.idMin;
    }

    bool
    AdvanceServoID(DynamixelServoChain & chain)
    {
        initializationServoID++;
        if (initializationServoID <= chain.idMax)
            return false;

        initializationServoID = chain.idMin;
        return true;
    }

    bool
    AdvanceDetectRangeBegin(DynamixelServoChain & chain, const dictionary & controlTable)
    {
        if (initializationServoID < chain.idMin || initializationServoID > chain.idMax)
            initializationServoID = chain.idMin;

        if (!CanStartInitializationOperation())
            return false;

        switch (detectRangeStep)
        {
            case DetectRangeStep::disableTorque:
                if (!WriteInitialization1Byte(chain, controlTable, initializationServoID, "Torque Enable", 0))
                    return false;
                if (AdvanceServoID(chain))
                    BeginDetectRangeStep(chain, DetectRangeStep::writeCWAngleLimit);
                return false;

            case DetectRangeStep::writeCWAngleLimit:
                if (!WriteInitialization2Byte(chain, controlTable, initializationServoID, "CW Angle Limit", 0))
                    return false;
                if (AdvanceServoID(chain))
                    BeginDetectRangeStep(chain, DetectRangeStep::writeCCWAngleLimit);
                return false;

            case DetectRangeStep::writeCCWAngleLimit:
                if (!WriteInitialization2Byte(chain, controlTable, initializationServoID, "CCW Angle Limit", chain.detectRangeFullRangePosition))
                    return false;
                if (AdvanceServoID(chain))
                    BeginDetectRangeStep(chain, DetectRangeStep::writeDetectRangeTorqueLimit);
                return false;

            case DetectRangeStep::writeDetectRangeTorqueLimit:
                if (!WriteInitialization2Byte(chain, controlTable, initializationServoID, "Torque Limit", chain.detectRangeTorqueLimit))
                    return false;
                if (AdvanceServoID(chain))
                    BeginDetectRangeStep(chain, DetectRangeStep::enableTorque);
                return false;

            case DetectRangeStep::enableTorque:
                if (!WriteInitialization1Byte(chain, controlTable, initializationServoID, "Torque Enable", 1))
                    return false;
                if (AdvanceServoID(chain))
                    BeginDetectRangeStep(chain, DetectRangeStep::writeDetectRangeGoalPosition);
                return false;

            case DetectRangeStep::writeDetectRangeGoalPosition:
                if (!WriteInitialization2Byte(chain, controlTable, initializationServoID, "Goal Position", chain.detectRangeGoalPosition))
                    return false;
                if (AdvanceServoID(chain))
                {
                    initializationWaitTicks = TickStepsForDuration(chain.detectRangeMoveDelay);
                    BeginDetectRangeStep(chain, DetectRangeStep::readPresentPosition);
                    initializationStep = InitializationStep::detectRangeWait;
                    return true;
                }
                return false;

            default:
                return false;
        }
    }

    bool
    AdvanceDetectRangeFinish(DynamixelServoChain & chain, const dictionary & controlTable)
    {
        if (initializationServoID < chain.idMin || initializationServoID > chain.idMax)
            initializationServoID = chain.idMin;

        if (!CanStartInitializationOperation())
            return false;

        switch (detectRangeStep)
        {
            case DetectRangeStep::readPresentPosition:
            {
                uint16_t presentPosition = 0;
                if (!ReadInitialization2Byte(chain, controlTable, initializationServoID, "Present Position", presentPosition))
                    return false;

                chain.positionMin[initializationServoID] = presentPosition + chain.detectRangePositionOffset;
                chain.positionMax[initializationServoID] = chain.positionMin[initializationServoID] + chain.detectRangePositionRange;
                if (AdvanceServoID(chain))
                    BeginDetectRangeStep(chain, DetectRangeStep::disableTorqueAfterRead);
                return false;
            }

            case DetectRangeStep::disableTorqueAfterRead:
            {
                const uint8_t restoreTorque = TorqueWasEnabledAtInitialization(chain, initializationServoID) ? 1 : 0;
                if (!WriteInitialization1Byte(chain, controlTable, initializationServoID, "Torque Enable", restoreTorque))
                    return false;
                if (AdvanceServoID(chain))
                    BeginDetectRangeStep(chain, DetectRangeStep::restoreTorqueLimit);
                return false;
            }

            case DetectRangeStep::restoreTorqueLimit:
                if (!WriteInitialization2Byte(chain, controlTable, initializationServoID, "Torque Limit", chain.detectRangeMaxTorqueLimit))
                    return false;
                if (AdvanceServoID(chain))
                {
                    detectRangeStep = DetectRangeStep::done;
                    Debug("Position limits chain " + chain.name + " (detect_range)");
                    return true;
                }
                return false;

            default:
                return false;
        }
    }

    void
    TickInitialization()
    {
        initializationTickStart = std::chrono::steady_clock::now();

        if (simulate)
        {
            hardwareInitialized = true;
            EnterState(ControllerMode::operation);
            return;
        }

        switch (initializationStep)
        {
            case InitializationStep::openChains:
                if (initializationChainIndex >= servoChains.size())
                {
                    hardwareOpened = true;
                    initializationChainIndex = 0;
                    if (!servoChains.empty())
                        initializationServoID = servoChains[0].idMin;
                    initializationStep = InitializationStep::readTorqueState;
                    break;
                }
                if (!CanStartInitializationOperation())
                    break;
                {
                    DynamixelServoChain & chain = servoChains[initializationChainIndex];
                    const double startTime = InitializationElapsedTime();
                    if (!chain.Open())
                    {
                        EnterState(ControllerMode::idle);
                        return;
                    }
                    DebugSlowInitializationOperation("open chain " + chain.name, startTime);
                    initializationChainIndex++;
                }
                break;

            case InitializationStep::readTorqueState:
                if (initializationChainIndex >= servoChains.size())
                {
                    initializationChainIndex = 0;
                    initializationStep = InitializationStep::createSyncObjects;
                    break;
                }
                if (!ReadInitializationTorqueState(servoChains[initializationChainIndex]))
                    break;
                break;

            case InitializationStep::createSyncObjects:
                if (initializationChainIndex >= servoChains.size())
                {
                    initializationChainIndex = 0;
                    initializationStep = InitializationStep::loadParameters;
                    break;
                }
                if (!CanStartInitializationOperation())
                    break;
                {
                    DynamixelServoChain & chain = servoChains[initializationChainIndex];
                    initializationChainIndex++;
                    if (!chain.UsesSyncIndirectCommunication())
                        break;

                    const dictionary * indirectLayout = chain.IndirectLayout(configuration.modelRegistry, configuration.indirectLayouts);
                    if (!indirectLayout)
                    {
                        Notify(msg_fatal_error, "No indirect layout resolved for servo chain " + chain.name + ".");
                        EnterState(ControllerMode::idle);
                        return;
                    }

                    const double startTime = InitializationElapsedTime();
                    if (!chain.CreateSyncObjects(*indirectLayout))
                    {
                        EnterState(ControllerMode::idle);
                        return;
                    }
                    DebugSlowInitializationOperation("create sync objects for " + chain.name, startTime);
                }
                break;

            case InitializationStep::loadParameters:
            {
                DynamixelServoParameterFile parameterFile = ParameterFile();
                if (parameterFile.Exists())
                {
                    Debug("Reading JSON parameter file");
                    if (!LoadParameterMatrices(parameterFile))
                    {
                        Notify(msg_fatal_error, "Failed to load servo parameter matrices.");
                        EnterState(ControllerMode::idle);
                        return;
                    }
                    initializationStep = InitializationStep::applySettings;
                }
                else
                    initializationStep = InitializationStep::applyDefaultSettings;
                break;
            }

            case InitializationStep::applySettings:
                Notify(msg_trace, "Setting servo settings");
                if (initializationChainIndex >= servoChains.size())
                {
                    initializationChainIndex = 0;
                    initializationPositionLimitIndex = 0;
                    initializationPositionLimitsBuilt = false;
                    initializationPositionLimits.clear();
                    initializationStep = InitializationStep::applyPositionLimits;
                    break;
                }
                if (!CanStartInitializationOperation())
                    break;
                {
                    uint8_t dxl_error = 0;
                    DynamixelServoChain & chain = servoChains[initializationChainIndex];
                    const double startTime = InitializationElapsedTime();
                    if (!chain.ApplyLoadedControlTableSettingsForCommunicationMode(
                        configuration.modelRegistry,
                        configuration.controlTables,
                        configuration.indirectLayouts,
                        dxl_error))
                    {
                        Notify(msg_fatal_error, "Failed to set servo settings.");
                        EnterState(ControllerMode::idle);
                        return;
                    }
                    DebugSlowInitializationOperation("apply settings for " + chain.name, startTime);
                    initializationChainIndex++;
                }
                break;

            case InitializationStep::applyPositionLimits:
                Notify(msg_trace, "Setting min max limits");
                if (!initializationPositionLimitsBuilt)
                {
                    if (!BuildEffectivePositionLimits(initializationPositionLimits))
                    {
                        Notify(msg_fatal_error, "Failed to build min/max hardware limits for servos.");
                        EnterState(ControllerMode::idle);
                        return;
                    }
                    initializationPositionLimitsBuilt = true;
                    initializationPositionLimitIndex = 0;
                }
                if (initializationPositionLimitIndex >= initializationPositionLimits.size())
                {
                    initializationChainIndex = 0;
                    initializationStep = InitializationStep::detectRangeBegin;
                    break;
                }
                if (!CanStartInitializationOperation())
                    break;
                {
                    uint8_t dxl_error = 0;
                    const DynamixelServoPositionLimit & limit = initializationPositionLimits[initializationPositionLimitIndex];
                    DynamixelServoChain * chain = FindDynamixelServoChain(limit.chainName);
                    if (!chain)
                    {
                        Error("Servo chain is not ready: " + limit.chainName);
                        EnterState(ControllerMode::idle);
                        return;
                    }

                    const dictionary * controlTable = ControlTableFor(*chain);
                    if (!controlTable)
                    {
                        EnterState(ControllerMode::idle);
                        return;
                    }

                    const double startTime = InitializationElapsedTime();
                    if (!chain->WritePositionLimit(*controlTable, limit.id, limit.minPosition, limit.maxPosition, dxl_error))
                    {
                        Notify(msg_fatal_error, "Failed to set min/max hardware limits on servos.");
                        EnterState(ControllerMode::idle);
                        return;
                    }
                    DebugSlowInitializationOperation("write position limits for " + limit.chainName +
                        " servo ID " + std::to_string(limit.id), startTime);
                    initializationPositionLimitIndex++;
                }
                break;

            case InitializationStep::applyDefaultSettings:
                Notify(msg_warning, "No parameter file found for this robot type. Using default settings.");
                if (!ApplyDefaultServoSettings())
                {
                    Notify(msg_fatal_error, "Unable to write default settings on servos\n");
                    EnterState(ControllerMode::idle);
                    return;
                }
                initializationChainIndex = 0;
                initializationStep = InitializationStep::detectRangeBegin;
                break;

            case InitializationStep::detectRangeBegin:
                if (initializationChainIndex >= servoChains.size())
                {
                    initializationStep = InitializationStep::complete;
                    break;
                }
                if (!BeginDetectRangeChain(servoChains[initializationChainIndex]))
                    return;
                break;

            case InitializationStep::detectRangeWait:
                if (initializationWaitTicks > 0)
                {
                    initializationWaitTicks--;
                    break;
                }
                initializationStep = InitializationStep::detectRangeFinish;
                break;

            case InitializationStep::detectRangeFinish:
                if (!FinishDetectRangeChain(servoChains[initializationChainIndex]))
                    return;
                initializationChainIndex++;
                detectRangeStep = DetectRangeStep::disableTorque;
                initializationStep = InitializationStep::detectRangeBegin;
                break;

            case InitializationStep::detectRange:
                initializationStep = InitializationStep::detectRangeBegin;
                break;

            case InitializationStep::complete:
                hardwareInitialized = true;
                Debug("Servo initialization complete\n");
                {
                    const double startTime = InitializationElapsedTime();
                    ReadPresentPositionForConfiguredChains();
                    DebugSlowInitializationOperation("read final startup feedback", startTime);
                }
                EnterState(ControllerMode::idle);
                break;
        }

        WarnSlowInitializationTick();
    }

    bool
    BeginDetectRangeChain(DynamixelServoChain & chain)
    {
        const dictionary * controlTable = ControlTableFor(chain);
        if (!controlTable)
        {
            EnterState(ControllerMode::idle);
            return false;
        }

        if (!chain.detectRange)
        {
            initializationChainIndex++;
            initializationStep = InitializationStep::detectRangeBegin;
            return true;
        }

        if (detectRangeStep != DetectRangeStep::disableTorque &&
            detectRangeStep != DetectRangeStep::writeCWAngleLimit &&
            detectRangeStep != DetectRangeStep::writeCCWAngleLimit &&
            detectRangeStep != DetectRangeStep::writeDetectRangeTorqueLimit &&
            detectRangeStep != DetectRangeStep::enableTorque &&
            detectRangeStep != DetectRangeStep::writeDetectRangeGoalPosition)
            BeginDetectRangeStep(chain, DetectRangeStep::disableTorque);

        if (!AdvanceDetectRangeBegin(chain, *controlTable))
            return false;

        if (initializationStep != InitializationStep::detectRangeWait)
            return false;

        return true;
    }

    bool
    FinishDetectRangeChain(DynamixelServoChain & chain)
    {
        const dictionary * controlTable = ControlTableFor(chain);
        if (!controlTable)
        {
            EnterState(ControllerMode::idle);
            return false;
        }

        if (!chain.detectRange)
            return true;

        return AdvanceDetectRangeFinish(chain, *controlTable);
    }

    void
    TickRampUp()
    {
        if (!hardwareInitialized)
        {
            Warning("Cannot ramp up torque before DynamixelController initialization.");
            EnterState(ControllerMode::idle);
            return;
        }

        if (rampUpStep == 0)
        {
            ReadPresentPositionForConfiguredChains();

            matrix poseTorqueEnable(configuredServoCount);
            for (int i = 0; i < configuredServoCount; i++)
                poseTorqueEnable(i) = i < static_cast<int>(torqueWasEnabledAtInitialization.size()) &&
                    torqueWasEnabledAtInitialization[i] ? 1 : 0;

            for (DynamixelServoChain & chain : servoChains)
                for (int id = chain.idMin; id <= chain.idMax; id++)
                {
                    const int index = chain.ioIndex + chain.row(id);
                    servoGoalPosition(index) = servoPresentPosition(index);
            }

            WritePreparedServoGoals(true, poseTorqueEnable);
            PreparePowerOnRampRobot();
        }

        rampUpStep++;
        const int steps = TickStepsForDuration(DynamixelServoChainConstants::TIMER_POWER_ON);
        RampPowerOnRobot(static_cast<double>(rampUpStep) / steps);
        if (rampUpStep >= steps)
        {
            RestorePowerOnRampRobot();
            EnterState(ControllerMode::idle);
        }
    }

    void
    TickStartUp()
    {
        if (!hardwareInitialized)
        {
            Warning("Cannot start up before DynamixelController initialization.");
            EnterState(ControllerMode::idle);
            return;
        }

        if (!HasCompleteConfiguredPose(true))
        {
            Warning("No complete startup pose configured; leaving start-up state.");
            EnterState(ControllerMode::idle);
            return;
        }

        const int steps = TickStepsForDuration(DynamixelServoChainConstants::STARTUP_POSE_DURATION);
        const double phase = static_cast<double>(startUpPoseStep) / steps;
        CopyInterpolatedConfiguredPoseToServoSpace(true, phase);
        startUpPoseStep++;

        matrix poseTorqueEnable(configuredServoCount);
        for (int i = 0; i < configuredServoCount; i++)
            poseTorqueEnable(i) = 1;

        SendPoseGoal(true, poseTorqueEnable);
        if (startUpPoseStep > steps)
        {
            shutDownComplete = false;
            EnterState(ControllerMode::idle);
            Notify(msg_print, "DynamixelController start-up complete.");
        }
    }

    void
    TickShutDown()
    {
        if (!hardwareInitialized)
        {
            Warning("Cannot shut down before DynamixelController initialization.");
            EnterState(ControllerMode::idle);
            return;
        }

        if (!HasCompleteConfiguredPose(false))
        {
            Warning("No complete shutdown pose configured; skipping shutdown pose move.");
            BeginRampDownMode();
            return;
        }

        const int steps = TickStepsForDuration(DynamixelServoChainConstants::SHUTDOWN_POSE_DURATION);
        shutDownPoseStep++;
        CopyInterpolatedConfiguredPoseToServoSpace(false, static_cast<double>(shutDownPoseStep) / steps);

        matrix poseTorqueEnable(configuredServoCount);
        for (int i = 0; i < configuredServoCount; i++)
            poseTorqueEnable(i) = 1;

        SendPoseGoal(true, poseTorqueEnable);
        if (shutDownPoseStep >= steps)
            BeginRampDownMode();
    }

    void
    TickRampDown()
    {
        if (!hardwareInitialized)
        {
            EnterState(ControllerMode::idle);
            return;
        }

        const int steps = TickStepsForDuration(DynamixelServoChainConstants::TIMER_POWER_OFF);
        if (rampDownStep == 0)
        {
            Notify(msg_print, "Powering off servos. Support the robot if needed.");
            RunPowerOffRampAction(&DynamixelServoChain::PreparePowerOffRampForCommunicationMode);
        }

        rampDownStep++;
        RampPowerOffRobot(static_cast<double>(rampDownStep) / steps);
        if (rampDownStep >= steps)
        {
            RunPowerOffRampAction(&DynamixelServoChain::DisableTorqueForCommunicationMode);
            RunPowerOffRampAction(&DynamixelServoChain::RestorePowerOffRampForCommunicationMode);
            shutDownComplete = true;
            EnterState(ControllerMode::idle);
            Notify(msg_print, "DynamixelController shut-down complete.");
        }
    }

    void
    TickTorqueOff()
    {
        bool success = true;
        for (DynamixelServoChain & chain : servoChains)
        {
            const dictionary * controlTable = ControlTableFor(chain);
            if (!controlTable)
            {
                success = false;
                continue;
            }

            if (!chain.DisableTorqueForCommunicationMode(*controlTable))
            {
                Warning("Cannot disable torque for " + chain.name);
                chain.ClearPort();
                success = false;
            }
        }

        if (success)
            Notify(msg_warning, "DynamixelController emergency torque off complete. Support the robot if needed.");
        EnterState(ControllerMode::idle);
    }

    void
    TickOperation()
    {
        if (!hardwareInitialized && !simulate)
        {
            Warning("Cannot operate before DynamixelController initialization.");
            EnterState(ControllerMode::idle);
            return;
        }

        CopyGoalPositionToServoSpace();

        for (DynamixelServoChain & chain : servoChains)
            chain.PrepareFeedbackForCommunicationMode(servoGoalPosition, servoPresentPosition);

        if (!HasReceivedGoalPosition())
        {
            PublishFeedbackInGraphSpace();
            return;
        }

        ClipGoalPositionsToLimits();

        if (simulate)
        {
            SimulateFeedback();
            PublishFeedbackInGraphSpace();
            return;
        }

        for (DynamixelServoChain & chain : servoChains)
            chain.ConvertGoalsForCommunicationMode(servoGoalPosition);

        for (DynamixelServoChain & chain : servoChains)
        {
            const dictionary * controlTable = chain.ControlTable(configuration.modelRegistry, configuration.controlTables);
            if (!controlTable)
            {
                Warning("No control table resolved for servo chain " + chain.name + ".");
                continue;
            }

            if (!chain.CommunicateForCommunicationMode(
                    servoGoalPosition,
                    goalCurrent,
                    goalPWM,
                    torqueEnable,
                    servoPresentPosition,
                    servoPresentCurrent,
                    controlMode,
                    *controlTable))
            {
                Warning("Cannot communicate with " + chain.name);
                chain.ClearPort();
            }
        }

        PublishFeedbackInGraphSpace();
    }

    void
    TickControllerMode()
    {
        switch (controllerMode)
        {
            case ControllerMode::idle:
                break;

            case ControllerMode::initialization:
                TickInitialization();
                break;

            case ControllerMode::rampUp:
                TickRampUp();
                break;

            case ControllerMode::startUp:
                TickStartUp();
                break;

            case ControllerMode::shutDown:
                TickShutDown();
                break;

            case ControllerMode::rampDown:
                TickRampDown();
                break;

            case ControllerMode::operation:
                TickOperation();
                break;

            case ControllerMode::torqueOff:
                TickTorqueOff();
                break;
        }
    }

    bool
    ControlTableSupportsCurrent(const std::string & controlTableName) const
    {
        if (controlTableName.empty() ||
            !configuration.controlTables.contains(controlTableName) ||
            !configuration.controlTables[controlTableName].is_dictionary())
            return false;

        const dictionary & controlTable = configuration.controlTables[controlTableName].as_dictionary();
        return HasControlTableEntry(controlTable, "Goal Current", 2) && HasControlTableEntry(controlTable, "Present Current", 2);
    }

    void
    DebugConfigurationSummary(const DynamixelRobotConfiguration & robotConfiguration)
    {
        Debug("DynamixelController configuration: robot=" + robotName +
            ", type=" + robotConfiguration.type +
            ", servo_count=" + std::to_string(configuredServoCount));

        for (const DynamixelServoChain & chain : servoChains)
        {
            Debug("Chain " + chain.name +
                ": communication=" + chain.communication +
                ", serial_port=" + chain.serialPort +
                ", baud_rate=" + std::to_string(chain.baudRate) +
                ", ids=" + std::to_string(chain.idMin) + ".." + std::to_string(chain.idMax) +
                ", io_index=" + std::to_string(chain.ioIndex));

            const DynamixelChainConfiguration * chainConfiguration = DynamixelChainConfigurationFor(chain);
            for (int id = chain.idMin; id <= chain.idMax; id++)
            {
                const std::string modelName = chain.ModelName(id);
                const std::string controlTableName = chain.ControlTableName(id, configuration.modelRegistry);
                const std::string indirectLayoutName = chain.IndirectLayoutName(id, configuration.modelRegistry);
                std::string label;
                std::string unit;

                if (chainConfiguration)
                {
                    const auto labelEntry = chainConfiguration->labels.find(id);
                    if (labelEntry != chainConfiguration->labels.end())
                        label = labelEntry->second;

                    const auto unitEntry = chainConfiguration->units.find(id);
                    if (unitEntry != chainConfiguration->units.end())
                        unit = unitEntry->second;
                }

                Debug("  ID " + std::to_string(id) +
                    ": model=" + modelName +
                    ", io_index=" + std::to_string(chain.ioIndex + chain.row(id)) +
                    ", label=" + label +
                    ", unit=" + unit +
                    ", control_table=" + controlTableName +
                    ", indirect_layout=" + indirectLayoutName +
                    ", current=" + std::string(ControlTableSupportsCurrent(controlTableName) ? "supported" : "unsupported"));
            }
        }
    }

    void
    SetOutputLabels(matrix & output)
    {
        output.clear_labels(0);

        std::vector<std::string> labels(configuredServoCount);
        const std::vector<DynamixelChainConfiguration> * chainConfigurations = CurrentDynamixelChainConfigurations();
        if (!chainConfigurations)
            return;

        for (const DynamixelChainConfiguration & chainConfiguration : *chainConfigurations)
            for (int id = chainConfiguration.idMin; id <= chainConfiguration.idMax; id++)
            {
                const int labelIndex = chainConfiguration.ioIndex + id - chainConfiguration.idMin;
                const auto label = chainConfiguration.labels.find(id);
                if (labelIndex >= 0 && labelIndex < configuredServoCount && label != chainConfiguration.labels.end())
                    labels[labelIndex] = label->second;
            }

        for (const std::string & label : labels)
            output.push_label(0, label);
    }

    void
    SetStateOutputLabels()
    {
        state.clear_labels(0);
        state.push_label(0, "idle");
        state.push_label(0, "initialization");
        state.push_label(0, "ramp-up");
        state.push_label(0, "start-up");
        state.push_label(0, "shut-down");
        state.push_label(0, "ramp-down");
        state.push_label(0, "operation");
        state.push_label(0, "torque-off");
    }

    int
    StateIndex(ControllerMode mode) const
    {
        return static_cast<int>(mode);
    }

    void
    PublishControllerState()
    {
        for (int i = 0; i < state.size(); i++)
            state[i] = 0;

        const int index = StateIndex(controllerMode);
        if (index >= 0 && index < state.size())
            state[index] = 1;
    }

    void
    EnterState(ControllerMode mode)
    {
        controllerMode = mode;
        PublishControllerState();
    }

    int
    ConfiguredServoCount() const
    {
        int size = 0;
        for (const DynamixelServoChain & chain : servoChains)
            size = std::max(size, chain.ioIndex + chain.size());
        return size;
    }

    int
    ParameterServoCount() const
    {
        int size = 0;
        for (const DynamixelServoChain & chain : servoChains)
            if (chain.IsParameterChain())
                size += chain.size();

        return size;
    }

    DynamixelServoParameterFile
    ParameterFile()
    {
        return DynamixelServoParameterFile(
            CurrentDynamixelRobotConfiguration()->type,
            controlMode,
            [this](const std::string & filename, std::filesystem::path & resolvedPath) { return ResolveModuleFile(filename, resolvedPath); });
    }

    const DynamixelRobotConfiguration *
    CurrentDynamixelRobotConfiguration() const
    {
        const auto robotEntry = configuration.robots.find(robotName);
        if (robotEntry == configuration.robots.end())
            return nullptr;

        return &robotEntry->second;
    }

    const std::vector<DynamixelChainConfiguration> *
    CurrentDynamixelChainConfigurations() const
    {
        const DynamixelRobotConfiguration * robotConfiguration = CurrentDynamixelRobotConfiguration();
        if (!robotConfiguration)
            return nullptr;

        const auto typeEntry = configuration.robotTypeChains.find(robotConfiguration->type);
        if (typeEntry == configuration.robotTypeChains.end())
            return nullptr;

        return &typeEntry->second;
    }

    const DynamixelChainConfiguration *
    DynamixelChainConfigurationFor(const DynamixelServoChain & chain) const
    {
        const std::vector<DynamixelChainConfiguration> * chainConfigurations = CurrentDynamixelChainConfigurations();
        if (!chainConfigurations)
            return nullptr;

        for (const DynamixelChainConfiguration & chainConfiguration : *chainConfigurations)
            if (chainConfiguration.name == chain.name)
                return &chainConfiguration;

        return nullptr;
    }

    DynamixelServoChain *
    FindDynamixelServoChain(const std::string & name)
    {
        for (auto & chain : servoChains)
            if (chain.name == name)
                return &chain;
        return nullptr;
    }

    const DynamixelServoPositionLimit *
    DefaultPositionLimitFor(const DynamixelChainConfiguration & chainConfiguration, int id) const
    {
        for (const DynamixelServoPositionLimit & limit : chainConfiguration.defaultPositionLimits)
            if (limit.id == id)
                return &limit;

        return nullptr;
    }

    const dictionary *
    ControlTableFor(const DynamixelServoChain & chain)
    {
        const dictionary * controlTable = chain.ControlTable(configuration.modelRegistry, configuration.controlTables);
        if (!controlTable)
            Error("No control table resolved for servo chain " + chain.name + ".");

        return controlTable;
    }

    bool
    ValidateChainCommunication()
    {
        for (const DynamixelServoChain & chain : servoChains)
        {
            if (!chain.UsesSyncIndirectCommunication() && !chain.UsesDirectPositionCommunication())
            {
                Error("Servo chain " + chain.name + " has unknown communication mode " + chain.communication + ".");
                return false;
            }

            for (int id = chain.idMin; id <= chain.idMax; id++)
                if (!chain.ControlTable(id, configuration.modelRegistry, configuration.controlTables))
                {
                    Error("Servo chain " + chain.name + " has no resolved control table for servo ID " + std::to_string(id) + ".");
                    return false;
                }
        }

        for (const DynamixelServoChain & chain : servoChains)
        {
            if (!chain.UsesSyncIndirectCommunication())
                continue;

            if (chain.UsesSingleIndirectLayout(configuration.modelRegistry))
                continue;

            Error("Servo chain " + chain.name + " uses multiple or unknown indirect layouts. The current sync-write path requires one indirect layout per chain.");
            return false;
        }

        return true;
    }

    bool
    ApplyProfileAndPID(uint32_t profileAcceleration, uint32_t profileVelocity, uint8_t & dxl_error)
    {
        for (DynamixelServoChain & chain : servoChains)
        {
            const DynamixelChainConfiguration * chainConfiguration = DynamixelChainConfigurationFor(chain);

            if (!chainConfiguration)
            {
                Error("No configuration found for servo chain: " + chain.name);
                return false;
            }

            const dictionary * controlTable = ControlTableFor(chain);
            if (!controlTable)
                return false;

            if (!chain.ApplyProfileAndPIDForCommunicationMode(
                *controlTable,
                profileAcceleration,
                profileVelocity,
                chainConfiguration->defaults.pGain,
                chainConfiguration->defaults.iGain,
                chainConfiguration->defaults.dGain,
                dxl_error))
                return false;
        }

        return true;
    }

    bool
    BuildConfigurationPositionLimitVectors(std::vector<double> & minLimits, std::vector<double> & maxLimits)
    {
        const int limitCount = ParameterServoCount();
        minLimits.assign(limitCount, 0);
        maxLimits.assign(limitCount, 0);

        for (DynamixelServoChain * chainPtr : ParameterChains())
        {
            DynamixelServoChain & chain = *chainPtr;
            const DynamixelChainConfiguration * chainConfiguration = DynamixelChainConfigurationFor(chain);
            if (!chainConfiguration)
            {
                Error("No configuration found for servo chain: " + chain.name);
                return false;
            }

            std::vector<int> limitIndexes = ParameterIndexesFor(chain);
            if (limitIndexes.size() != static_cast<size_t>(chain.size()))
            {
                Error("Position limit size does not match servo chain: " + chain.name);
                return false;
            }

            for (int id = chain.idMin; id <= chain.idMax; id++)
            {
                const DynamixelServoPositionLimit * limit = DefaultPositionLimitFor(*chainConfiguration, id);
                if (!limit)
                {
                    Error("No configured position limits found for servo chain " + chain.name + " ID " + std::to_string(id) + ".");
                    return false;
                }

                const int index = limitIndexes[chain.row(id)];
                minLimits[index] = limit->minPosition * chain.PositionUnitDegrees(id) * chain.ConversionFactor(id);
                maxLimits[index] = limit->maxPosition * chain.PositionUnitDegrees(id) * chain.ConversionFactor(id);
            }
        }

        return true;
    }

    bool
    BuildEffectivePositionLimitVectors(std::vector<double> & minLimits, std::vector<double> & maxLimits)
    {
        if (!BuildConfigurationPositionLimitVectors(minLimits, maxLimits))
            return false;

        if (!usePositionLimitParameters)
            return true;

        const int limitCount = ParameterServoCount();
        if (minLimitPosition.size() < limitCount || maxLimitPosition.size() < limitCount)
        {
            Error("Position limit parameter matrices are smaller than the configured parameter-chain servo count.");
            return false;
        }

        for (int i = 0; i < limitCount; i++)
        {
            minLimits[i] = minLimitPosition[i];
            maxLimits[i] = maxLimitPosition[i];
        }

        return true;
    }

    bool
    ApplyPositionLimits(const std::vector<DynamixelServoPositionLimit> & limits, uint8_t & dxl_error)
    {
        for (const DynamixelServoPositionLimit & limit : limits)
        {
            DynamixelServoChain * chain = FindDynamixelServoChain(limit.chainName);
            if (!chain)
            {
                Error("Servo chain is not ready: " + limit.chainName);
                return false;
            }

            const dictionary * controlTable = ControlTableFor(*chain);
            if (!controlTable)
                return false;

            if (!chain->WritePositionLimit(*controlTable, limit.id, limit.minPosition, limit.maxPosition, dxl_error))
                return false;
        }

        return true;
    }

    bool
    BuildPositionLimitsForChain(
        DynamixelServoChain & chain,
        const std::vector<double> & minLimits,
        const std::vector<double> & maxLimits,
        std::vector<DynamixelServoPositionLimit> & limits)
    {
        std::vector<int> limitIndexes = ParameterIndexesFor(chain);
        if (limitIndexes.size() != static_cast<size_t>(chain.size()))
        {
            Error("Position limit size does not match servo chain: " + chain.name);
            return false;
        }

        for (int id = chain.idMin; id <= chain.idMax; id++)
        {
            int row = chain.row(id);
            double minDegrees = minLimits[limitIndexes[row]];
            double maxDegrees = maxLimits[limitIndexes[row]];

            if (maxDegrees <= minDegrees)
            {
                Error("Invalid position limits for servo chain " + chain.name + " ID " + std::to_string(id) + ".");
                return false;
            }

            DynamixelServoPositionLimit limit;
            limit.chainName = chain.name;
            limit.id = id;
            limit.minPosition = minDegrees / chain.ConversionFactor(id) / chain.PositionUnitDegrees(id);
            limit.maxPosition = maxDegrees / chain.ConversionFactor(id) / chain.PositionUnitDegrees(id);
            limits.push_back(limit);
        }

        return true;
    }

    bool
    BuildEffectivePositionLimits(std::vector<DynamixelServoPositionLimit> & limits, bool requireReady = true)
    {
        limits.clear();

        std::vector<double> minLimits;
        std::vector<double> maxLimits;
        if (!BuildEffectivePositionLimitVectors(minLimits, maxLimits))
            return false;

        for (DynamixelServoChain * chainPtr : ParameterChains())
        {
            DynamixelServoChain & chain = *chainPtr;
            if (requireReady && !chain.ready())
            {
                Error("Servo chain is not ready: " + chain.name);
                return false;
            }

            if (!BuildPositionLimitsForChain(chain, minLimits, maxLimits, limits))
                return false;
        }

        return true;
    }

    bool
    ConfigureParameterChainIndirectAddresses(uint8_t & dxl_error)
    {
        for (DynamixelServoChain & chain : servoChains)
        {
            if (!chain.ConfigureIndirectAddressesForCommunicationMode(
                configuration.modelRegistry,
                configuration.controlTables,
                configuration.indirectLayouts,
                dxl_error))
                return false;
        }

        return true;
    }

    bool
    ApplyDefaultServoSettings()
    {
        constexpr uint32_t profileAcceleration = 50;
        constexpr uint32_t profileVelocity = 210;

        uint8_t dxl_error = 0;

        Debug("Setting control table on servos\n");

        if (!ConfigureParameterChainIndirectAddresses(dxl_error))
            return false;

        if (!ApplyProfileAndPID(profileAcceleration, profileVelocity, dxl_error))
            return false;

        std::vector<DynamixelServoPositionLimit> limits;
        if (!BuildEffectivePositionLimits(limits) || !ApplyPositionLimits(limits, dxl_error))
            return false;

        for (DynamixelServoChain & chain : servoChains)
        {
            const dictionary * controlTable = ControlTableFor(chain);
            if (!controlTable)
                return false;

            if (!chain.ApplyStartupForCommunicationMode(*controlTable))
                return false;
        }

        return true;
    }

    std::vector<DynamixelServoChain *>
    ParameterChains()
    {
        std::vector<DynamixelServoChain *> chains;
        for (DynamixelServoChain & chain : servoChains)
            if (chain.IsParameterChain())
                chains.push_back(&chain);

        return chains;
    }

    std::vector<int>
    ParameterIndexesFor(const DynamixelServoChain & chain)
    {
        int index = 0;
        for (const DynamixelServoChain & configuredChain : servoChains)
        {
            if (!configuredChain.IsParameterChain())
                continue;

            if (configuredChain.name == chain.name)
            {
                std::vector<int> indexes;
                for (int i = 0; i < chain.size(); i++)
                    indexes.push_back(index + i);
                return indexes;
            }

            index += configuredChain.size();
        }

        return {};
    }

    bool
    LoadParameterMatrices(DynamixelServoParameterFile & parameterFile)
    {
        for (DynamixelServoChain * chain : ParameterChains())
        {
            const dictionary * controlTable = ControlTableFor(*chain);
            if (!controlTable)
                return false;

            chain->parameters = parameterFile.ParametersFor(*chain, *controlTable);
            if (chain->parameters.empty())
            {
                Error(parameterFile.last_error());
                return false;
            }
        }

        return true;
    }

    bool
    HasReceivedGoalPosition() const
    {
        // Extra startup guard for legacy models where Tick may be called before real GOAL_POSITION data has arrived.
        return goalPosition.connected() && goalPosition.size() >= configuredServoCount && servoGoalPosition(0) > 0;
    }

    void
    ClipGoalPositionsToLimits()
    {
        std::vector<double> minLimits;
        std::vector<double> maxLimits;
        if (!BuildEffectivePositionLimitVectors(minLimits, maxLimits))
            return;

        for (DynamixelServoChain * chain : ParameterChains())
        {
            std::vector<int> limitIndexes = ParameterIndexesFor(*chain);
            for (int row = 0; row < chain->size(); row++)
            {
                int ioIndex = chain->ioIndex + row;
                servoGoalPosition(ioIndex) = clip(servoGoalPosition(ioIndex), minLimits[limitIndexes[row]], maxLimits[limitIndexes[row]]);
            }
        }
    }

    double
    ClampGraphPoseToConfiguredLimits(DynamixelServoChain & chain, int id, double position, const std::string & key)
    {
        if (chain.UsesDirectPositionCommunication())
            return clip(position, chain.softwareMin[id], chain.softwareMax[id]);

        const DynamixelChainConfiguration * chainConfiguration = DynamixelChainConfigurationFor(chain);
        if (!chainConfiguration)
            return position;

        const DynamixelServoPositionLimit * limit = DefaultPositionLimitFor(*chainConfiguration, id);
        if (!limit)
            return position;

        const double minServoPosition = limit->minPosition * chain.PositionUnitDegrees(id) * chain.ConversionFactor(id);
        const double maxServoPosition = limit->maxPosition * chain.PositionUnitDegrees(id) * chain.ConversionFactor(id);
        const double servoPosition = ApplyTransform(
            position,
            BoolMapValue(chain.goalPreInverted, id),
            DoubleMapValue(chain.goalOffset, id),
            BoolMapValue(chain.goalPostInverted, id));
        const double clippedServoPosition = clip(servoPosition, minServoPosition, maxServoPosition);
        if (std::fabs(clippedServoPosition - servoPosition) > 1e-6)
        {
            const double clippedGraphPosition = InvertTransform(
                clippedServoPosition,
                BoolMapValue(chain.goalPreInverted, id),
                DoubleMapValue(chain.goalOffset, id),
                BoolMapValue(chain.goalPostInverted, id));
            Warning(
                "Clamped " + key + " for " + chain.name +
                " servo ID " + std::to_string(id) +
                " from " + std::to_string(position) +
                " to reachable value " + std::to_string(clippedGraphPosition) + ".");
            return clippedGraphPosition;
        }

        return position;
    }

    void
    SimulateFeedback()
    {
        float maxStep = std::max(0.0f, simulationRate.as_float());

        for (int i = 0; i < configuredServoCount; i++)
        {
            if (goalPosition.connected() && std::isnan(goalPosition(i)))
            {
                Warning("DynamixelController module position input has NaN.");
                return;
            }
            if (goalCurrent.connected() && std::isnan(goalCurrent(i)))
            {
                Warning("DynamixelController module current input has NaN.");
                return;
            }

            if (goalPosition.connected())
                servoPresentPosition(i) = servoPresentPosition(i) + 0.9 * clip(servoGoalPosition(i) - servoPresentPosition(i), -maxStep, maxStep);

            if (goalCurrent.connected())
                servoPresentCurrent(i) = servoPresentCurrent(i) + 0.06 * (goalCurrent(i) - servoPresentCurrent(i));
        }
    }

    bool
    CreateDynamixelServoChains()
    {
        servoChains.clear();

        const DynamixelRobotConfiguration * robotConfiguration = CurrentDynamixelRobotConfiguration();
        const std::vector<DynamixelChainConfiguration> * chainConfigurations = CurrentDynamixelChainConfigurations();
        if (!robotConfiguration || !chainConfigurations)
        {
            Error("No servo chain configuration found for " + robotName);
            return false;
        }

        for (const DynamixelChainConfiguration & chainConfiguration : *chainConfigurations)
        {
            const auto serialPort = robotConfiguration->serialPorts.find(chainConfiguration.name);
            if (serialPort == robotConfiguration->serialPorts.end())
            {
                Error("No serial port configured for " + robotName + " chain " + chainConfiguration.name);
                return false;
            }

            DynamixelServoChainSettings settings;
            settings.name = chainConfiguration.name;
            settings.idMin = chainConfiguration.idMin;
            settings.idMax = chainConfiguration.idMax;
            settings.ioIndex = chainConfiguration.ioIndex;
            settings.baudRate = chainConfiguration.baudRate;
            settings.serialPort = serialPort->second;
            settings.parameterChain = chainConfiguration.parameterChain;
            settings.communication = chainConfiguration.communication;
            settings.models = chainConfiguration.models;
            settings.positionUnitDegrees = chainConfiguration.positionUnitDegrees;
            settings.conversionFactors = chainConfiguration.conversionFactors;
            settings.softwareMin = chainConfiguration.softwareMin;
            settings.softwareMax = chainConfiguration.softwareMax;
            settings.inputMin = chainConfiguration.inputMin;
            settings.inputMax = chainConfiguration.inputMax;
            settings.positionMin = chainConfiguration.positionMin;
            settings.positionMax = chainConfiguration.positionMax;
            settings.goalPreInverted = chainConfiguration.goalPreInverted;
            settings.goalOffset = chainConfiguration.goalOffset;
            settings.goalPostInverted = chainConfiguration.goalPostInverted;
            settings.presentPositionPreInverted = chainConfiguration.presentPositionPreInverted;
            settings.presentPositionOffset = chainConfiguration.presentPositionOffset;
            settings.presentPositionPostInverted = chainConfiguration.presentPositionPostInverted;
            settings.presentCurrentPreInverted = chainConfiguration.presentCurrentPreInverted;
            settings.presentCurrentOffset = chainConfiguration.presentCurrentOffset;
            settings.presentCurrentPostInverted = chainConfiguration.presentCurrentPostInverted;
            settings.startupPosition = chainConfiguration.startupPosition;
            settings.shutdownPosition = chainConfiguration.shutdownPosition;
            settings.startupWrites = chainConfiguration.startupWrites;
            settings.detectRange = chainConfiguration.detectRange;
            settings.detectRangeGoalPosition = chainConfiguration.detectRangeGoalPosition;
            settings.detectRangePositionOffset = chainConfiguration.detectRangePositionOffset;
            settings.detectRangePositionRange = chainConfiguration.detectRangePositionRange;
            settings.detectRangeTorqueLimit = chainConfiguration.detectRangeTorqueLimit;
            settings.detectRangeMaxTorqueLimit = chainConfiguration.detectRangeMaxTorqueLimit;
            settings.detectRangeFullRangePosition = chainConfiguration.detectRangeFullRangePosition;
            settings.writeDelay = chainConfiguration.writeDelay;
            settings.detectRangeMoveDelay = chainConfiguration.detectRangeMoveDelay;
            servoChains.emplace_back(std::move(settings));
        }

        if (!ValidateChainCommunication())
            return false;

        configuredServoCount = ConfiguredServoCount();

        for (DynamixelServoChain & chain : servoChains)
        {
            chain.debug = [this](const std::string & message)
            {
                Debug(message);
            };
            chain.warning = [this](const std::string & message)
            {
                Warning(message);
            };
            chain.fatal = [this](const std::string & message)
            {
                Notify(msg_fatal_error, message);
            };
        }

        return true;
    }

    void
    SetParameters() override
    {
        const int requestedServoCount = RequestedServoCount();
        if (requestedServoCount > 0)
            return;

        if (!LoadConfiguration())
            return;

        std::string configuredRobotName = RequestedRobotName();
        if (configuredRobotName.empty())
        {
            if (configuration.robots.empty())
            {
                Error("No robot configurations are available.");
                return;
            }

            configuredRobotName = configuration.robots.begin()->first;
            SetParameter("robot", configuredRobotName);
        }

        const int detectedServoCount = ConfiguredServoCountForRobot(configuredRobotName);
        if (detectedServoCount <= 0)
        {
            Error("Could not determine ServoCount for robot " + configuredRobotName + ".");
            return;
        }

        SetParameter("robot", configuredRobotName);
        SetParameter("ServoCount", std::to_string(detectedServoCount));
    }

    void
    Init() override
    {
        Bind(simulate, "Simulate");
        Bind(robot, "robot");
        Bind(servoCount, "ServoCount");

        if (!LoadConfiguration())
            return;

        robotName = ResolveParameterReference(robot.as_string());

        const DynamixelRobotConfiguration * robotConfiguration = CurrentDynamixelRobotConfiguration();
        if (!robotConfiguration)
        {
            Error(robotName + " is not supported");
            return;
        }

        if (!CreateDynamixelServoChains() || servoChains.empty())
        {
            Notify(msg_fatal_error, "Robot servo chain configuration is incomplete");
            return;
        }

        if (servoCount.as_int() != configuredServoCount)
        {
            Notify(
                msg_fatal_error,
                "DynamixelController ServoCount is " + std::to_string(servoCount.as_int()) +
                " but robot " + robotName + " configuration has " + std::to_string(configuredServoCount) + " servos.");
            return;
        }

        if (!ValidateGraphServoCount())
            return;

        Debug("Connecting to " + robotName + " (" + robotConfiguration->type + ")");
        DebugConfigurationSummary(*robotConfiguration);

        Bind(minLimitPosition, "MinLimitPosition");
        Bind(maxLimitPosition, "MaxLimitPosition");
        Bind(usePositionLimitParameters, "UsePositionLimitParameters");
        Bind(servoControlMode, "ServoControlMode");
        Bind(simulationRate, "SimulationRate");
        controlMode = servoControlMode.as_string();

        Bind(goalPosition, "GOAL_POSITION");
        Bind(goalCurrent, "GOAL_CURRENT");
        Bind(goalPWM, "GOAL_PWM");
        Bind(torqueEnable, "TORQUE_ENABLE");

        Bind(presentPosition, "PRESENT_POSITION");
        Bind(presentCurrent, "PRESENT_CURRENT");
        Bind(state, "STATE");
        SetOutputLabels(presentPosition);
        SetOutputLabels(presentCurrent);
        SetStateOutputLabels();
        PublishControllerState();

        Notify(msg_print, "DynamixelController: " + robotName);

        Debug("Servo control mode: " + controlMode);

        if (presentPosition.size() != configuredServoCount || presentCurrent.size() != configuredServoCount)
        {
            Notify(msg_fatal_error, "Output size does not match robot configuration.");
            return;
        }
        servoGoalPosition.realloc(configuredServoCount);
        servoPresentPosition.realloc(configuredServoCount);
        servoPresentCurrent.realloc(configuredServoCount);

        if (goalPosition.connected() && goalPosition.size() != configuredServoCount)
        {
            Notify(msg_fatal_error, "Input size goal position does not match robot configuration\n");
            return;
        }
        if (goalCurrent.connected() && goalCurrent.size() != configuredServoCount)
        {
            Notify(msg_fatal_error, "Input size goal current does not match robot configuration\n");
            return;
        }
        if (goalPWM.connected() && goalPWM.size() != configuredServoCount)
        {
            Notify(msg_fatal_error, "Input size goal PWM does not match robot configuration\n");
            return;
        }
        if (torqueEnable.connected() && torqueEnable.size() != configuredServoCount)
        {
            Notify(msg_fatal_error, "Input size torque enable does not match robot configuration\n");
            return;
        }

        if (simulate)
        {
            Notify(msg_print, "Simulate servos");
            hardwareInitialized = true;
            return;
        }
    }

    void
    Tick() override
    {
        PublishControllerState();
        TickControllerMode();
    }

    bool
    ApplyLoadedServoSettings()
    {
        uint8_t dxl_error = 0;

        Debug("Setting control table on servos\n");

        for (DynamixelServoChain & chain : servoChains)
        {
            if (!chain.ApplyLoadedControlTableSettingsForCommunicationMode(
                configuration.modelRegistry,
                configuration.controlTables,
                configuration.indirectLayouts,
                dxl_error))
                return false;
        }

        return true;
    }

    bool
    ApplyConfiguredPositionLimits()
    {
        uint8_t dxl_error = 0;
        std::vector<DynamixelServoPositionLimit> limits;
        if (!BuildEffectivePositionLimits(limits))
            return false;

        return ApplyPositionLimits(limits, dxl_error);
    }

    bool
    UpdateRobotTypeServoValue(dictionary & robotTypeData, const std::string & chainName, int id, const std::string & key, double servoValue)
    {
        if (!robotTypeData.contains("chains") || !robotTypeData["chains"].is_list())
        {
            Error("Robot type configuration is missing chains.");
            return false;
        }

        list & chains = robotTypeData["chains"].as_list();
        for (value & chainValue : chains)
        {
            if (!chainValue.is_dictionary())
                continue;

            dictionary & chainData = chainValue.as_dictionary();
            if (!chainData.contains("name") || chainData["name"].as_string() != chainName)
                continue;

            const std::string idKey = std::to_string(id);
            if (!chainData.contains("servos") || !chainData["servos"].is_dictionary() ||
                !chainData["servos"].as_dictionary().contains(idKey) ||
                !chainData["servos"][idKey].is_dictionary())
            {
                Error("Robot type configuration is missing servo " + idKey + " in chain " + chainName + ".");
                return false;
            }

            chainData["servos"][idKey][key] = servoValue;
            return true;
        }

        Error("Robot type configuration is missing chain " + chainName + ".");
        return false;
    }

    bool
    UpdateRobotTypePositionLimit(dictionary & robotTypeData, const DynamixelServoPositionLimit & limit)
    {
        return UpdateRobotTypeServoValue(robotTypeData, limit.chainName, limit.id, "min", std::lround(limit.minPosition)) &&
            UpdateRobotTypeServoValue(robotTypeData, limit.chainName, limit.id, "max", std::lround(limit.maxPosition));
    }

    bool
    LoadRobotTypeData(dictionary & robotTypeData, std::filesystem::path & path)
    {
        const DynamixelRobotConfiguration * robotConfiguration = CurrentDynamixelRobotConfiguration();
        if (!robotConfiguration)
        {
            Error("No robot configuration found for " + robotName + ".");
            return false;
        }

        const auto pathEntry = configuration.robotTypePaths.find(robotConfiguration->type);
        if (pathEntry == configuration.robotTypePaths.end())
        {
            Error("No robot type file found for " + robotConfiguration->type + ".");
            return false;
        }

        path = pathEntry->second;

        try
        {
            robotTypeData.load_json(path.string());
        }
        catch (const std::exception & e)
        {
            Error("Could not read robot type configuration file " + path.string() + ": " + e.what());
            return false;
        }

        return true;
    }

    bool
    WriteRobotTypeData(const dictionary & robotTypeData, const std::filesystem::path & path)
    {
        std::ofstream file(path);
        if (!file)
        {
            Error("Could not write robot type configuration file " + path.string() + ".");
            return false;
        }

        file << PrettyJson(robotTypeData.json());
        return true;
    }

    bool
    SavePositionLimits()
    {
        std::vector<DynamixelServoPositionLimit> limits;
        if (!BuildEffectivePositionLimits(limits, false))
            return false;

        dictionary robotTypeData;
        std::filesystem::path path;
        if (!LoadRobotTypeData(robotTypeData, path))
            return false;

        for (const DynamixelServoPositionLimit & limit : limits)
            if (!UpdateRobotTypePositionLimit(robotTypeData, limit))
                return false;

        if (!WriteRobotTypeData(robotTypeData, path))
            return false;

        Notify(msg_print, "Saved Dynamixel position limits to " + path.string() + ".");
        return true;
    }

    bool
    SavePresentPositionAsPose(const std::string & key, bool startup)
    {
        if (hardwareInitialized)
            ReadPresentPositionForConfiguredChains();

        if (presentPosition.size() != configuredServoCount)
        {
            Error("Cannot save Dynamixel pose because PRESENT_POSITION is not configured.");
            return false;
        }

        dictionary robotTypeData;
        std::filesystem::path path;
        if (!LoadRobotTypeData(robotTypeData, path))
            return false;

        for (DynamixelServoChain & chain : servoChains)
        {
            std::map<int, double> & pose = ConfiguredPose(chain, startup);
            for (int id = chain.idMin; id <= chain.idMax; id++)
            {
                const int index = chain.ioIndex + chain.row(id);
                const double position = presentPosition(index);
                if (std::isnan(position))
                {
                    Error("Cannot save Dynamixel pose because PRESENT_POSITION contains NaN.");
                    return false;
                }

                if (!UpdateRobotTypeServoValue(robotTypeData, chain.name, id, key, position))
                    return false;

                pose[id] = position;
            }
        }

        if (!WriteRobotTypeData(robotTypeData, path))
            return false;

        Notify(msg_print, "Saved Dynamixel " + key + " values to " + path.string() + ".");
        return true;
    }

    void
    Command(std::string commandName, dictionary & parameters) override
    {
        if (commandName == "SavePositionLimits")
        {
            SavePositionLimits();
            return;
        }
        if (commandName == "SaveStartupPosition")
        {
            SavePresentPositionAsPose("startup_position", true);
            return;
        }
        if (commandName == "SaveShutdownPosition")
        {
            SavePresentPositionAsPose("shutdown_position", false);
            return;
        }
        if (commandName == "start_up" || commandName == "StartUp")
        {
            BeginStartUpMode();
            return;
        }
        if (commandName == "shut_down" || commandName == "ShutDown")
        {
            BeginShutDownMode();
            return;
        }
        if (commandName == "idle" || commandName == "Idle")
        {
            EnterState(ControllerMode::idle);
            return;
        }
        if (commandName == "initialize" || commandName == "Initialize" || commandName == "initialization")
        {
            BeginInitializationMode();
            return;
        }
        if (commandName == "ramp_up" || commandName == "RampUp")
        {
            BeginRampUpMode();
            return;
        }
        if (commandName == "ramp_down" || commandName == "RampDown")
        {
            BeginRampDownMode();
            return;
        }
        if (commandName == "torque_off" || commandName == "TorqueOff" || commandName == "emergency_stop")
        {
            Notify(msg_warning, "DynamixelController emergency torque off requested.");
            EnterState(ControllerMode::torqueOff);
            return;
        }
        if (commandName == "operate" || commandName == "Operation")
        {
            EnterState(ControllerMode::operation);
            return;
        }

        Module::Command(commandName, parameters);
    }

    ~DynamixelController()
    {
        if (!simulate && hardwareOpened)
        {
            for (DynamixelServoChain & chain : servoChains)
            {
                chain.debug = nullptr;
                chain.warning = nullptr;
                chain.fatal = nullptr;
            }
        }

        for (DynamixelServoChain & chain : servoChains)
            chain.Close();
    }
};

INSTALL_CLASS(DynamixelController)
