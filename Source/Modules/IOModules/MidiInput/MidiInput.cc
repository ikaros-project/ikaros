#include <atomic>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "ikaros.h"
#include "MidiProtocol.h"

#ifdef MAC_OS_X
#include <CoreFoundation/CoreFoundation.h>
#include <CoreMIDI/CoreMIDI.h>
#endif

using namespace ikaros;

#ifdef MAC_OS_X
namespace
{
    class CoreMIDIService
    {
    public:
        static MIDIClientRef client()
        {
            std::lock_guard<std::mutex> lock(clientMutex_);
            if(client_ != 0)
                return client_;

            const OSStatus status = MIDIClientCreate(
                CFSTR("Ikaros"), &CoreMIDIService::TopologyChanged, nullptr,
                &client_);
            if(status != noErr)
                throw exception("MidiInput could not create a CoreMIDI client (OSStatus " +
                                std::to_string(status) + ").");

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
}
#endif


class MidiInput: public Module
{
public:
    ~MidiInput() override
    {
        Stop();
    }


    void
    Stop() override
    {
#ifdef MAC_OS_X
        StopPort();
#endif
    }


    void
    Init() override
    {
        Bind(sourceIndex_, "source_index");
        Bind(trigHoldTicks_, "trig_hold_ticks");
        Bind(key_, "KEY");
        Bind(gate_, "GATE");
        Bind(trig_, "TRIG");
        Bind(eventCountOut_, "EVENT_COUNT");
        Bind(sourceCountOut_, "SOURCE_COUNT");
        Bind(connectedOut_, "CONNECTED");
        Bind(lastBatchCountOut_, "LAST_BATCH_COUNT");

        ValidateOutputs();
        sourceIndexValue_ = IntegerParameter(sourceIndex_, "source_index");
        trigHoldTicksValue_ = IntegerParameter(trigHoldTicks_, "trig_hold_ticks");

        key_ = 0;
        gate_ = 0;
        trig_ = 0;
        eventCountOut_ = 0;
        sourceCountOut_ = 0;
        connectedOut_ = 0;
        lastBatchCountOut_ = 0;

#ifndef MAC_OS_X
        throw exception("MidiInput is only available on macOS.");
#else
        StartPort();
#endif
    }


    void
    Tick() override
    {
#ifdef MAC_OS_X
        MaintainPort();
#endif

        if(!eventState_)
        {
            gate_ = 0;
            trig_ = 0;
            lastBatchCountOut_ = 0;
            return;
        }

        const midi::EventBatch batch = eventState_->consume();
        if(batch.count != 0)
        {
            key_ = batch.latestNote;
            if(batch.count > std::numeric_limits<std::uint64_t>::max() - eventCount_)
                eventCount_ = std::numeric_limits<std::uint64_t>::max();
            else
                eventCount_ += batch.count;
            eventCountOut_ = static_cast<float>(eventCount_);
            lastBatchCountOut_ = static_cast<float>(batch.count);
            trigCountdown_ = trigHoldTicksValue_;
        }
        else
            lastBatchCountOut_ = 0;

        gate_ = eventState_->anyNoteHeld() ? 1.0f : 0.0f;
        trig_ = trigCountdown_ > 0 ? 1.0f : 0.0f;
        if(trigCountdown_ > 0)
            --trigCountdown_;
    }

private:
    static int
    IntegerParameter(const parameter & value, const std::string & name)
    {
        const double numericValue = value.as_double();
        const int integerValue = value.as_int();
        if(numericValue != static_cast<double>(integerValue))
            throw exception("MidiInput parameter \"" + name +
                            "\" must be an integer.");
        return integerValue;
    }


    void
    ValidateOutputs() const
    {
        if(key_.size() != 1 || gate_.size() != 1 || trig_.size() != 1 ||
           eventCountOut_.size() != 1 || sourceCountOut_.size() != 1 ||
           connectedOut_.size() != 1 || lastBatchCountOut_.size() != 1)
            throw exception("MidiInput outputs must all contain exactly one value.");
    }


    parameter sourceIndex_;
    parameter trigHoldTicks_;
    matrix key_;
    matrix gate_;
    matrix trig_;
    matrix eventCountOut_;
    matrix sourceCountOut_;
    matrix connectedOut_;
    matrix lastBatchCountOut_;

    std::shared_ptr<midi::EventState> eventState_;
    int sourceIndexValue_ = -1;
    int trigHoldTicksValue_ = 1;
    int trigCountdown_ = 0;
    std::uint64_t eventCount_ = 0;

#ifdef MAC_OS_X
    struct Connection
    {
        MIDIEndpointRef source = 0;
        midi::SourceState * state = nullptr;
    };

    MIDIPortRef inputPort_ = 0;
    std::vector<Connection> connections_;
    std::uint64_t topologyGeneration_ = 0;
    bool runtimeStartWarningReported_ = false;


    static std::string
    CFStringToStdString(CFStringRef value)
    {
        if(value == nullptr)
            return "";

        const char * cString =
            CFStringGetCStringPtr(value, kCFStringEncodingUTF8);
        if(cString != nullptr)
            return std::string(cString);

        const CFIndex length = CFStringGetLength(value);
        const CFIndex maxSize =
            CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
        std::string result(static_cast<std::size_t>(maxSize), '\0');
        if(CFStringGetCString(value, result.data(), maxSize,
                              kCFStringEncodingUTF8))
        {
            result.resize(std::strlen(result.c_str()));
            return result;
        }

        return "";
    }


    static std::string
    SourceName(MIDIEndpointRef source)
    {
        if(source == 0)
            return "Unknown";

        CFStringRef name = nullptr;
        const OSStatus status =
            MIDIObjectGetStringProperty(source, kMIDIPropertyName, &name);
        if(status != noErr || name == nullptr)
            return "Unknown";

        std::string result = CFStringToStdString(name);
        CFRelease(name);
        return result.empty() ? "Unknown" : result;
    }


    void
    StartPort()
    {
        const MIDIClientRef client = CoreMIDIService::client();
        const std::uint64_t observedGeneration =
            CoreMIDIService::topologyGeneration();

        eventState_ = std::make_shared<midi::EventState>();
        eventState_->activate();
        CreateInputPort(client);
        RefreshSources();
        topologyGeneration_ = observedGeneration;
        runtimeStartWarningReported_ = false;
    }


    void
    StopPort() noexcept
    {
        if(eventState_)
            eventState_->deactivate();

        if(inputPort_ != 0)
        {
            for(const Connection & connection : connections_)
                MIDIPortDisconnectSource(inputPort_, connection.source);
            MIDIPortDispose(inputPort_);
            inputPort_ = 0;
        }

        if(eventState_)
        {
            eventState_->waitForCallbacks();
            eventState_->clear();
        }

        connections_.clear();
        eventState_.reset();
    }


    void
    MaintainPort()
    {
        try
        {
            if(inputPort_ == 0)
            {
                StartPort();
                return;
            }

            const std::uint64_t currentGeneration =
                CoreMIDIService::topologyGeneration();
            if(currentGeneration != topologyGeneration_)
            {
                StopPort();
                StartPort();
            }
        }
        catch(const std::exception & error)
        {
            StopPort();
            connectedOut_ = 0;
            if(!runtimeStartWarningReported_)
            {
                Notify(msg_warning,
                       "MidiInput could not restart CoreMIDI: " +
                           std::string(error.what()));
                runtimeStartWarningReported_ = true;
            }
        }
    }


    void
    CreateInputPort(MIDIClientRef client)
    {
        const std::shared_ptr<midi::EventState> callbackState = eventState_;
        if(__builtin_available(macOS 11.0, *))
        {
            const OSStatus status = MIDIInputPortCreateWithProtocol(
                client,
                CFSTR("Input"),
                kMIDIProtocol_2_0,
                &inputPort_,
                ^(const MIDIEventList * eventList, void * sourceContext)
                {
                    if(!callbackState->beginCallback())
                        return;

                    midi::SourceState & source = sourceContext == nullptr
                        ? callbackState->fallbackSource()
                        : *static_cast<midi::SourceState *>(sourceContext);
                    const MIDIEventPacket * packet = &eventList->packet[0];
                    for(UInt32 i = 0; i < eventList->numPackets; ++i)
                    {
                        midi::parseUMP(
                            std::span<const std::uint32_t>(
                                packet->words,
                                static_cast<std::size_t>(packet->wordCount)),
                            *callbackState,
                            source);
                        packet = MIDIEventPacketNext(packet);
                    }
                    callbackState->endCallback();
                });
            if(status == noErr)
                return;

            Notify(msg_warning,
                   "MidiInput could not create a protocol-based CoreMIDI "
                   "input port. Falling back to legacy packets.");
        }

        const OSStatus status = MIDIInputPortCreateWithBlock(
            client,
            CFSTR("Input"),
            &inputPort_,
            ^(const MIDIPacketList * packetList, void * sourceContext)
            {
                if(!callbackState->beginCallback())
                    return;

                midi::SourceState & source = sourceContext == nullptr
                    ? callbackState->fallbackSource()
                    : *static_cast<midi::SourceState *>(sourceContext);
                const MIDIPacket * packet = &packetList->packet[0];
                for(UInt32 i = 0; i < packetList->numPackets; ++i)
                {
                    midi::parseMidi1(
                        std::span<const std::uint8_t>(
                            packet->data,
                            static_cast<std::size_t>(packet->length)),
                        *callbackState,
                        source);
                    packet = MIDIPacketNext(packet);
                }
                callbackState->endCallback();
            });
        if(status != noErr)
            throw exception(
                "MidiInput could not create a CoreMIDI input port (OSStatus " +
                std::to_string(status) + ").");
    }


    void
    RefreshSources()
    {
        const ItemCount sourceCount = MIDIGetNumberOfSources();
        sourceCountOut_ = static_cast<float>(sourceCount);

        std::ostringstream availableSources;
        availableSources << "MidiInput found " << sourceCount << " MIDI source";
        if(sourceCount != 1)
            availableSources << "s";
        for(ItemCount i = 0; i < sourceCount; ++i)
            availableSources << " [" << i << ": "
                             << SourceName(MIDIGetSource(i)) << "]";
        Notify(msg_print, availableSources.str());

        if(sourceCount == 0)
        {
            Notify(msg_warning,
                   "MidiInput found no MIDI sources and will wait for one to "
                   "be connected.");
            connectedOut_ = 0;
            return;
        }

        if(sourceIndexValue_ >= 0)
        {
            if(static_cast<ItemCount>(sourceIndexValue_) >= sourceCount)
            {
                Notify(msg_warning,
                       "MidiInput source_index is not currently available and "
                       "will be retried when the MIDI configuration changes.");
                connectedOut_ = 0;
                return;
            }
            ConnectSource(MIDIGetSource(static_cast<ItemCount>(sourceIndexValue_)),
                          sourceIndexValue_);
        }
        else
        {
            for(ItemCount i = 0; i < sourceCount; ++i)
                ConnectSource(MIDIGetSource(i), static_cast<int>(i));
        }

        connectedOut_ = connections_.empty() ? 0.0f : 1.0f;
    }


    void
    ConnectSource(MIDIEndpointRef source, int index)
    {
        if(source == 0)
            return;

        midi::SourceState & sourceState = eventState_->createSource();
        const OSStatus status =
            MIDIPortConnectSource(inputPort_, source, &sourceState);
        if(status != noErr)
        {
            Notify(msg_warning,
                   "MidiInput could not connect to MIDI source " +
                       std::to_string(index) + " (OSStatus " +
                       std::to_string(status) + ").");
            return;
        }

        connections_.push_back({source, &sourceState});
        Notify(msg_print,
               "MidiInput connected to source " + std::to_string(index) +
                   ": " + SourceName(source));
    }
#endif
};

INSTALL_CLASS(MidiInput)
