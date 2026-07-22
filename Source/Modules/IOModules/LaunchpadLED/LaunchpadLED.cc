#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "ikaros.h"
#include "LaunchpadProtocol.h"

#ifdef MAC_OS_X
#include <CoreFoundation/CoreFoundation.h>
#include <CoreMIDI/CoreMIDI.h>
#endif

using namespace ikaros;

#ifdef MAC_OS_X
namespace
{
    class LaunchpadMIDIService
    {
    public:
        static MIDIClientRef client()
        {
            std::lock_guard<std::mutex> lock(clientMutex_);
            if(client_ != 0)
                return client_;

            const OSStatus status = MIDIClientCreate(
                CFSTR("Ikaros Launchpad"),
                &LaunchpadMIDIService::TopologyChanged,
                nullptr,
                &client_);
            if(status != noErr)
                throw exception(
                    "LaunchpadLED could not create a CoreMIDI client "
                    "(OSStatus " + std::to_string(status) + ").");

            topologyGeneration_.fetch_add(1, std::memory_order_release);
            return client_;
        }


        static std::uint64_t topologyGeneration() noexcept
        {
            return topologyGeneration_.load(std::memory_order_acquire);
        }

    private:
        static void TopologyChanged(const MIDINotification *, void *)
        {
            topologyGeneration_.fetch_add(1, std::memory_order_release);
        }

        inline static std::mutex clientMutex_;
        inline static MIDIClientRef client_ = 0;
        inline static std::atomic<std::uint64_t> topologyGeneration_{0};
    };


    std::string
    CFStringToStdString(CFStringRef value)
    {
        if(value == nullptr)
            return "";

        const char * direct =
            CFStringGetCStringPtr(value, kCFStringEncodingUTF8);
        if(direct != nullptr)
            return direct;

        const CFIndex length = CFStringGetLength(value);
        const CFIndex capacity =
            CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
        std::string result(static_cast<std::size_t>(capacity), '\0');
        if(!CFStringGetCString(value, result.data(), capacity,
                              kCFStringEncodingUTF8))
            return "";

        result.resize(std::strlen(result.c_str()));
        return result;
    }


    std::string
    MIDIObjectName(MIDIObjectRef object, CFStringRef property)
    {
        if(object == 0)
            return "";

        CFStringRef name = nullptr;
        const OSStatus status =
            MIDIObjectGetStringProperty(object, property, &name);
        if(status != noErr || name == nullptr)
            return "";

        const std::string result = CFStringToStdString(name);
        CFRelease(name);
        return result;
    }


    std::string
    DestinationName(MIDIEndpointRef destination)
    {
        std::string name =
            MIDIObjectName(destination, kMIDIPropertyDisplayName);
        if(name.empty())
            name = MIDIObjectName(destination, kMIDIPropertyName);
        return name.empty() ? "Unknown" : name;
    }


    bool
    NameMatches(MIDIEndpointRef destination, const std::string & requested)
    {
        const std::string name =
            MIDIObjectName(destination, kMIDIPropertyName);
        const std::string displayName =
            MIDIObjectName(destination, kMIDIPropertyDisplayName);
        return name == requested || displayName == requested ||
               (!requested.empty() &&
                (name.find(requested) != std::string::npos ||
                 displayName.find(requested) != std::string::npos));
    }
}
#endif


class LaunchpadLED: public Module
{
public:
    ~LaunchpadLED() override
    {
        Stop();
    }


    void
    Stop() override
    {
#ifdef MAC_OS_X
        StopPort(true);
#endif
    }


    void
    Init() override
    {
        Bind(destinationName_, "destination_name");
        Bind(layoutWidth_, "layout_width");
        Bind(idleBrightness_, "idle_brightness");
        Bind(playingBrightness_, "playing_brightness");
        Bind(programmerMode_, "programmer_mode");
        Bind(clearOnStop_, "clear_on_stop");
        Bind(restoreLiveMode_, "restore_live_mode");
        Bind(color_, "COLOR");
        Bind(playing_, "PLAYING");
        Bind(connected_, "CONNECTED");
        Bind(destinationCount_, "DESTINATION_COUNT");
        Bind(lastUpdateCount_, "LAST_UPDATE_COUNT");

        ValidateConfiguration();
        destinationNameValue_ = destinationName_.as_string();
        layoutWidthValue_ = IntegerParameter(layoutWidth_, "layout_width");
        programmerModeValue_ = programmerMode_.as_bool();
        clearOnStopValue_ = clearOnStop_.as_bool();
        restoreLiveModeValue_ = restoreLiveMode_.as_bool();

        connected_ = 0;
        destinationCount_ = 0;
        lastUpdateCount_ = 0;
        cachedColors_.fill({});
        desiredColors_.fill({});
        pendingUpdates_.reserve(launchpad::padCount);
        midiMessage_.reserve(8 + 5 * launchpad::padCount);

#ifndef MAC_OS_X
        throw exception("LaunchpadLED is only available on macOS.");
#else
        StartPort();
#endif
    }


    void
    Tick() override
    {
        lastUpdateCount_ = 0;

#ifdef MAC_OS_X
        MaintainPort();
        if(destination_ == 0)
            return;

        BuildDesiredColors();
        pendingUpdates_.clear();
        for(std::size_t cell = 0; cell < launchpad::padCount; ++cell)
        {
            if(cacheValid_ && cachedColors_[cell] == desiredColors_[cell])
                continue;

            std::uint8_t ledIndex = 0;
            const bool valid =
                launchpad::padLEDIndex(cell, launchpad::padColumns, ledIndex);
            if(!valid)
                continue;
            pendingUpdates_.push_back({ledIndex, desiredColors_[cell]});
        }

        if(pendingUpdates_.empty())
            return;

        launchpad::buildRGBMessage(pendingUpdates_, midiMessage_);
        const OSStatus status = SendMessage(midiMessage_);
        if(status != noErr)
        {
            HandleSendFailure(status);
            return;
        }

        cachedColors_ = desiredColors_;
        cacheValid_ = true;
        lastUpdateCount_ = static_cast<float>(pendingUpdates_.size());
        runtimeWarningReported_ = false;
#endif
    }

private:
    static int
    IntegerParameter(const parameter & value, const std::string & name)
    {
        const double numericValue = value.as_double();
        const int integerValue = value.as_int();
        if(numericValue != static_cast<double>(integerValue))
            throw exception("LaunchpadLED parameter \"" + name +
                            "\" must be an integer.");
        return integerValue;
    }


    void
    ValidateConfiguration() const
    {
        if(color_.rank() != 2 || color_.cols() != 3)
            throw exception(
                "LaunchpadLED COLOR must have one RGB row per sequence.");

        const int width = IntegerParameter(layoutWidth_, "layout_width");
        if(width < 1 || width > static_cast<int>(launchpad::padColumns))
            throw exception("LaunchpadLED layout_width must be between 1 and 8.");
        if(color_.rows() > width * static_cast<int>(launchpad::padRows))
            throw exception(
                "LaunchpadLED COLOR contains more sequences than fit in the "
                "configured pad layout.");

        if(playing_.connected() && playing_.size() != color_.rows())
            throw exception(
                "LaunchpadLED PLAYING must contain one value per COLOR row.");
        if(connected_.size() != 1 || destinationCount_.size() != 1 ||
           lastUpdateCount_.size() != 1)
            throw exception(
                "LaunchpadLED status outputs must each contain one value.");
    }


    void
    BuildDesiredColors()
    {
        desiredColors_.fill({});
        const float idleBrightness =
            static_cast<float>(idleBrightness_.as_double());
        const float playingBrightness =
            static_cast<float>(playingBrightness_.as_double());
        const bool hasPlaying = playing_.connected();

        for(int sequence = 0; sequence < color_.rows(); ++sequence)
        {
            const std::size_t row =
                static_cast<std::size_t>(sequence) /
                static_cast<std::size_t>(layoutWidthValue_);
            const std::size_t column =
                static_cast<std::size_t>(sequence) %
                static_cast<std::size_t>(layoutWidthValue_);
            const std::size_t cell = row * launchpad::padColumns + column;
            const bool active = hasPlaying && playing_(sequence) > 0;
            const float brightness = hasPlaying
                ? (active ? playingBrightness : idleBrightness)
                : playingBrightness;
            desiredColors_[cell] = {
                launchpad::colorComponent(color_(sequence, 0), brightness),
                launchpad::colorComponent(color_(sequence, 1), brightness),
                launchpad::colorComponent(color_(sequence, 2), brightness),
            };
        }
    }


    parameter destinationName_;
    parameter layoutWidth_;
    parameter idleBrightness_;
    parameter playingBrightness_;
    parameter programmerMode_;
    parameter clearOnStop_;
    parameter restoreLiveMode_;
    matrix color_;
    matrix playing_;
    matrix connected_;
    matrix destinationCount_;
    matrix lastUpdateCount_;

    std::string destinationNameValue_;
    int layoutWidthValue_ = 8;
    bool programmerModeValue_ = true;
    bool clearOnStopValue_ = true;
    bool restoreLiveModeValue_ = true;
    std::array<launchpad::RGBColor, launchpad::padCount> cachedColors_{};
    std::array<launchpad::RGBColor, launchpad::padCount> desiredColors_{};
    std::vector<launchpad::LEDUpdate> pendingUpdates_;
    std::vector<std::uint8_t> midiMessage_;
    bool cacheValid_ = false;

#ifdef MAC_OS_X
    MIDIPortRef outputPort_ = 0;
    MIDIEndpointRef destination_ = 0;
    std::uint64_t topologyGeneration_ = 0;
    bool runtimeWarningReported_ = false;


    void
    StartPort()
    {
        const MIDIClientRef client = LaunchpadMIDIService::client();
        const std::uint64_t observedGeneration =
            LaunchpadMIDIService::topologyGeneration();
        const OSStatus status =
            MIDIOutputPortCreate(client, CFSTR("Launchpad LED Output"),
                                 &outputPort_);
        if(status != noErr)
            throw exception(
                "LaunchpadLED could not create a CoreMIDI output port "
                "(OSStatus " + std::to_string(status) + ").");

        RefreshDestinations();
        topologyGeneration_ = observedGeneration;
    }


    void
    StopPort(bool restoreDevice) noexcept
    {
        if(destination_ != 0 && outputPort_ != 0 && restoreDevice)
        {
            if(clearOnStopValue_)
                SendClear();
            if(programmerModeValue_ && restoreLiveModeValue_)
                SendProgrammerMode(false);
        }

        if(outputPort_ != 0)
        {
            MIDIPortDispose(outputPort_);
            outputPort_ = 0;
        }
        destination_ = 0;
        connected_ = 0;
        cacheValid_ = false;
    }


    void
    MaintainPort()
    {
        try
        {
            if(outputPort_ == 0)
            {
                StartPort();
                return;
            }

            const std::uint64_t currentGeneration =
                LaunchpadMIDIService::topologyGeneration();
            if(currentGeneration != topologyGeneration_)
            {
                StopPort(false);
                StartPort();
            }
        }
        catch(const std::exception & error)
        {
            StopPort(false);
            if(!runtimeWarningReported_)
            {
                Notify(msg_warning,
                       "LaunchpadLED could not restart CoreMIDI: " +
                           std::string(error.what()));
                runtimeWarningReported_ = true;
            }
        }
    }


    void
    RefreshDestinations()
    {
        const ItemCount count = MIDIGetNumberOfDestinations();
        destinationCount_ = static_cast<float>(count);

        std::ostringstream available;
        available << "LaunchpadLED found " << count << " MIDI destination";
        if(count != 1)
            available << "s";

        MIDIEndpointRef match = 0;
        int matchIndex = -1;
        for(ItemCount index = 0; index < count; ++index)
        {
            const MIDIEndpointRef candidate = MIDIGetDestination(index);
            available << " [" << index << ": "
                      << DestinationName(candidate) << "]";
            if(match == 0 &&
               NameMatches(candidate, destinationNameValue_))
            {
                match = candidate;
                matchIndex = static_cast<int>(index);
            }
        }
        Notify(msg_print, available.str());

        if(match == 0)
        {
            destination_ = 0;
            connected_ = 0;
            Notify(msg_warning,
                   "LaunchpadLED could not find MIDI destination \"" +
                       destinationNameValue_ +
                       "\" and will retry when the MIDI configuration changes.");
            return;
        }

        destination_ = match;
        connected_ = 1;
        cacheValid_ = false;
        Notify(msg_print,
               "LaunchpadLED connected to destination " +
                   std::to_string(matchIndex) + ": " +
                   DestinationName(destination_));

        if(programmerModeValue_)
        {
            const OSStatus status = SendProgrammerMode(true);
            if(status != noErr)
                HandleSendFailure(status);
        }
    }


    OSStatus
    SendMessage(const std::vector<std::uint8_t> & message) const noexcept
    {
        if(outputPort_ == 0 || destination_ == 0 || message.empty())
            return kMIDIInvalidPort;

        alignas(MIDIPacketList) std::array<std::byte, 1024> storage{};
        auto * packetList =
            reinterpret_cast<MIDIPacketList *>(storage.data());
        MIDIPacket * packet = MIDIPacketListInit(packetList);
        packet = MIDIPacketListAdd(
            packetList,
            storage.size(),
            packet,
            0,
            static_cast<UInt16>(message.size()),
            message.data());
        if(packet == nullptr)
            return kMIDIMessageSendErr;

        return MIDISend(outputPort_, destination_, packetList);
    }


    OSStatus
    SendProgrammerMode(bool enabled) noexcept
    {
        launchpad::buildProgrammerModeMessage(enabled, midiMessage_);
        return SendMessage(midiMessage_);
    }


    void
    SendClear() noexcept
    {
        pendingUpdates_.clear();
        for(std::size_t cell = 0; cell < launchpad::padCount; ++cell)
        {
            std::uint8_t ledIndex = 0;
            if(launchpad::padLEDIndex(cell, launchpad::padColumns, ledIndex))
                pendingUpdates_.push_back({ledIndex, {}});
        }
        launchpad::buildRGBMessage(pendingUpdates_, midiMessage_);
        SendMessage(midiMessage_);
    }


    void
    HandleSendFailure(OSStatus status)
    {
        connected_ = 0;
        cacheValid_ = false;
        if(!runtimeWarningReported_)
        {
            Notify(msg_warning,
                   "LaunchpadLED could not send MIDI data (OSStatus " +
                       std::to_string(status) + ").");
            runtimeWarningReported_ = true;
        }
        StopPort(false);
    }
#endif
};

INSTALL_CLASS(LaunchpadLED)
