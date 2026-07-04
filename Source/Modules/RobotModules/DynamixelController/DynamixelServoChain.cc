#include "DynamixelServoChain.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>
#include <utility>

using namespace ikaros;

namespace
{
    double
    MapValue(const std::map<int, double> & values, int id, double fallback = 0)
    {
        const auto value = values.find(id);
        if (value == values.end())
            return fallback;

        return value->second;
    }

    std::string
    DynamixelError(const DynamixelServoChain & chain, uint8_t dxl_error)
    {
        std::string error = chain.PacketError(dxl_error);
        if (error.empty())
            return "";
        return ", DXL Error: " + error;
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

DynamixelServoChain::DynamixelServoChain(DynamixelServoChainSettings settings):
    name(std::move(settings.name)),
    idMin(settings.idMin),
    idMax(settings.idMax),
    ioIndex(settings.ioIndex),
    baudRate(settings.baudRate),
    serialPort(std::move(settings.serialPort)),
    parameterChain(settings.parameterChain),
    communication(std::move(settings.communication)),
    models(std::move(settings.models)),
    positionUnitDegrees(std::move(settings.positionUnitDegrees)),
    conversionFactors(std::move(settings.conversionFactors)),
    softwareMin(std::move(settings.softwareMin)),
    softwareMax(std::move(settings.softwareMax)),
    inputMin(std::move(settings.inputMin)),
    inputMax(std::move(settings.inputMax)),
    positionMin(std::move(settings.positionMin)),
    positionMax(std::move(settings.positionMax)),
    goalPreInverted(std::move(settings.goalPreInverted)),
    goalOffset(std::move(settings.goalOffset)),
    goalPostInverted(std::move(settings.goalPostInverted)),
    presentPositionPreInverted(std::move(settings.presentPositionPreInverted)),
    presentPositionOffset(std::move(settings.presentPositionOffset)),
    presentPositionPostInverted(std::move(settings.presentPositionPostInverted)),
    presentCurrentPreInverted(std::move(settings.presentCurrentPreInverted)),
    presentCurrentOffset(std::move(settings.presentCurrentOffset)),
    presentCurrentPostInverted(std::move(settings.presentCurrentPostInverted)),
    startupPosition(std::move(settings.startupPosition)),
    shutdownPosition(std::move(settings.shutdownPosition)),
    startupWrites(std::move(settings.startupWrites)),
    detectRange(settings.detectRange),
    detectRangeFirstGoalPosition(settings.detectRangeFirstGoalPosition),
    detectRangeSecondGoalPosition(settings.detectRangeSecondGoalPosition),
    detectRangePositionMinOffset(settings.detectRangePositionMinOffset),
    detectRangePositionMaxOffset(settings.detectRangePositionMaxOffset),
    detectRangeMovingSpeed(settings.detectRangeMovingSpeed),
    detectRangeTorqueLimit(settings.detectRangeTorqueLimit),
    detectRangeMaxTorqueLimit(settings.detectRangeMaxTorqueLimit),
    detectRangeFullRangePosition(settings.detectRangeFullRangePosition),
    writeDelay(settings.writeDelay),
    detectRangeMoveDelay(settings.detectRangeMoveDelay)
{
}

DynamixelServoChain::~DynamixelServoChain()
{
    Close();
}

DynamixelServoChain::DynamixelServoChain(DynamixelServoChain && other) noexcept:
    name(std::move(other.name)),
    idMin(other.idMin),
    idMax(other.idMax),
    ioIndex(other.ioIndex),
    baudRate(other.baudRate),
    serialPort(std::move(other.serialPort)),
    parameterChain(other.parameterChain),
    communication(std::move(other.communication)),
    models(std::move(other.models)),
    positionUnitDegrees(std::move(other.positionUnitDegrees)),
    conversionFactors(std::move(other.conversionFactors)),
    softwareMin(std::move(other.softwareMin)),
    softwareMax(std::move(other.softwareMax)),
    inputMin(std::move(other.inputMin)),
    inputMax(std::move(other.inputMax)),
    positionMin(std::move(other.positionMin)),
    positionMax(std::move(other.positionMax)),
    goalPreInverted(std::move(other.goalPreInverted)),
    goalOffset(std::move(other.goalOffset)),
    goalPostInverted(std::move(other.goalPostInverted)),
    presentPositionPreInverted(std::move(other.presentPositionPreInverted)),
    presentPositionOffset(std::move(other.presentPositionOffset)),
    presentPositionPostInverted(std::move(other.presentPositionPostInverted)),
    presentCurrentPreInverted(std::move(other.presentCurrentPreInverted)),
    presentCurrentOffset(std::move(other.presentCurrentOffset)),
    presentCurrentPostInverted(std::move(other.presentCurrentPostInverted)),
    startupPosition(std::move(other.startupPosition)),
    shutdownPosition(std::move(other.shutdownPosition)),
    startupWrites(std::move(other.startupWrites)),
    detectRange(other.detectRange),
    detectRangeFirstGoalPosition(other.detectRangeFirstGoalPosition),
    detectRangeSecondGoalPosition(other.detectRangeSecondGoalPosition),
    detectRangePositionMinOffset(other.detectRangePositionMinOffset),
    detectRangePositionMaxOffset(other.detectRangePositionMaxOffset),
    detectRangeMovingSpeed(other.detectRangeMovingSpeed),
    detectRangeTorqueLimit(other.detectRangeTorqueLimit),
    detectRangeMaxTorqueLimit(other.detectRangeMaxTorqueLimit),
    detectRangeFullRangePosition(other.detectRangeFullRangePosition),
    writeDelay(other.writeDelay),
    detectRangeMoveDelay(other.detectRangeMoveDelay),
    debug(std::move(other.debug)),
    warning(std::move(other.warning)),
    fatal(std::move(other.fatal)),
    parameters(std::move(other.parameters)),
    parameterNames(std::move(other.parameterNames)),
    portHandler(std::move(other.portHandler)),
    packetHandler(std::exchange(other.packetHandler, nullptr)),
    syncRead(std::move(other.syncRead)),
    syncWrite(std::move(other.syncWrite)),
    detectRangeFirstEndpointPosition(std::move(other.detectRangeFirstEndpointPosition)),
    syncWriteDataStart(other.syncWriteDataStart),
    syncWriteDataLength(other.syncWriteDataLength),
    syncReadDataStart(other.syncReadDataStart),
    syncReadDataLength(other.syncReadDataLength),
    syncWriteTorqueEnableOffset(other.syncWriteTorqueEnableOffset),
    syncWriteGoalPositionOffset(other.syncWriteGoalPositionOffset),
    syncWriteGoalCurrentOffset(other.syncWriteGoalCurrentOffset),
    syncWriteGoalPWMOffset(other.syncWriteGoalPWMOffset),
    syncReadPresentPositionOffset(other.syncReadPresentPositionOffset),
    syncReadPresentCurrentOffset(other.syncReadPresentCurrentOffset),
    currentSupported(std::move(other.currentSupported)),
    powerOnRampTargetValue(std::move(other.powerOnRampTargetValue)),
    powerOnRampHasTargetValue(std::move(other.powerOnRampHasTargetValue)),
    powerOffRampStartValue(std::move(other.powerOffRampStartValue)),
    powerOffRampHasStartValue(std::move(other.powerOffRampHasStartValue))
{
}

DynamixelServoChain &
DynamixelServoChain::operator=(DynamixelServoChain && other) noexcept
{
    if (this == &other)
        return *this;

    Close();

    name = std::move(other.name);
    idMin = other.idMin;
    idMax = other.idMax;
    ioIndex = other.ioIndex;
    baudRate = other.baudRate;
    serialPort = std::move(other.serialPort);
    parameterChain = other.parameterChain;
    communication = std::move(other.communication);
    models = std::move(other.models);
    positionUnitDegrees = std::move(other.positionUnitDegrees);
    conversionFactors = std::move(other.conversionFactors);
    softwareMin = std::move(other.softwareMin);
    softwareMax = std::move(other.softwareMax);
    inputMin = std::move(other.inputMin);
    inputMax = std::move(other.inputMax);
    positionMin = std::move(other.positionMin);
    positionMax = std::move(other.positionMax);
    goalPreInverted = std::move(other.goalPreInverted);
    goalOffset = std::move(other.goalOffset);
    goalPostInverted = std::move(other.goalPostInverted);
    presentPositionPreInverted = std::move(other.presentPositionPreInverted);
    presentPositionOffset = std::move(other.presentPositionOffset);
    presentPositionPostInverted = std::move(other.presentPositionPostInverted);
    presentCurrentPreInverted = std::move(other.presentCurrentPreInverted);
    presentCurrentOffset = std::move(other.presentCurrentOffset);
    presentCurrentPostInverted = std::move(other.presentCurrentPostInverted);
    startupPosition = std::move(other.startupPosition);
    shutdownPosition = std::move(other.shutdownPosition);
    startupWrites = std::move(other.startupWrites);
    detectRange = other.detectRange;
    detectRangeFirstGoalPosition = other.detectRangeFirstGoalPosition;
    detectRangeSecondGoalPosition = other.detectRangeSecondGoalPosition;
    detectRangePositionMinOffset = other.detectRangePositionMinOffset;
    detectRangePositionMaxOffset = other.detectRangePositionMaxOffset;
    detectRangeMovingSpeed = other.detectRangeMovingSpeed;
    detectRangeTorqueLimit = other.detectRangeTorqueLimit;
    detectRangeMaxTorqueLimit = other.detectRangeMaxTorqueLimit;
    detectRangeFullRangePosition = other.detectRangeFullRangePosition;
    writeDelay = other.writeDelay;
    detectRangeMoveDelay = other.detectRangeMoveDelay;
    detectRangeFirstEndpointPosition = std::move(other.detectRangeFirstEndpointPosition);
    debug = std::move(other.debug);
    warning = std::move(other.warning);
    fatal = std::move(other.fatal);
    parameters = std::move(other.parameters);
    parameterNames = std::move(other.parameterNames);
    portHandler = std::move(other.portHandler);
    packetHandler = std::exchange(other.packetHandler, nullptr);
    syncRead = std::move(other.syncRead);
    syncWrite = std::move(other.syncWrite);
    syncWriteDataStart = other.syncWriteDataStart;
    syncWriteDataLength = other.syncWriteDataLength;
    syncReadDataStart = other.syncReadDataStart;
    syncReadDataLength = other.syncReadDataLength;
    syncWriteTorqueEnableOffset = other.syncWriteTorqueEnableOffset;
    syncWriteGoalPositionOffset = other.syncWriteGoalPositionOffset;
    syncWriteGoalCurrentOffset = other.syncWriteGoalCurrentOffset;
    syncWriteGoalPWMOffset = other.syncWriteGoalPWMOffset;
    syncReadPresentPositionOffset = other.syncReadPresentPositionOffset;
    syncReadPresentCurrentOffset = other.syncReadPresentCurrentOffset;
    currentSupported = std::move(other.currentSupported);
    powerOnRampTargetValue = std::move(other.powerOnRampTargetValue);
    powerOnRampHasTargetValue = std::move(other.powerOnRampHasTargetValue);
    powerOffRampStartValue = std::move(other.powerOffRampStartValue);
    powerOffRampHasStartValue = std::move(other.powerOffRampHasStartValue);

    return *this;
}

int
DynamixelServoChain::size() const
{
    return idMax - idMin + 1;
}

int
DynamixelServoChain::row(int id) const
{
    return id - idMin;
}

std::string
DynamixelServoChain::ModelName(int id) const
{
    const auto model = models.find(id);
    if (model == models.end())
        return "";

    return model->second;
}

double
DynamixelServoChain::PositionUnitDegrees(int id) const
{
    const auto unitDegrees = positionUnitDegrees.find(id);
    if (unitDegrees == positionUnitDegrees.end())
        return 1.0;

    return unitDegrees->second;
}

double
DynamixelServoChain::ConversionFactor(int id) const
{
    const auto conversionFactor = conversionFactors.find(id);
    if (conversionFactor == conversionFactors.end())
        return 1.0;

    return conversionFactor->second;
}

std::string
DynamixelServoChain::ControlTableName(int id, const dictionary & modelRegistry) const
{
    const std::string modelName = ModelName(id);
    if (modelName.empty())
        return "";

    const dictionary & registeredModels = modelRegistry["models"].as_dictionary();
    if (!registeredModels.contains(modelName) || !registeredModels[modelName].is_dictionary())
        return "";

    const dictionary & model = registeredModels[modelName].as_dictionary();
    if (!model.contains("control_table"))
        return "";

    return model["control_table"].as_string();
}

const dictionary *
DynamixelServoChain::ControlTable(int id, const dictionary & modelRegistry, const dictionary & controlTables) const
{
    const std::string controlTableName = ControlTableName(id, modelRegistry);
    if (controlTableName.empty() || !controlTables.contains(controlTableName) || !controlTables[controlTableName].is_dictionary())
        return nullptr;

    return &controlTables[controlTableName].as_dictionary();
}

const dictionary *
DynamixelServoChain::ControlTable(const dictionary & modelRegistry, const dictionary & controlTables) const
{
    return ControlTable(idMin, modelRegistry, controlTables);
}

std::string
DynamixelServoChain::IndirectLayoutName(int id, const dictionary & modelRegistry) const
{
    const std::string modelName = ModelName(id);
    if (modelName.empty())
        return "";

    const dictionary & registeredModels = modelRegistry["models"].as_dictionary();
    if (!registeredModels.contains(modelName) || !registeredModels[modelName].is_dictionary())
        return "";

    const dictionary & model = registeredModels[modelName].as_dictionary();
    if (!model.contains("indirect_layout"))
        return "";

    return model["indirect_layout"].as_string();
}

const dictionary *
DynamixelServoChain::IndirectLayout(const dictionary & modelRegistry, const dictionary & indirectLayouts) const
{
    const std::string layoutName = IndirectLayoutName(idMin, modelRegistry);
    if (layoutName.empty() || !indirectLayouts.contains(layoutName) || !indirectLayouts[layoutName].is_dictionary())
        return nullptr;

    return &indirectLayouts[layoutName].as_dictionary();
}

bool
DynamixelServoChain::UsesSingleControlTable(const dictionary & modelRegistry) const
{
    std::set<std::string> controlTableNames;

    for (int id = idMin; id <= idMax; id++)
    {
        const std::string controlTableName = ControlTableName(id, modelRegistry);
        if (controlTableName.empty())
            return false;

        controlTableNames.insert(controlTableName);
    }

    return controlTableNames.size() == 1;
}

bool
DynamixelServoChain::UsesSingleIndirectLayout(const dictionary & modelRegistry) const
{
    std::set<std::string> layoutNames;

    for (int id = idMin; id <= idMax; id++)
    {
        const std::string layoutName = IndirectLayoutName(id, modelRegistry);
        if (layoutName.empty())
            return false;

        layoutNames.insert(layoutName);
    }

    return layoutNames.size() == 1;
}


bool
DynamixelServoChain::IsParameterChain() const
{
    return parameterChain;
}


bool
DynamixelServoChain::UsesSyncIndirectCommunication() const
{
    return communication == "sync_indirect";
}


bool
DynamixelServoChain::UsesDirectPositionCommunication() const
{
    return communication == "direct_position";
}


void
DynamixelServoChain::Warn(const std::string & message) const
{
    if (warning)
        warning(message);
    else
        std::cerr << message << std::endl;
}

void
DynamixelServoChain::Debug(const std::string & message) const
{
    if (debug)
        debug(message);
}

void
DynamixelServoChain::Fatal(const std::string & message) const
{
    if (fatal)
        fatal(message);
    else
        std::cerr << message << std::endl;
}

bool
DynamixelServoChain::Open()
{
    Close();

    portHandler.reset(dynamixel::PortHandler::getPortHandler(serialPort.c_str()));
    packetHandler = dynamixel::PacketHandler::getPacketHandler(DynamixelServoChainConstants::PROTOCOL_VERSION);

    Debug("Setting up serial port (" + name + ")");

    if (!portHandler->openPort())
    {
        Warn("Could not open serial port " + serialPort + " for " + name +
            ". It may already be used by another process.");
        Close();
        return false;
    }
    Debug("Succeeded to open serial port!");

    if (!portHandler->setBaudRate(baudRate))
    {
        Warn("Could not set baud rate " + std::to_string(baudRate) +
            " for " + name + " on serial port " + serialPort + ".");
        Close();
        return false;
    }
    Debug("Succeeded to change baudrate!");

    return true;
}

bool
DynamixelServoChain::CreateSyncObjects(const dictionary & indirectLayout)
{
    if (!ready())
    {
        Fatal("Cannot create sync objects for " + name + " before opening the servo chain");
        return false;
    }

    if (!IndirectLayoutAddress(indirectLayout, "Write Data Start", syncWriteDataStart) ||
        !IndirectLayoutAddress(indirectLayout, "Write Data Length", syncWriteDataLength) ||
        !IndirectLayoutAddress(indirectLayout, "Read Data Start", syncReadDataStart) ||
        !IndirectLayoutAddress(indirectLayout, "Read Data Length", syncReadDataLength))
        return false;

    if (!IndirectLayoutOffset(indirectLayout, "Write Offsets", "Torque Enable", 1, syncWriteDataLength, syncWriteTorqueEnableOffset) ||
        !IndirectLayoutOffset(indirectLayout, "Write Offsets", "Goal Position", 4, syncWriteDataLength, syncWriteGoalPositionOffset) ||
        !IndirectLayoutOffset(indirectLayout, "Write Offsets", "Goal Current", 2, syncWriteDataLength, syncWriteGoalCurrentOffset) ||
        !IndirectLayoutOffset(indirectLayout, "Write Offsets", "Goal PWM", 2, syncWriteDataLength, syncWriteGoalPWMOffset) ||
        !IndirectLayoutOffset(indirectLayout, "Read Offsets", "Present Position", 4, syncReadDataLength, syncReadPresentPositionOffset) ||
        !IndirectLayoutOffset(indirectLayout, "Read Offsets", "Present Current", 2, syncReadDataLength, syncReadPresentCurrentOffset))
        return false;

    syncWrite = std::make_unique<dynamixel::GroupSyncWrite>(portHandler.get(), packetHandler, syncWriteDataStart, syncWriteDataLength);
    syncRead = std::make_unique<dynamixel::GroupSyncRead>(portHandler.get(), packetHandler, syncReadDataStart, syncReadDataLength);
    return true;
}

void
DynamixelServoChain::Close()
{
    ReleaseSyncObjects();
    if (portHandler)
        portHandler->closePort();
    portHandler.reset();
    packetHandler = nullptr;
}

void
DynamixelServoChain::ReleaseSyncObjects()
{
    syncWrite.reset();
    syncRead.reset();
}

bool
DynamixelServoChain::ready() const
{
    return portHandler && packetHandler;
}

void
DynamixelServoChain::ClearPort()
{
    if (portHandler)
        portHandler->clearPort();
}

std::string
DynamixelServoChain::PacketError(uint8_t dxl_error) const
{
    if (!packetHandler)
        return "";

    return packetHandler->getRxPacketError(dxl_error);
}

std::string
DynamixelServoChain::PortName() const
{
    if (!portHandler)
        return "";

    return portHandler->getPortName();
}

bool
DynamixelServoChain::Write1Byte(int id, int address, uint8_t value, uint8_t & dxl_error)
{
    if (!ready())
    {
        Warn("Cannot write 1 byte to " + name + " because the chain is not ready.");
        return false;
    }

    return COMM_SUCCESS == packetHandler->write1ByteTxRx(portHandler.get(), id, address, value, &dxl_error);
}

bool
DynamixelServoChain::Read1Byte(int id, int address, uint8_t & value, uint8_t & dxl_error)
{
    if (!ready())
    {
        Warn("Cannot read 1 byte from " + name + " because the chain is not ready.");
        return false;
    }

    return COMM_SUCCESS == packetHandler->read1ByteTxRx(portHandler.get(), id, address, &value, &dxl_error);
}

bool
DynamixelServoChain::Write2Byte(int id, int address, uint16_t value, uint8_t & dxl_error)
{
    if (!ready())
    {
        Warn("Cannot write 2 bytes to " + name + " because the chain is not ready.");
        return false;
    }

    return COMM_SUCCESS == packetHandler->write2ByteTxRx(portHandler.get(), id, address, value, &dxl_error);
}

bool
DynamixelServoChain::Write4Byte(int id, int address, uint32_t value, uint8_t & dxl_error)
{
    if (!ready())
    {
        Warn("Cannot write 4 bytes to " + name + " because the chain is not ready.");
        return false;
    }

    return COMM_SUCCESS == packetHandler->write4ByteTxRx(portHandler.get(), id, address, value, &dxl_error);
}

bool
DynamixelServoChain::Read2Byte(int id, int address, uint16_t & value, uint8_t & dxl_error)
{
    if (!ready())
    {
        Warn("Cannot read 2 bytes from " + name + " because the chain is not ready.");
        return false;
    }

    return COMM_SUCCESS == packetHandler->read2ByteTxRx(portHandler.get(), id, address, &value, &dxl_error);
}

bool
DynamixelServoChain::Read4Byte(int id, int address, uint32_t & value, uint8_t & dxl_error)
{
    if (!ready())
    {
        Warn("Cannot read 4 bytes from " + name + " because the chain is not ready.");
        return false;
    }

    return COMM_SUCCESS == packetHandler->read4ByteTxRx(portHandler.get(), id, address, &value, &dxl_error);
}

bool
DynamixelServoChain::Write2ByteForAll(int address, uint16_t value, const std::string & description, uint8_t & dxl_error)
{
    for (int id = idMin; id <= idMax; id++)
        if (!Write2Byte(id, address, value, dxl_error))
        {
            Warn(description + " not set for " + name + " servo ID: " + std::to_string(id));
            return false;
        }
    return true;
}

bool
DynamixelServoChain::Write4ByteForAll(int address, uint32_t value, const std::string & description, uint8_t & dxl_error)
{
    for (int id = idMin; id <= idMax; id++)
        if (!Write4Byte(id, address, value, dxl_error))
        {
            Warn(description + " not set for " + name + " servo ID: " + std::to_string(id));
            return false;
        }
    return true;
}

bool
DynamixelServoChain::WriteIndirectAddressBytes(int indirectAddress, int directAddress, int byteCount, const std::string & description, uint8_t & dxl_error)
{
    for (int id = idMin; id <= idMax; id++)
        for (int byte = 0; byte < byteCount; byte++)
            if (!Write2Byte(id, indirectAddress + (2 * byte), directAddress + byte, dxl_error))
            {
                Warn(description + " not set for " + name + " servo ID: " + std::to_string(id) + ", byte: " + std::to_string(byte));
                return false;
            }

    return true;
}

bool
DynamixelServoChain::ConfigureIndirectAddresses(const dictionary & controlTable, const dictionary & indirectLayout, uint8_t & dxl_error)
{
    int torqueEnableAddress = 0;
    int goalPositionAddress = 0;
    int goalCurrentAddress = 0;
    int goalPWMAddress = 0;
    int presentPositionAddress = 0;
    int presentCurrentAddress = 0;
    int presentTemperatureAddress = 0;
    int torqueEnableIndirectAddress = 0;
    int goalPositionIndirectAddress = 0;
    int goalCurrentIndirectAddress = 0;
    int goalPWMIndirectAddress = 0;
    int presentPositionIndirectAddress = 0;
    int presentCurrentIndirectAddress = 0;
    int presentTemperatureIndirectAddress = 0;

    if (!ControlTableAddress(controlTable, "Torque Enable", 1, torqueEnableAddress) ||
        !ControlTableAddress(controlTable, "Goal Position", 4, goalPositionAddress) ||
        !ControlTableAddress(controlTable, "Goal Current", 2, goalCurrentAddress) ||
        !ControlTableAddress(controlTable, "Goal PWM", 2, goalPWMAddress) ||
        !ControlTableAddress(controlTable, "Present Position", 4, presentPositionAddress) ||
        !ControlTableAddress(controlTable, "Present Current", 2, presentCurrentAddress) ||
        !ControlTableAddress(controlTable, "Present Temperature", 1, presentTemperatureAddress))
        return false;

    if (!IndirectLayoutAddress(indirectLayout, "Torque Enable", torqueEnableIndirectAddress) ||
        !IndirectLayoutAddress(indirectLayout, "Goal Position", goalPositionIndirectAddress) ||
        !IndirectLayoutAddress(indirectLayout, "Goal Current", goalCurrentIndirectAddress) ||
        !IndirectLayoutAddress(indirectLayout, "Goal PWM", goalPWMIndirectAddress) ||
        !IndirectLayoutAddress(indirectLayout, "Present Position", presentPositionIndirectAddress) ||
        !IndirectLayoutAddress(indirectLayout, "Present Current", presentCurrentIndirectAddress) ||
        !IndirectLayoutAddress(indirectLayout, "Present Temperature", presentTemperatureIndirectAddress))
        return false;

    if (!WriteIndirectAddressBytes(torqueEnableIndirectAddress, torqueEnableAddress, 1, "Torque enable indirect address", dxl_error))
        return false;
    if (!WriteIndirectAddressBytes(goalPositionIndirectAddress, goalPositionAddress, 4, "Goal position indirect address", dxl_error))
        return false;
    if (!WriteIndirectAddressBytes(goalCurrentIndirectAddress, goalCurrentAddress, 2, "Goal current indirect address", dxl_error))
        return false;
    if (!WriteIndirectAddressBytes(goalPWMIndirectAddress, goalPWMAddress, 2, "Goal PWM indirect address", dxl_error))
        return false;
    if (!WriteIndirectAddressBytes(presentPositionIndirectAddress, presentPositionAddress, 4, "Present position indirect address", dxl_error))
        return false;
    if (!WriteIndirectAddressBytes(presentCurrentIndirectAddress, presentCurrentAddress, 2, "Present current indirect address", dxl_error))
        return false;
    if (!WriteIndirectAddressBytes(presentTemperatureIndirectAddress, presentTemperatureAddress, 1, "Present temperature indirect address", dxl_error))
        return false;

    return true;
}

bool
DynamixelServoChain::ConfigureIndirectAddresses(const dictionary & modelRegistry, const dictionary & controlTables, const dictionary & indirectLayout, uint8_t & dxl_error)
{
    auto writeIndirectAddressBytesForID = [this, &dxl_error](int id, int indirectAddress, int directAddress, int byteCount, const std::string & description)
    {
        for (int byte = 0; byte < byteCount; byte++)
            if (!Write2Byte(id, indirectAddress + (2 * byte), directAddress + byte, dxl_error))
            {
                Warn(description + " not set for " + name + " servo ID: " + std::to_string(id) + ", byte: " + std::to_string(byte));
                return false;
            }

        return true;
    };

    currentSupported.clear();

    for (int id = idMin; id <= idMax; id++)
    {
        const dictionary * controlTable = ControlTable(id, modelRegistry, controlTables);
        if (!controlTable)
        {
            Warn("No control table resolved for " + name + " servo ID: " + std::to_string(id));
            return false;
        }

        int torqueEnableAddress = 0;
        int goalPositionAddress = 0;
        int goalPWMAddress = 0;
        int presentPositionAddress = 0;
        int presentTemperatureAddress = 0;
        int torqueEnableIndirectAddress = 0;
        int goalPositionIndirectAddress = 0;
        int goalPWMIndirectAddress = 0;
        int presentPositionIndirectAddress = 0;
        int presentTemperatureIndirectAddress = 0;

        if (!ControlTableAddress(*controlTable, "Torque Enable", 1, torqueEnableAddress) ||
            !ControlTableAddress(*controlTable, "Goal Position", 4, goalPositionAddress) ||
            !ControlTableAddress(*controlTable, "Goal PWM", 2, goalPWMAddress) ||
            !ControlTableAddress(*controlTable, "Present Position", 4, presentPositionAddress) ||
            !ControlTableAddress(*controlTable, "Present Temperature", 1, presentTemperatureAddress))
            return false;

        if (!IndirectLayoutAddress(indirectLayout, "Torque Enable", torqueEnableIndirectAddress) ||
            !IndirectLayoutAddress(indirectLayout, "Goal Position", goalPositionIndirectAddress) ||
            !IndirectLayoutAddress(indirectLayout, "Goal PWM", goalPWMIndirectAddress) ||
            !IndirectLayoutAddress(indirectLayout, "Present Position", presentPositionIndirectAddress) ||
            !IndirectLayoutAddress(indirectLayout, "Present Temperature", presentTemperatureIndirectAddress))
            return false;

        if (!writeIndirectAddressBytesForID(id, torqueEnableIndirectAddress, torqueEnableAddress, 1, "Torque enable indirect address"))
            return false;
        if (!writeIndirectAddressBytesForID(id, goalPositionIndirectAddress, goalPositionAddress, 4, "Goal position indirect address"))
            return false;
        if (!writeIndirectAddressBytesForID(id, goalPWMIndirectAddress, goalPWMAddress, 2, "Goal PWM indirect address"))
            return false;
        if (!writeIndirectAddressBytesForID(id, presentPositionIndirectAddress, presentPositionAddress, 4, "Present position indirect address"))
            return false;
        if (!writeIndirectAddressBytesForID(id, presentTemperatureIndirectAddress, presentTemperatureAddress, 1, "Present temperature indirect address"))
            return false;

        const bool hasGoalCurrent = HasControlTableEntry(*controlTable, "Goal Current", 2);
        const bool hasPresentCurrent = HasControlTableEntry(*controlTable, "Present Current", 2);
        currentSupported[id] = hasGoalCurrent && hasPresentCurrent;

        if (hasGoalCurrent)
        {
            int goalCurrentAddress = 0;
            int goalCurrentIndirectAddress = 0;
            if (!ControlTableAddress(*controlTable, "Goal Current", 2, goalCurrentAddress) ||
                !IndirectLayoutAddress(indirectLayout, "Goal Current", goalCurrentIndirectAddress))
                return false;

            if (!writeIndirectAddressBytesForID(id, goalCurrentIndirectAddress, goalCurrentAddress, 2, "Goal current indirect address"))
                return false;
        }

        if (hasPresentCurrent)
        {
            int presentCurrentAddress = 0;
            int presentCurrentIndirectAddress = 0;
            if (!ControlTableAddress(*controlTable, "Present Current", 2, presentCurrentAddress) ||
                !IndirectLayoutAddress(indirectLayout, "Present Current", presentCurrentIndirectAddress))
                return false;

            if (!writeIndirectAddressBytesForID(id, presentCurrentIndirectAddress, presentCurrentAddress, 2, "Present current indirect address"))
                return false;
        }
    }

    return true;
}


bool
DynamixelServoChain::ConfigureIndirectAddressesForCommunicationMode(const dictionary & modelRegistry, const dictionary & controlTables, const dictionary & indirectLayouts, uint8_t & dxl_error)
{
    if (!UsesSyncIndirectCommunication())
        return true;

    if (!ready())
    {
        Fatal("Servo chain is not ready: " + name);
        return false;
    }

    const dictionary * indirectLayout = IndirectLayout(modelRegistry, indirectLayouts);
    if (!indirectLayout)
    {
        Warn("No indirect layout resolved for servo chain " + name + ".");
        return false;
    }

    return ConfigureIndirectAddresses(modelRegistry, controlTables, *indirectLayout, dxl_error);
}


bool
DynamixelServoChain::DisableTorque(const dictionary & controlTable, uint8_t & dxl_error)
{
    if (!ready())
    {
        Warn("Cannot disable torque for " + name + " because the chain is not ready.");
        return false;
    }

    int torqueEnableAddress = 0;
    if (!ControlTableAddress(controlTable, "Torque Enable", 1, torqueEnableAddress))
        return false;

    for (int id = idMin; id <= idMax; id++)
        if (!Write1Byte(id, torqueEnableAddress, 0, dxl_error))
        {
            Warn("Failed to disable torque for " + name + " servo ID: " + std::to_string(id));
            return false;
        }

    return true;
}

bool
DynamixelServoChain::ControlTableAddress(const dictionary & controlTable, const std::string & parameterName, int expectedBytes, int & address) const
{
    if (!controlTable.contains(parameterName) || !controlTable[parameterName].is_dictionary())
    {
        Warn("Control table for " + name + " is missing " + parameterName + ".");
        return false;
    }

    const dictionary & parameter = controlTable[parameterName].as_dictionary();
    if (!parameter.contains("Address") || !parameter.contains("Bytes"))
    {
        Warn("Control table entry " + parameterName + " for " + name + " must include Address and Bytes.");
        return false;
    }

    const int bytes = parameter["Bytes"].as_int();
    if (bytes != expectedBytes)
    {
        Warn("Control table entry " + parameterName + " for " + name +
            " has byte width " + std::to_string(bytes) +
            "; expected " + std::to_string(expectedBytes) + ".");
        return false;
    }

    address = parameter["Address"].as_int();
    return true;
}

bool
DynamixelServoChain::IndirectLayoutAddress(const dictionary & indirectLayout, const std::string & parameterName, int & address) const
{
    if (!indirectLayout.contains(parameterName))
    {
        Warn("Indirect layout for " + name + " is missing " + parameterName + ".");
        return false;
    }

    address = indirectLayout[parameterName].as_int();
    return true;
}

bool
DynamixelServoChain::IndirectLayoutOffset(const dictionary & indirectLayout, const std::string & offsetGroup, const std::string & parameterName, int byteCount, int dataLength, int & offset) const
{
    if (!indirectLayout.contains(offsetGroup) || !indirectLayout[offsetGroup].is_dictionary())
    {
        Warn("Indirect layout for " + name + " is missing " + offsetGroup + ".");
        return false;
    }

    const dictionary & offsets = indirectLayout[offsetGroup].as_dictionary();
    if (!offsets.contains(parameterName))
    {
        Warn("Indirect layout " + offsetGroup + " for " + name + " is missing " + parameterName + ".");
        return false;
    }

    offset = offsets[parameterName].as_int();
    if (offset < 0 || offset + byteCount > dataLength)
    {
        Warn("Indirect layout " + offsetGroup + " entry " + parameterName + " for " + name +
            " has offset " + std::to_string(offset) +
            " and byte width " + std::to_string(byteCount) +
            " outside data length " + std::to_string(dataLength) + ".");
        return false;
    }

    return true;
}

bool
DynamixelServoChain::ApplyProfileAndPID(const dictionary & controlTable, uint32_t profileAcceleration, uint32_t profileVelocity, uint16_t pGain, uint16_t iGain, uint16_t dGain, uint8_t & dxl_error)
{
    if (!ready())
    {
        Fatal("Servo chain is not ready: " + name);
        return false;
    }

    int profileAccelerationAddress = 0;
    int profileVelocityAddress = 0;
    int pGainAddress = 0;
    int iGainAddress = 0;
    int dGainAddress = 0;

    if (!ControlTableAddress(controlTable, "Profile Acceleration", 4, profileAccelerationAddress) ||
        !ControlTableAddress(controlTable, "Profile Velocity", 4, profileVelocityAddress) ||
        !ControlTableAddress(controlTable, "Position P Gain", 2, pGainAddress) ||
        !ControlTableAddress(controlTable, "Position I Gain", 2, iGainAddress) ||
        !ControlTableAddress(controlTable, "Position D Gain", 2, dGainAddress))
        return false;

    if (!Write4ByteForAll(profileAccelerationAddress, profileAcceleration, "Profile acceleration", dxl_error))
        return false;
    if (!Write4ByteForAll(profileVelocityAddress, profileVelocity, "Profile velocity", dxl_error))
        return false;
    if (!Write2ByteForAll(pGainAddress, pGain, "P gain", dxl_error))
        return false;
    if (!Write2ByteForAll(iGainAddress, iGain, "I gain", dxl_error))
        return false;
    if (!Write2ByteForAll(dGainAddress, dGain, "D gain", dxl_error))
        return false;

    return true;
}


bool
DynamixelServoChain::ApplyProfileAndPIDForCommunicationMode(const dictionary & controlTable, uint32_t profileAcceleration, uint32_t profileVelocity, uint16_t pGain, uint16_t iGain, uint16_t dGain, uint8_t & dxl_error)
{
    if (!UsesSyncIndirectCommunication())
        return true;

    return ApplyProfileAndPID(controlTable, profileAcceleration, profileVelocity, pGain, iGain, dGain, dxl_error);
}


bool
DynamixelServoChain::WritePositionLimit(const dictionary & controlTable, int id, uint32_t minPosition, uint32_t maxPosition, uint8_t & dxl_error)
{
    if (!ready())
    {
        Fatal("Servo chain is not ready: " + name);
        return false;
    }

    int minPositionAddress = 0;
    int maxPositionAddress = 0;
    if (!ControlTableAddress(controlTable, "Min Position Limit", 4, minPositionAddress) ||
        !ControlTableAddress(controlTable, "Max Position Limit", 4, maxPositionAddress))
        return false;

    if (!Write4Byte(id, minPositionAddress, minPosition, dxl_error))
    {
        Warn("Failed to set min position limit for servo ID: " +
            std::to_string(id) + " of port: " +
            PortName() +
            DynamixelError(*this, dxl_error));
        return false;
    }

    if (!Write4Byte(id, maxPositionAddress, maxPosition, dxl_error))
    {
        Warn("Failed to set max position limit for servo ID: " +
            std::to_string(id) + " of port: " +
            PortName() +
            DynamixelError(*this, dxl_error));
        return false;
    }

    return true;
}

bool
DynamixelServoChain::ApplyLoadedControlTableSettings(const dictionary & controlTable, const dictionary & indirectLayout, uint8_t & dxl_error)
{
    int addressRead = 0;
    if (!IndirectLayoutAddress(indirectLayout, "Present Position", addressRead))
        return false;

    for (int id = idMin; id <= idMax; id++)
    {
        Debug("Setting control table for " + name + " servo ID: " + std::to_string(id));

        for (size_t param = 0; param < parameterNames.size(); param++)
        {
            const std::string & parameterName = parameterNames[param];
            int byteLength = controlTable[parameterName]["Bytes"];
            int address = static_cast<int>(controlTable[parameterName]["Address"]);

            if (parameterName == "Present Current" || parameterName == "Present Position")
            {
                for (int byte = 0; byte < byteLength; byte++)
                {
                    if (!Write2Byte(id, addressRead, address + byte, dxl_error))
                    {
                        Warn("Failed to set indirect reading address for " + parameterName +
                            " for servo ID: " + std::to_string(id) +
                            " of port: " + PortName() +
                            DynamixelError(*this, dxl_error));
                        return false;
                    }
                    addressRead += 2;
                }
                continue;
            }

            if (byteLength == 2)
            {
                uint16_t value = parameters(row(id), static_cast<int>(param));
                if (parameterName == "Goal PWM")
                    value = parameters(row(id), static_cast<int>(param)) / DynamixelServoChainConstants::PWM_PERCENT_UNIT;

                if (!Write2Byte(id, address, value, dxl_error))
                {
                    Warn("Failed to set " + parameterName +
                        " for servo ID: " + std::to_string(id) +
                        " of port: " + PortName() +
                        DynamixelError(*this, dxl_error));
                    return false;
                }
                continue;
            }

            if (byteLength == 4)
            {
                uint32_t value = parameters(row(id), static_cast<int>(param));
                if (!Write4Byte(id, address, value, dxl_error))
                {
                    Warn("Failed to set " + parameterName +
                        " for servo ID: " + std::to_string(id) +
                        " of port: " + PortName() +
                        DynamixelError(*this, dxl_error));
                    return false;
                }
                continue;
            }

            Fatal("Unsupported byte length for parameter " + parameterName + ": " + std::to_string(byteLength));
            return false;
        }
    }

    return true;
}


bool
DynamixelServoChain::ApplyLoadedControlTableSettingsForCommunicationMode(const dictionary & modelRegistry, const dictionary & controlTables, const dictionary & indirectLayouts, uint8_t & dxl_error)
{
    if (!UsesSyncIndirectCommunication())
        return true;

    if (!ready())
    {
        Fatal("Servo chain is not ready: " + name);
        return false;
    }

    const dictionary * controlTable = ControlTable(modelRegistry, controlTables);
    if (!controlTable)
    {
        Warn("No control table resolved for servo chain " + name + ".");
        return false;
    }

    const dictionary * indirectLayout = IndirectLayout(modelRegistry, indirectLayouts);
    if (!indirectLayout)
    {
        Warn("No indirect layout resolved for servo chain " + name + ".");
        return false;
    }

    if (!DisableTorque(*controlTable, dxl_error))
        return false;

    if (!ConfigureIndirectAddresses(modelRegistry, controlTables, *indirectLayout, dxl_error))
        return false;

    return ApplyLoadedControlTableSettings(modelRegistry, controlTables, *indirectLayout, dxl_error);
}


bool
DynamixelServoChain::ApplyLoadedControlTableSettings(const dictionary & modelRegistry, const dictionary & controlTables, const dictionary & indirectLayout, uint8_t & dxl_error)
{
    for (int id = idMin; id <= idMax; id++)
    {
        const dictionary * controlTable = ControlTable(id, modelRegistry, controlTables);
        if (!controlTable)
        {
            Warn("No control table resolved for " + name + " servo ID: " + std::to_string(id));
            return false;
        }

        Debug("Setting control table for " + name + " servo ID: " + std::to_string(id));

        for (size_t param = 0; param < parameterNames.size(); param++)
        {
            const std::string & parameterName = parameterNames[param];
            if (!controlTable->contains(parameterName) || !(*controlTable)[parameterName].is_dictionary())
            {
                Debug("Skipping unsupported parameter " + parameterName + " for " + name + " servo ID: " + std::to_string(id));
                continue;
            }

            const dictionary & parameter = (*controlTable)[parameterName].as_dictionary();
            const int byteLength = parameter["Bytes"].as_int();
            const int address = parameter["Address"].as_int();

            if (parameterName == "Present Current" || parameterName == "Present Position")
            {
                int indirectAddress = 0;
                if (!IndirectLayoutAddress(indirectLayout, parameterName, indirectAddress))
                    return false;

                for (int byte = 0; byte < byteLength; byte++)
                {
                    if (!Write2Byte(id, indirectAddress + 2 * byte, address + byte, dxl_error))
                    {
                        Warn("Failed to set indirect reading address for " + parameterName +
                            " for servo ID: " + std::to_string(id) +
                            " of port: " + PortName() +
                            DynamixelError(*this, dxl_error));
                        return false;
                    }
                }
                continue;
            }

            if (byteLength == 2)
            {
                uint16_t value = parameters(row(id), static_cast<int>(param));
                if (parameterName == "Goal PWM")
                    value = parameters(row(id), static_cast<int>(param)) / DynamixelServoChainConstants::PWM_PERCENT_UNIT;

                if (!Write2Byte(id, address, value, dxl_error))
                {
                    Warn("Failed to set " + parameterName +
                        " for servo ID: " + std::to_string(id) +
                        " of port: " + PortName() +
                        DynamixelError(*this, dxl_error));
                    return false;
                }
                continue;
            }

            if (byteLength == 4)
            {
                uint32_t value = parameters(row(id), static_cast<int>(param));
                if (!Write4Byte(id, address, value, dxl_error))
                {
                    Warn("Failed to set " + parameterName +
                        " for servo ID: " + std::to_string(id) +
                        " of port: " + PortName() +
                        DynamixelError(*this, dxl_error));
                    return false;
                }
                continue;
            }

            Fatal("Unsupported byte length for parameter " + parameterName + ": " + std::to_string(byteLength));
            return false;
        }
    }

    return true;
}

bool
DynamixelServoChain::PowerOn(const dictionary & controlTable)
{
    if (!portHandler)
        return true;

    Timer t;
    const int nrOfServos = size();
    uint8_t dxl_error = 0;
    std::vector<uint16_t> startPValue(nrOfServos, 0);
    std::vector<uint32_t> presentPositionValue(nrOfServos, 0);
    int torqueEnableAddress = 0;
    int pGainAddress = 0;
    int presentPositionAddress = 0;
    int goalPositionAddress = 0;

    if (!ControlTableAddress(controlTable, "Torque Enable", 1, torqueEnableAddress) ||
        !ControlTableAddress(controlTable, "Position P Gain", 2, pGainAddress) ||
        !ControlTableAddress(controlTable, "Present Position", 4, presentPositionAddress) ||
        !ControlTableAddress(controlTable, "Goal Position", 4, goalPositionAddress))
        return false;

    for (int i = 0; i < nrOfServos; i++)
        if (!Write1Byte(idMin + i, torqueEnableAddress, 0, dxl_error))
        {
            Warn("Cannot turn off torque for servo ID: " + std::to_string(idMin + i));
            return false;
        }

    for (int i = 0; i < nrOfServos; i++)
        if (!Read2Byte(idMin + i, pGainAddress, startPValue[i], dxl_error))
        {
            Warn("Cannot read P value for servo ID: " + std::to_string(idMin + i));
            return false;
        }

    for (int i = 0; i < nrOfServos; i++)
        if (!Write2Byte(idMin + i, pGainAddress, 0, dxl_error))
        {
            Warn("Cannot set P value to 0 for servo ID: " + std::to_string(idMin + i));
            return false;
        }

    for (int i = 0; i < nrOfServos; i++)
        if (!Write1Byte(idMin + i, torqueEnableAddress, 1, dxl_error))
        {
            Warn("Cannot turn on torque for servo ID: " + std::to_string(idMin + i));
            return false;
        }

    while (t.GetTime() < DynamixelServoChainConstants::TIMER_POWER_ON)
    {
        for (int i = 0; i < nrOfServos; i++)
            if (!Read4Byte(idMin + i, presentPositionAddress, presentPositionValue[i], dxl_error))
            {
                Warn("Cannot read present position for servo ID: " + std::to_string(idMin + i));
                return false;
            }

        for (int i = 0; i < nrOfServos; i++)
            if (!Write4Byte(idMin + i, goalPositionAddress, presentPositionValue[i], dxl_error))
            {
                Warn("Cannot set goal position for servo ID: " + std::to_string(idMin + i));
                return false;
            }

        Sleep(0.01);
        for (int i = 0; i < nrOfServos; i++)
            if (!Write2Byte(idMin + i, pGainAddress, int(float(startPValue[i]) / float(DynamixelServoChainConstants::TIMER_POWER_ON) * t.GetTime()), dxl_error))
            {
                Warn("Cannot ramp up P value for servo ID: " + std::to_string(idMin + i));
                return false;
            }
    }

    for (int i = 0; i < nrOfServos; i++)
        if (!Write2Byte(idMin + i, pGainAddress, startPValue[i], dxl_error))
        {
            Warn("Cannot restore P value for servo ID: " + std::to_string(idMin + i));
            return false;
        }

    return true;
}

bool
DynamixelServoChain::PowerOff(const dictionary & controlTable)
{
    if (!portHandler)
        return true;

    const int nrOfServos = size();
    uint8_t dxl_error = 0;
    std::vector<uint16_t> startPValue(nrOfServos, 0);
    std::vector<bool> hasStartPValue(nrOfServos, false);
    int torqueEnableAddress = 0;
    int pGainAddress = 0;

    if (!ControlTableAddress(controlTable, "Torque Enable", 1, torqueEnableAddress) ||
        !ControlTableAddress(controlTable, "Position P Gain", 2, pGainAddress))
        return false;

    for (int i = 0; i < nrOfServos; i++)
        if (!Read2Byte(idMin + i, pGainAddress, startPValue[i], dxl_error))
            Warn("Cannot read P value for servo ID: " + std::to_string(idMin + i));
        else
            hasStartPValue[i] = true;

    Warn("Powering off servos. Support the robot if needed.");
    bool success = true;

    const int steps = std::max(1, static_cast<int>(
        std::ceil(DynamixelServoChainConstants::TIMER_POWER_OFF /
            DynamixelServoChainConstants::POWER_OFF_RAMP_STEP_TIME)));

    for (int step = 1; step <= steps; step++)
    {
        const double phase = static_cast<double>(step) / steps;
        for (int i = 0; i < nrOfServos; i++)
        {
            if (!hasStartPValue[i])
                continue;

            const uint16_t pValue = static_cast<uint16_t>(std::lround(startPValue[i] * (1.0 - phase)));
            if (!Write2Byte(idMin + i, pGainAddress, pValue, dxl_error))
            {
                Warn("Cannot ramp down P value for servo ID: " + std::to_string(idMin + i));
                success = false;
            }
        }
        Sleep(DynamixelServoChainConstants::TIMER_POWER_OFF / steps);
    }

    Sleep(DynamixelServoChainConstants::TIMER_POWER_OFF_EXTENDED);

    for (int i = 0; i < nrOfServos; i++)
        if (!Write1Byte(idMin + i, torqueEnableAddress, 0, dxl_error))
        {
            Warn("Cannot turn off torque for servo ID: " + std::to_string(idMin + i));
            success = false;
        }

    for (int i = 0; i < nrOfServos; i++)
    {
        if (!hasStartPValue[i])
            continue;

        if (!Write2Byte(idMin + i, pGainAddress, startPValue[i], dxl_error))
        {
            Warn("Cannot set P value for servo ID: " + std::to_string(idMin + i));
            success = false;
        }
    }

    return success;
}

bool
DynamixelServoChain::Communicate(matrix & goalPosition, matrix & goalCurrent, matrix & goalPWM, matrix & torqueEnable, matrix & presentPosition, matrix & presentCurrent, const std::string & controlMode)
{
    if (!portHandler)
        return true;
    if (!syncRead || !syncWrite)
    {
        Fatal("Cannot communicate with " + name + " before creating sync objects");
        return false;
    }
    std::vector<uint8_t> paramSyncWrite(syncWriteDataLength);

    for (int id = idMin; id <= idMax; id++)
    {
        if (!syncRead->addParam(id))
        {
            syncWrite->clearParam();
            syncRead->clearParam();
            return false;
        }
    }

    int dxl_comm_result = syncRead->txRxPacket();
    if (dxl_comm_result != COMM_SUCCESS)
    {
        Warn("GroupSyncRead failed");
        syncWrite->clearParam();
        syncRead->clearParam();
        return false;
    }

    for (int id = idMin; id <= idMax; id++)
    {
        bool dataAvailable = syncRead->isAvailable(id, syncReadDataStart, syncReadDataLength);
        if (!dataAvailable)
        {
            Warn("SyncRead data not available for ID: " + std::to_string(id) + " at indirect data addr " + std::to_string(syncReadDataStart));
            syncWrite->clearParam();
            syncRead->clearParam();
            return false;
        }
    }

    int index = ioIndex;
    for (int id = idMin; id <= idMax; id++)
    {
        int32_t dxlPresentPosition = syncRead->getData(id, syncReadDataStart + syncReadPresentPositionOffset, 4);

        presentPosition[index] = dxlPresentPosition * PositionUnitDegrees(id) * ConversionFactor(id);
        if (currentSupported.empty() || currentSupported[id])
        {
            int16_t dxlPresentCurrent = syncRead->getData(id, syncReadDataStart + syncReadPresentCurrentOffset, 2);
            presentCurrent[index] = dxlPresentCurrent * DynamixelServoChainConstants::CURRENT_UNIT;
        }
        else
            presentCurrent[index] = 0;
        index++;
    }

    index = ioIndex;
    for (int id = idMin; id <= idMax; id++)
    {
        paramSyncWrite[syncWriteTorqueEnableOffset] = torqueEnable[index];

        int value = goalPosition[index] / ConversionFactor(id) / PositionUnitDegrees(id);
        paramSyncWrite[syncWriteGoalPositionOffset] = DXL_LOBYTE(DXL_LOWORD(value));
        paramSyncWrite[syncWriteGoalPositionOffset + 1] = DXL_HIBYTE(DXL_LOWORD(value));
        paramSyncWrite[syncWriteGoalPositionOffset + 2] = DXL_LOBYTE(DXL_HIWORD(value));
        paramSyncWrite[syncWriteGoalPositionOffset + 3] = DXL_HIBYTE(DXL_HIWORD(value));

        if ((currentSupported.empty() || currentSupported[id]) && goalCurrent.connected() && controlMode == "CurrentPosition")
        {
            int valueCurrent = goalCurrent[index] / DynamixelServoChainConstants::CURRENT_UNIT;
            paramSyncWrite[syncWriteGoalCurrentOffset] = DXL_LOBYTE(valueCurrent);
            paramSyncWrite[syncWriteGoalCurrentOffset + 1] = DXL_HIBYTE(valueCurrent);
        }
        else
        {
            paramSyncWrite[syncWriteGoalCurrentOffset] = 0;
            paramSyncWrite[syncWriteGoalCurrentOffset + 1] = 0;
        }

        int valuePWM = goalPWM.connected() ? goalPWM[index] / DynamixelServoChainConstants::PWM_PERCENT_UNIT : 100 / DynamixelServoChainConstants::PWM_PERCENT_UNIT;
        paramSyncWrite[syncWriteGoalPWMOffset] = DXL_LOBYTE(valuePWM);
        paramSyncWrite[syncWriteGoalPWMOffset + 1] = DXL_HIBYTE(valuePWM);

        if (!syncWrite->addParam(id, paramSyncWrite.data()))
        {
            Warn("SyncWrite addParam failed for " + name + " servo ID: " + std::to_string(id));
            syncWrite->clearParam();
            syncRead->clearParam();
            return false;
        }

        index++;
    }

    dxl_comm_result = syncWrite->txPacket();
    if (dxl_comm_result != COMM_SUCCESS)
    {
        syncWrite->clearParam();
        syncRead->clearParam();
        Warn("SyncWrite failed for " + name);
        return false;
    }

    syncWrite->clearParam();
    syncRead->clearParam();
    return true;
}


bool
DynamixelServoChain::PowerOnForCommunicationMode(const dictionary & controlTable)
{
    if (UsesSyncIndirectCommunication())
        return PowerOn(controlTable);

    if (UsesDirectPositionCommunication())
        return SetTorque(controlTable, 1);

    Warn("Unknown communication mode " + communication + " for servo chain " + name);
    return false;
}


bool
DynamixelServoChain::PreparePowerOnRampForCommunicationMode(const dictionary & controlTable)
{
    return PreparePowerOnRampForCommunicationMode(controlTable, {});
}


bool
DynamixelServoChain::PreparePowerOnRampForCommunicationMode(const dictionary & controlTable, const std::vector<bool> & skipServo)
{
    if (!portHandler)
        return true;

    uint8_t dxl_error = 0;
    const std::string parameterName = UsesSyncIndirectCommunication() ? "Position P Gain" : "Torque Limit";
    int address = 0;
    if (!ControlTableAddress(controlTable, parameterName, 2, address))
    {
        int torqueEnableAddress = 0;
        if (!ControlTableAddress(controlTable, "Torque Enable", 1, torqueEnableAddress))
            return false;

        bool success = true;
        for (int id = idMin; id <= idMax; id++)
        {
            const int index = ioIndex + row(id);
            if (index < static_cast<int>(skipServo.size()) && skipServo[index])
                continue;

            if (!Write1Byte(id, torqueEnableAddress, 1, dxl_error))
            {
                Warn("Cannot turn on torque for " + name + " servo ID: " + std::to_string(id));
                success = false;
            }
        }
        return success;
    }

    powerOnRampTargetValue.assign(size(), 0);
    powerOnRampHasTargetValue.assign(size(), false);
    bool success = true;
    for (int i = 0; i < size(); i++)
    {
        const int index = ioIndex + i;
        if (index < static_cast<int>(skipServo.size()) && skipServo[index])
            continue;

        if (!Read2Byte(idMin + i, address, powerOnRampTargetValue[i], dxl_error))
        {
            Warn("Cannot read " + parameterName + " for " + name + " servo ID: " + std::to_string(idMin + i));
            success = false;
            continue;
        }

        powerOnRampHasTargetValue[i] = true;
        if (!Write2Byte(idMin + i, address, 0, dxl_error))
        {
            Warn("Cannot set " + parameterName + " to zero for " + name + " servo ID: " + std::to_string(idMin + i));
            success = false;
        }
    }

    int torqueEnableAddress = 0;
    if (!ControlTableAddress(controlTable, "Torque Enable", 1, torqueEnableAddress))
        return false;

    for (int id = idMin; id <= idMax; id++)
    {
        const int index = ioIndex + row(id);
        if (index < static_cast<int>(skipServo.size()) && skipServo[index])
            continue;

        if (!Write1Byte(id, torqueEnableAddress, 1, dxl_error))
        {
            Warn("Cannot turn on torque for " + name + " servo ID: " + std::to_string(id));
            success = false;
        }
    }

    return success;
}


bool
DynamixelServoChain::RampPowerOnForCommunicationMode(const dictionary & controlTable, double phase)
{
    return RampPowerOnForCommunicationMode(controlTable, phase, {});
}


bool
DynamixelServoChain::RampPowerOnForCommunicationMode(const dictionary & controlTable, double phase, const std::vector<bool> & skipServo)
{
    if (!portHandler)
        return true;

    uint8_t dxl_error = 0;
    const std::string parameterName = UsesSyncIndirectCommunication() ? "Position P Gain" : "Torque Limit";
    int address = 0;
    if (!ControlTableAddress(controlTable, parameterName, 2, address))
        return true;

    phase = clip(phase, 0.0, 1.0);
    bool success = true;
    for (int i = 0; i < size(); i++)
    {
        const int index = ioIndex + i;
        if (index < static_cast<int>(skipServo.size()) && skipServo[index])
            continue;

        if (i >= static_cast<int>(powerOnRampHasTargetValue.size()) || !powerOnRampHasTargetValue[i])
            continue;

        const uint16_t value = static_cast<uint16_t>(std::lround(powerOnRampTargetValue[i] * phase));
        if (!Write2Byte(idMin + i, address, value, dxl_error))
        {
            Warn("Cannot ramp up " + parameterName + " for " + name + " servo ID: " + std::to_string(idMin + i));
            success = false;
        }
    }

    return success;
}


bool
DynamixelServoChain::RestorePowerOnRampForCommunicationMode(const dictionary & controlTable)
{
    return RestorePowerOnRampForCommunicationMode(controlTable, {});
}


bool
DynamixelServoChain::RestorePowerOnRampForCommunicationMode(const dictionary & controlTable, const std::vector<bool> & skipServo)
{
    if (!portHandler)
        return true;

    uint8_t dxl_error = 0;
    const std::string parameterName = UsesSyncIndirectCommunication() ? "Position P Gain" : "Torque Limit";
    int address = 0;
    if (!ControlTableAddress(controlTable, parameterName, 2, address))
        return true;

    bool success = true;
    for (int i = 0; i < size(); i++)
    {
        const int index = ioIndex + i;
        if (index < static_cast<int>(skipServo.size()) && skipServo[index])
            continue;

        if (i >= static_cast<int>(powerOnRampHasTargetValue.size()) || !powerOnRampHasTargetValue[i])
            continue;

        if (!Write2Byte(idMin + i, address, powerOnRampTargetValue[i], dxl_error))
        {
            Warn("Cannot restore " + parameterName + " for " + name + " servo ID: " + std::to_string(idMin + i));
            success = false;
        }
    }

    return success;
}


bool
DynamixelServoChain::PowerOffForCommunicationMode(const dictionary & controlTable)
{
    if (UsesSyncIndirectCommunication())
        return PowerOff(controlTable);

    if (UsesDirectPositionCommunication())
        return PowerOffDirectPosition(controlTable);

    Warn("Unknown communication mode " + communication + " for servo chain " + name);
    return false;
}


bool
DynamixelServoChain::CommunicateForCommunicationMode(
    matrix & goalPosition,
    matrix & goalCurrent,
    matrix & goalPWM,
    matrix & torqueEnable,
    matrix & presentPosition,
    matrix & presentCurrent,
    const std::string & controlMode,
    const dictionary & controlTable)
{
    if (UsesSyncIndirectCommunication())
        return Communicate(goalPosition, goalCurrent, goalPWM, torqueEnable, presentPosition, presentCurrent, controlMode);

    if (UsesDirectPositionCommunication())
        return CommunicateDirectPosition(goalPosition, controlTable);

    Warn("Unknown communication mode " + communication + " for servo chain " + name);
    return false;
}


bool
DynamixelServoChain::ReadFeedbackForCommunicationMode(matrix & presentPosition, matrix & presentCurrent, const dictionary & controlTable)
{
    static_cast<void>(controlTable);

    if (!portHandler)
        return true;

    if (UsesDirectPositionCommunication())
        return true;

    if (!UsesSyncIndirectCommunication())
    {
        Warn("Unknown communication mode " + communication + " for servo chain " + name);
        return false;
    }

    if (!syncRead)
    {
        Fatal("Cannot read feedback from " + name + " before creating sync read object");
        return false;
    }

    for (int id = idMin; id <= idMax; id++)
        if (!syncRead->addParam(id))
        {
            syncRead->clearParam();
            return false;
        }

    const int dxl_comm_result = syncRead->txRxPacket();
    if (dxl_comm_result != COMM_SUCCESS)
    {
        Warn("GroupSyncRead failed");
        syncRead->clearParam();
        return false;
    }

    for (int id = idMin; id <= idMax; id++)
    {
        const bool dataAvailable = syncRead->isAvailable(id, syncReadDataStart, syncReadDataLength);
        if (!dataAvailable)
        {
            Warn("SyncRead data not available for ID: " + std::to_string(id) + " at indirect data addr " + std::to_string(syncReadDataStart));
            syncRead->clearParam();
            return false;
        }
    }

    int index = ioIndex;
    for (int id = idMin; id <= idMax; id++)
    {
        const int32_t dxlPresentPosition = syncRead->getData(id, syncReadDataStart + syncReadPresentPositionOffset, 4);

        presentPosition[index] = dxlPresentPosition * PositionUnitDegrees(id) * ConversionFactor(id);
        if (currentSupported.empty() || currentSupported[id])
        {
            const int16_t dxlPresentCurrent = syncRead->getData(id, syncReadDataStart + syncReadPresentCurrentOffset, 2);
            presentCurrent[index] = dxlPresentCurrent * DynamixelServoChainConstants::CURRENT_UNIT;
        }
        else
            presentCurrent[index] = 0;
        index++;
    }

    syncRead->clearParam();
    return true;
}


bool
DynamixelServoChain::CommunicateDirectPosition(matrix & goalPosition, const dictionary & controlTable)
{
    if (!portHandler)
        return true;

    uint8_t dxl_error = 0;
    int goalPositionAddress = 0;
    if (!ControlTableAddress(controlTable, "Goal Position", 2, goalPositionAddress))
        return false;

    int index = ioIndex;
    for (int id = idMin; id <= idMax; id++)
    {
        uint16_t value = goalPosition[index];
        if (!Write2Byte(id, goalPositionAddress, value, dxl_error))
        {
            Warn("[ID:" + std::to_string(id) + "] write2ByteTxRx failed");
            portHandler->clearPort();
            return false;
        }
        index++;
    }

    return true;
}


bool
DynamixelServoChain::WriteGoal(matrix & goalPosition, matrix & goalPWM, matrix & torqueEnable)
{
    if (!portHandler)
        return true;
    if (!syncWrite)
    {
        Fatal("Cannot write goals to " + name + " before creating sync objects");
        return false;
    }

    std::vector<uint8_t> paramSyncWrite(syncWriteDataLength);
    int index = ioIndex;
    for (int id = idMin; id <= idMax; id++)
    {
        paramSyncWrite[syncWriteTorqueEnableOffset] = torqueEnable[index];

        int value = goalPosition[index] / ConversionFactor(id) / PositionUnitDegrees(id);
        paramSyncWrite[syncWriteGoalPositionOffset] = DXL_LOBYTE(DXL_LOWORD(value));
        paramSyncWrite[syncWriteGoalPositionOffset + 1] = DXL_HIBYTE(DXL_LOWORD(value));
        paramSyncWrite[syncWriteGoalPositionOffset + 2] = DXL_LOBYTE(DXL_HIWORD(value));
        paramSyncWrite[syncWriteGoalPositionOffset + 3] = DXL_HIBYTE(DXL_HIWORD(value));

        paramSyncWrite[syncWriteGoalCurrentOffset] = 0;
        paramSyncWrite[syncWriteGoalCurrentOffset + 1] = 0;

        int valuePWM = goalPWM.connected() ? goalPWM[index] / DynamixelServoChainConstants::PWM_PERCENT_UNIT : 100 / DynamixelServoChainConstants::PWM_PERCENT_UNIT;
        paramSyncWrite[syncWriteGoalPWMOffset] = DXL_LOBYTE(valuePWM);
        paramSyncWrite[syncWriteGoalPWMOffset + 1] = DXL_HIBYTE(valuePWM);

        if (!syncWrite->addParam(id, paramSyncWrite.data()))
        {
            Warn("SyncWrite addParam failed for " + name + " servo ID: " + std::to_string(id));
            syncWrite->clearParam();
            return false;
        }
        index++;
    }

    const int dxl_comm_result = syncWrite->txPacket();
    syncWrite->clearParam();
    if (dxl_comm_result != COMM_SUCCESS)
    {
        Warn("GroupSyncWrite failed for " + name);
        return false;
    }

    return true;
}


bool
DynamixelServoChain::WriteGoalForCommunicationMode(matrix & goalPosition, matrix & goalPWM, matrix & torqueEnable, const dictionary & controlTable)
{
    if (UsesSyncIndirectCommunication())
        return WriteGoal(goalPosition, goalPWM, torqueEnable);

    if (UsesDirectPositionCommunication())
        return CommunicateDirectPosition(goalPosition, controlTable);

    Warn("Unknown communication mode " + communication + " for servo chain " + name);
    return false;
}


bool
DynamixelServoChain::SetTorque(const dictionary & controlTable, uint8_t enabled)
{
    if (!ready())
    {
        Warn("Cannot set torque for " + name + " because the chain is not ready.");
        return false;
    }

    uint8_t dxl_error = 0;
    int torqueEnableAddress = 0;
    if (!ControlTableAddress(controlTable, "Torque Enable", 1, torqueEnableAddress))
        return false;

    for (int id = idMin; id <= idMax; id++)
        if (!Write1Byte(id, torqueEnableAddress, enabled, dxl_error))
        {
            Warn("Cannot set torque for " + name + " servo ID: " + std::to_string(id));
            return false;
        }

    return true;
}


bool
DynamixelServoChain::PowerOffDirectPosition(const dictionary & controlTable)
{
    if (!portHandler)
        return true;

    const int nrOfServos = size();
    uint8_t dxl_error = 0;
    std::vector<uint16_t> startTorqueLimit(nrOfServos, 0);
    std::vector<bool> hasStartTorqueLimit(nrOfServos, false);
    int torqueLimitAddress = 0;

    if (!ControlTableAddress(controlTable, "Torque Limit", 2, torqueLimitAddress))
        return SetTorque(controlTable, 0);

    for (int i = 0; i < nrOfServos; i++)
        if (!Read2Byte(idMin + i, torqueLimitAddress, startTorqueLimit[i], dxl_error))
            Warn("Cannot read torque limit for " + name + " servo ID: " + std::to_string(idMin + i));
        else
            hasStartTorqueLimit[i] = true;

    bool success = true;

    const int steps = std::max(1, static_cast<int>(
        std::ceil(DynamixelServoChainConstants::TIMER_POWER_OFF /
            DynamixelServoChainConstants::POWER_OFF_RAMP_STEP_TIME)));

    for (int step = 1; step <= steps; step++)
    {
        const double phase = static_cast<double>(step) / steps;
        for (int i = 0; i < nrOfServos; i++)
        {
            if (!hasStartTorqueLimit[i])
                continue;

            const uint16_t torqueLimit = static_cast<uint16_t>(std::lround(startTorqueLimit[i] * (1.0 - phase)));
            if (!Write2Byte(idMin + i, torqueLimitAddress, torqueLimit, dxl_error))
            {
                Warn("Cannot ramp down torque limit for " + name + " servo ID: " + std::to_string(idMin + i));
                success = false;
            }
        }
        Sleep(DynamixelServoChainConstants::TIMER_POWER_OFF / steps);
    }

    if (!SetTorque(controlTable, 0))
        success = false;

    for (int i = 0; i < nrOfServos; i++)
    {
        if (!hasStartTorqueLimit[i])
            continue;

        if (!Write2Byte(idMin + i, torqueLimitAddress, startTorqueLimit[i], dxl_error))
        {
            Warn("Cannot restore torque limit for " + name + " servo ID: " + std::to_string(idMin + i));
            success = false;
        }
    }

    return success;
}


bool
DynamixelServoChain::EnableTorqueForCommunicationMode(const dictionary & controlTable)
{
    return SetTorque(controlTable, 1);
}


bool
DynamixelServoChain::PreparePowerOffRampForCommunicationMode(const dictionary & controlTable)
{
    if (!portHandler)
        return true;

    uint8_t dxl_error = 0;
    const std::string parameterName = UsesSyncIndirectCommunication() ? "Position P Gain" : "Torque Limit";
    int address = 0;
    if (!ControlTableAddress(controlTable, parameterName, 2, address))
        return UsesDirectPositionCommunication();

    powerOffRampStartValue.assign(size(), 0);
    powerOffRampHasStartValue.assign(size(), false);
    bool success = true;
    for (int i = 0; i < size(); i++)
        if (!Read2Byte(idMin + i, address, powerOffRampStartValue[i], dxl_error))
        {
            Warn("Cannot read " + parameterName + " for " + name + " servo ID: " + std::to_string(idMin + i));
            success = false;
        }
        else
            powerOffRampHasStartValue[i] = true;

    return success;
}


bool
DynamixelServoChain::RampPowerOffForCommunicationMode(const dictionary & controlTable, double phase)
{
    if (!portHandler)
        return true;

    uint8_t dxl_error = 0;
    const std::string parameterName = UsesSyncIndirectCommunication() ? "Position P Gain" : "Torque Limit";
    int address = 0;
    if (!ControlTableAddress(controlTable, parameterName, 2, address))
        return UsesDirectPositionCommunication();

    bool success = true;
    for (int i = 0; i < size(); i++)
    {
        if (i >= static_cast<int>(powerOffRampHasStartValue.size()) || !powerOffRampHasStartValue[i])
            continue;

        const uint16_t value = static_cast<uint16_t>(std::lround(powerOffRampStartValue[i] * (1.0 - phase)));
        if (!Write2Byte(idMin + i, address, value, dxl_error))
        {
            Warn("Cannot ramp down " + parameterName + " for " + name + " servo ID: " + std::to_string(idMin + i));
            success = false;
        }
    }

    return success;
}


bool
DynamixelServoChain::DisableTorqueForCommunicationMode(const dictionary & controlTable)
{
    return SetTorque(controlTable, 0);
}


bool
DynamixelServoChain::RestorePowerOffRampForCommunicationMode(const dictionary & controlTable)
{
    if (!portHandler)
        return true;

    uint8_t dxl_error = 0;
    const std::string parameterName = UsesSyncIndirectCommunication() ? "Position P Gain" : "Torque Limit";
    int address = 0;
    if (!ControlTableAddress(controlTable, parameterName, 2, address))
        return true;

    bool success = true;
    for (int i = 0; i < size(); i++)
    {
        if (i >= static_cast<int>(powerOffRampHasStartValue.size()) || !powerOffRampHasStartValue[i])
            continue;

        if (!Write2Byte(idMin + i, address, powerOffRampStartValue[i], dxl_error))
        {
            Warn("Cannot restore " + parameterName + " for " + name + " servo ID: " + std::to_string(idMin + i));
            success = false;
        }
    }

    return success;
}


bool
DynamixelServoChain::ApplyStartupForCommunicationMode(const dictionary & controlTable)
{
    if (UsesDirectPositionCommunication())
        return ApplyDirectPositionStartup(controlTable);

    return true;
}


bool
DynamixelServoChain::DetectRangeForCommunicationMode(const dictionary & controlTable)
{
    if (UsesDirectPositionCommunication())
        return DetectRangeDirectPosition(controlTable);

    return true;
}


bool
DynamixelServoChain::ApplyDirectPositionStartup(const dictionary & controlTable)
{
    uint8_t dxl_error = 0;
    int cwAngleLimitAddress = 0;
    int ccwAngleLimitAddress = 0;

    if (!ControlTableAddress(controlTable, "CW Angle Limit", 2, cwAngleLimitAddress) ||
        !ControlTableAddress(controlTable, "CCW Angle Limit", 2, ccwAngleLimitAddress))
        return false;

    for (int id = idMin; id <= idMax; id++)
    {
        if (!Write2Byte(id, cwAngleLimitAddress, MapValue(positionMin, id), dxl_error))
            return false;
        if (!Write2Byte(id, ccwAngleLimitAddress, MapValue(positionMax, id), dxl_error))
            return false;

        for (const auto & startupWrite : startupWrites)
        {
            if (!controlTable.contains(startupWrite.first) || !controlTable[startupWrite.first].is_dictionary())
            {
                Warn("Control table for " + name + " is missing " + startupWrite.first + ".");
                return false;
            }

            const dictionary & parameter = controlTable[startupWrite.first].as_dictionary();
            const int address = parameter["Address"].as_int();
            const int bytes = parameter["Bytes"].as_int();

            if (bytes == 1)
            {
                if (!Write1Byte(id, address, static_cast<uint8_t>(startupWrite.second), dxl_error))
                    return false;
            }
            else if (bytes == 2)
            {
                if (!Write2Byte(id, address, static_cast<uint16_t>(startupWrite.second), dxl_error))
                    return false;
            }
            else
            {
                Warn("Startup write " + startupWrite.first + " for " + name + " must be 1 or 2 bytes.");
                return false;
            }
        }
    }

    return true;
}

bool
DynamixelServoChain::DetectRangeDirectPosition(const dictionary & controlTable)
{
    if (!BeginDetectRangeDirectPosition(controlTable))
        return false;

    Sleep(detectRangeMoveDelay);

    return FinishDetectRangeDirectPosition(controlTable);
}


bool
DynamixelServoChain::BeginDetectRangeDirectPosition(const dictionary & controlTable)
{
    uint8_t dxl_error = 0;
    int cwAngleLimitAddress = 0;
    int ccwAngleLimitAddress = 0;
    int movingSpeedAddress = 0;
    int torqueLimitAddress = 0;
    int goalPositionAddress = 0;

    if (!ControlTableAddress(controlTable, "CW Angle Limit", 2, cwAngleLimitAddress) ||
        !ControlTableAddress(controlTable, "CCW Angle Limit", 2, ccwAngleLimitAddress) ||
        !ControlTableAddress(controlTable, "Moving Speed", 2, movingSpeedAddress) ||
        !ControlTableAddress(controlTable, "Torque Limit", 2, torqueLimitAddress) ||
        !ControlTableAddress(controlTable, "Goal Position", 2, goalPositionAddress))
        return false;

    if (!SetTorque(controlTable, 0))
        return false;

    for (int id = idMin; id <= idMax; id++)
    {
        if (!Write2Byte(id, cwAngleLimitAddress, 0, dxl_error))
            return false;
        if (!Write2Byte(id, ccwAngleLimitAddress, detectRangeFullRangePosition, dxl_error))
            return false;
        if (!Write2Byte(id, movingSpeedAddress, detectRangeMovingSpeed, dxl_error))
            return false;
        if (!Write2Byte(id, torqueLimitAddress, detectRangeTorqueLimit, dxl_error))
            return false;
    }

    if (!SetTorque(controlTable, 1))
        return false;

    for (int id = idMin; id <= idMax; id++)
        if (!Write2Byte(id, goalPositionAddress, detectRangeFirstGoalPosition, dxl_error))
            return false;

    return true;
}


bool
DynamixelServoChain::FinishDetectRangeDirectPosition(const dictionary & controlTable)
{
    uint8_t dxl_error = 0;
    int torqueLimitAddress = 0;
    int presentPositionAddress = 0;

    if (!ControlTableAddress(controlTable, "Torque Limit", 2, torqueLimitAddress) ||
        !ControlTableAddress(controlTable, "Present Position", 2, presentPositionAddress))
        return false;

    for (int id = idMin; id <= idMax; id++)
    {
        uint16_t presentPosition = 0;
        if (!Read2Byte(id, presentPositionAddress, presentPosition, dxl_error))
            return false;

        detectRangeFirstEndpointPosition[id] = presentPosition;
    }

    int goalPositionAddress = 0;
    if (!ControlTableAddress(controlTable, "Goal Position", 2, goalPositionAddress))
        return false;

    for (int id = idMin; id <= idMax; id++)
    {
        const auto firstEndpoint = detectRangeFirstEndpointPosition.find(id);
        const uint16_t firstPosition = firstEndpoint == detectRangeFirstEndpointPosition.end() ?
            detectRangeFirstGoalPosition : firstEndpoint->second;
        const int secondGoal = std::min(
            detectRangeFullRangePosition,
            static_cast<int>(firstPosition) + detectRangePositionMaxOffset + 20);
        if (!Write2Byte(id, goalPositionAddress, secondGoal, dxl_error))
            return false;
    }

    Sleep(detectRangeMoveDelay);

    for (int id = idMin; id <= idMax; id++)
    {
        uint16_t presentPosition = 0;
        if (!Read2Byte(id, presentPositionAddress, presentPosition, dxl_error))
            return false;

        const auto firstEndpoint = detectRangeFirstEndpointPosition.find(id);
        if (firstEndpoint == detectRangeFirstEndpointPosition.end())
            return false;

        const double rawMin = std::min(firstEndpoint->second, presentPosition);
        const double rawMax = std::max(firstEndpoint->second, presentPosition);
        const double detectedMin = rawMin;
        const double workingMin = detectedMin + detectRangePositionMinOffset;
        const double workingMax = detectedMin + detectRangePositionMaxOffset;
        if (workingMax >= rawMax)
        {
            Debug("Detect range for " + name + " servo ID " + std::to_string(id) +
                " did not confirm the full configured range.");
        }

        positionMin[id] = workingMin;
        positionMax[id] = workingMax;
    }

    Debug("Position limits chain " + name + " (detect_range)");

    if (!SetTorque(controlTable, 0))
        return false;

    for (int id = idMin; id <= idMax; id++)
    {
        if (!Write2Byte(id, torqueLimitAddress, detectRangeMaxTorqueLimit, dxl_error))
            return false;
    }

    if (!ApplyDirectPositionStartup(controlTable))
        return false;

    return true;
}


void
DynamixelServoChain::PrepareFeedbackForCommunicationMode(matrix & goalPosition, matrix & presentPosition)
{
    if (!UsesDirectPositionCommunication())
        return;

    for (int id = idMin; id <= idMax; id++)
    {
        const int index = ioIndex + row(id);
        goalPosition[index] = clip(goalPosition[index], MapValue(softwareMin, id), MapValue(softwareMax, id));
        presentPosition[index] = goalPosition[index];
    }
}


void
DynamixelServoChain::ConvertGoalsForCommunicationMode(matrix & goalPosition)
{
    if (!UsesDirectPositionCommunication())
        return;

    for (int id = idMin; id <= idMax; id++)
    {
        const int index = ioIndex + row(id);
        const double minInput = MapValue(inputMin, id);
        const double maxInput = MapValue(inputMax, id);
        const double minPosition = MapValue(positionMin, id);
        const double maxPosition = MapValue(positionMax, id);
        goalPosition[index] = (-(goalPosition[index] - minInput) / (maxInput - minInput) * (maxPosition - minPosition) + maxPosition);
    }
}
