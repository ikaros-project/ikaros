#pragma once

#include "ikaros.h"
#include "dynamixel_sdk.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace DynamixelServoChainConstants
{
    inline constexpr double PROTOCOL_VERSION = 2.0;

    inline constexpr int ADDR_OPERATING_MODE = 11;

    inline constexpr double TIMER_POWER_ON = 2.0;
    inline constexpr double TIMER_POWER_OFF = 2.0;
    inline constexpr double STARTUP_POSE_DURATION = 3.0;
    inline constexpr double TIMER_POWER_OFF_EXTENDED = 3.0;
    inline constexpr double STARTUP_POSE_DELAY = 1.0;
    inline constexpr double SHUTDOWN_POSE_DURATION = 3.0;
    inline constexpr double SHUTDOWN_POSE_HOLD_DURATION = 0.5;
    inline constexpr double SHUTDOWN_STEP_TIME = 0.1;
    inline constexpr double POWER_OFF_RAMP_STEP_TIME = 0.1;
    inline constexpr double CURRENT_UNIT = 3.36;
    inline constexpr double PWM_PERCENT_UNIT = 0.11299;
}

struct DynamixelServoChainSettings
{
    std::string name;
    int idMin = 0;
    int idMax = 0;
    int ioIndex = 0;
    int baudRate = 0;
    std::string serialPort;
    bool parameterChain = true;
    std::string communication = "sync_indirect";
    std::map<int, std::string> models;
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
    bool detectRange = false;
    int detectRangeGoalPosition = 0;
    int detectRangePositionOffset = 0;
    int detectRangePositionRange = 0;
    int detectRangeTorqueLimit = 0;
    int detectRangeMaxTorqueLimit = 0;
    int detectRangeFullRangePosition = 0;
    double writeDelay = 0;
    double detectRangeMoveDelay = 0;
};

class DynamixelServoChain
{
public:
    std::string name;
    int idMin = 0;
    int idMax = 0;
    int ioIndex = 0;
    int baudRate = 0;
    std::string serialPort;
    bool parameterChain = true;
    std::string communication = "sync_indirect";
    std::map<int, std::string> models;
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
    bool detectRange = false;
    int detectRangeGoalPosition = 0;
    int detectRangePositionOffset = 0;
    int detectRangePositionRange = 0;
    int detectRangeTorqueLimit = 0;
    int detectRangeMaxTorqueLimit = 0;
    int detectRangeFullRangePosition = 0;
    double writeDelay = 0;
    double detectRangeMoveDelay = 0;

    std::function<void(const std::string &)> debug;
    std::function<void(const std::string &)> warning;
    std::function<void(const std::string &)> fatal;

    ikaros::matrix parameters;
    std::vector<std::string> parameterNames;

    DynamixelServoChain() = default;
    explicit DynamixelServoChain(DynamixelServoChainSettings settings);
    ~DynamixelServoChain();
    DynamixelServoChain(const DynamixelServoChain &) = delete;
    DynamixelServoChain &
    operator=(const DynamixelServoChain &) = delete;
    DynamixelServoChain(DynamixelServoChain && other) noexcept;
    DynamixelServoChain &
    operator=(DynamixelServoChain && other) noexcept;

    int
    size() const;
    int
    row(int id) const;
    std::string
    ModelName(int id) const;
    double
    PositionUnitDegrees(int id) const;
    double
    ConversionFactor(int id) const;
    std::string
    ControlTableName(int id, const ikaros::dictionary & modelRegistry) const;
    const ikaros::dictionary *
    ControlTable(int id, const ikaros::dictionary & modelRegistry, const ikaros::dictionary & controlTables) const;
    const ikaros::dictionary *
    ControlTable(const ikaros::dictionary & modelRegistry, const ikaros::dictionary & controlTables) const;
    std::string
    IndirectLayoutName(int id, const ikaros::dictionary & modelRegistry) const;
    const ikaros::dictionary *
    IndirectLayout(const ikaros::dictionary & modelRegistry, const ikaros::dictionary & indirectLayouts) const;
    bool
    UsesSingleControlTable(const ikaros::dictionary & modelRegistry) const;
    bool
    UsesSingleIndirectLayout(const ikaros::dictionary & modelRegistry) const;
    bool
    IsParameterChain() const;
    bool
    UsesSyncIndirectCommunication() const;
    bool
    UsesDirectPositionCommunication() const;

    void
    Warn(const std::string & message) const;
    void
    Debug(const std::string & message) const;
    void
    Fatal(const std::string & message) const;

    bool
    Open();
    bool
    CreateSyncObjects(const ikaros::dictionary & indirectLayout);
    void
    Close();
    void
    ClearPort();
    bool
    ready() const;
    std::string
    PacketError(uint8_t dxl_error) const;
    std::string
    PortName() const;

    bool
    Write1Byte(int id, int address, uint8_t value, uint8_t & dxl_error);
    bool
    Read1Byte(int id, int address, uint8_t & value, uint8_t & dxl_error);
    bool
    Write2Byte(int id, int address, uint16_t value, uint8_t & dxl_error);
    bool
    Write4Byte(int id, int address, uint32_t value, uint8_t & dxl_error);
    bool
    Read2Byte(int id, int address, uint16_t & value, uint8_t & dxl_error);
    bool
    Read4Byte(int id, int address, uint32_t & value, uint8_t & dxl_error);

    bool
    Write2ByteForAll(int address, uint16_t value, const std::string & description, uint8_t & dxl_error);
    bool
    Write4ByteForAll(int address, uint32_t value, const std::string & description, uint8_t & dxl_error);
    bool
    WriteIndirectAddressBytes(int indirectAddress, int directAddress, int byteCount, const std::string & description, uint8_t & dxl_error);
    bool
    ConfigureIndirectAddresses(const ikaros::dictionary & controlTable, const ikaros::dictionary & indirectLayout, uint8_t & dxl_error);
    bool
    ConfigureIndirectAddresses(const ikaros::dictionary & modelRegistry, const ikaros::dictionary & controlTables, const ikaros::dictionary & indirectLayout, uint8_t & dxl_error);
    bool
    ConfigureIndirectAddressesForCommunicationMode(const ikaros::dictionary & modelRegistry, const ikaros::dictionary & controlTables, const ikaros::dictionary & indirectLayouts, uint8_t & dxl_error);
    bool
    DisableTorque(const ikaros::dictionary & controlTable, uint8_t & dxl_error);
    bool
    ControlTableAddress(const ikaros::dictionary & controlTable, const std::string & parameterName, int expectedBytes, int & address) const;
    bool
    IndirectLayoutAddress(const ikaros::dictionary & indirectLayout, const std::string & parameterName, int & address) const;
    bool
    IndirectLayoutOffset(const ikaros::dictionary & indirectLayout, const std::string & offsetGroup, const std::string & parameterName, int byteCount, int dataLength, int & offset) const;
    bool
    ApplyProfileAndPID(const ikaros::dictionary & controlTable, uint32_t profileAcceleration, uint32_t profileVelocity, uint16_t pGain, uint16_t iGain, uint16_t dGain, uint8_t & dxl_error);
    bool
    ApplyProfileAndPIDForCommunicationMode(const ikaros::dictionary & controlTable, uint32_t profileAcceleration, uint32_t profileVelocity, uint16_t pGain, uint16_t iGain, uint16_t dGain, uint8_t & dxl_error);
    bool
    WritePositionLimit(const ikaros::dictionary & controlTable, int id, uint32_t minPosition, uint32_t maxPosition, uint8_t & dxl_error);
    bool
    ApplyLoadedControlTableSettings(const ikaros::dictionary & controlTable, const ikaros::dictionary & indirectLayout, uint8_t & dxl_error);
    bool
    ApplyLoadedControlTableSettings(const ikaros::dictionary & modelRegistry, const ikaros::dictionary & controlTables, const ikaros::dictionary & indirectLayout, uint8_t & dxl_error);
    bool
    ApplyLoadedControlTableSettingsForCommunicationMode(const ikaros::dictionary & modelRegistry, const ikaros::dictionary & controlTables, const ikaros::dictionary & indirectLayouts, uint8_t & dxl_error);

    bool
    PowerOn(const ikaros::dictionary & controlTable);
    bool
    PowerOff(const ikaros::dictionary & controlTable);
    bool
    PowerOffDirectPosition(const ikaros::dictionary & controlTable);
    bool
    PowerOnForCommunicationMode(const ikaros::dictionary & controlTable);
    bool
    PreparePowerOnRampForCommunicationMode(const ikaros::dictionary & controlTable);
    bool
    PreparePowerOnRampForCommunicationMode(const ikaros::dictionary & controlTable, const std::vector<bool> & skipServo);
    bool
    RampPowerOnForCommunicationMode(const ikaros::dictionary & controlTable, double phase);
    bool
    RampPowerOnForCommunicationMode(const ikaros::dictionary & controlTable, double phase, const std::vector<bool> & skipServo);
    bool
    RestorePowerOnRampForCommunicationMode(const ikaros::dictionary & controlTable);
    bool
    RestorePowerOnRampForCommunicationMode(const ikaros::dictionary & controlTable, const std::vector<bool> & skipServo);
    bool
    PowerOffForCommunicationMode(const ikaros::dictionary & controlTable);
    bool
    Communicate(ikaros::matrix & goalPosition, ikaros::matrix & goalCurrent, ikaros::matrix & goalPWM, ikaros::matrix & torqueEnable, ikaros::matrix & presentPosition, ikaros::matrix & presentCurrent, const std::string & controlMode);
    bool
    CommunicateForCommunicationMode(ikaros::matrix & goalPosition, ikaros::matrix & goalCurrent, ikaros::matrix & goalPWM, ikaros::matrix & torqueEnable, ikaros::matrix & presentPosition, ikaros::matrix & presentCurrent, const std::string & controlMode, const ikaros::dictionary & controlTable);
    bool
    ReadFeedbackForCommunicationMode(ikaros::matrix & presentPosition, ikaros::matrix & presentCurrent, const ikaros::dictionary & controlTable);
    bool
    CommunicateDirectPosition(ikaros::matrix & goalPosition, const ikaros::dictionary & controlTable);
    bool
    WriteGoal(ikaros::matrix & goalPosition, ikaros::matrix & goalPWM, ikaros::matrix & torqueEnable);
    bool
    WriteGoalForCommunicationMode(ikaros::matrix & goalPosition, ikaros::matrix & goalPWM, ikaros::matrix & torqueEnable, const ikaros::dictionary & controlTable);
    bool
    SetTorque(const ikaros::dictionary & controlTable, uint8_t enabled);
    bool
    EnableTorqueForCommunicationMode(const ikaros::dictionary & controlTable);
    bool
    PreparePowerOffRampForCommunicationMode(const ikaros::dictionary & controlTable);
    bool
    RampPowerOffForCommunicationMode(const ikaros::dictionary & controlTable, double phase);
    bool
    DisableTorqueForCommunicationMode(const ikaros::dictionary & controlTable);
    bool
    RestorePowerOffRampForCommunicationMode(const ikaros::dictionary & controlTable);
    bool
    ApplyStartupForCommunicationMode(const ikaros::dictionary & controlTable);
    bool
    DetectRangeForCommunicationMode(const ikaros::dictionary & controlTable);
    bool
    BeginDetectRangeDirectPosition(const ikaros::dictionary & controlTable);
    bool
    FinishDetectRangeDirectPosition(const ikaros::dictionary & controlTable);
    bool
    ApplyDirectPositionStartup(const ikaros::dictionary & controlTable);
    bool
    DetectRangeDirectPosition(const ikaros::dictionary & controlTable);
    void
    PrepareFeedbackForCommunicationMode(ikaros::matrix & goalPosition, ikaros::matrix & presentPosition);
    void
    ConvertGoalsForCommunicationMode(ikaros::matrix & goalPosition);

private:
    std::unique_ptr<dynamixel::PortHandler> portHandler;
    dynamixel::PacketHandler *packetHandler = nullptr;
    std::unique_ptr<dynamixel::GroupSyncRead> syncRead;
    std::unique_ptr<dynamixel::GroupSyncWrite> syncWrite;
    int syncWriteDataStart = 0;
    int syncWriteDataLength = 0;
    int syncReadDataStart = 0;
    int syncReadDataLength = 0;
    int syncWriteTorqueEnableOffset = 0;
    int syncWriteGoalPositionOffset = 0;
    int syncWriteGoalCurrentOffset = 0;
    int syncWriteGoalPWMOffset = 0;
    int syncReadPresentPositionOffset = 0;
    int syncReadPresentCurrentOffset = 0;
    std::map<int, bool> currentSupported;
    std::vector<uint16_t> powerOnRampTargetValue;
    std::vector<bool> powerOnRampHasTargetValue;
    std::vector<uint16_t> powerOffRampStartValue;
    std::vector<bool> powerOffRampHasStartValue;

    void
    ReleaseSyncObjects();
};
