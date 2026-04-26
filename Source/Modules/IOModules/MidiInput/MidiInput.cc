#include "ikaros.h"

#ifdef MAC_OS_X
#include <CoreFoundation/CoreFoundation.h>
#include <CoreMIDI/CoreMIDI.h>
#endif

#include <cstring>
#include <deque>
#include <mutex>
#include <sstream>
#include <unordered_set>

using namespace ikaros;

class MidiInput: public Module
{
public:
    ~MidiInput() override
    {
#ifdef MAC_OS_X
        for(MIDIEndpointRef source : connected_sources_)
            MIDIPortDisconnectSource(input_port_, source);

        if(input_port_ != 0)
            MIDIPortDispose(input_port_);

        if(client_ != 0)
            MIDIClientDispose(client_);
#endif
    }

    void Init() override
    {
        Bind(source_index_, "source_index");
        Bind(trig_hold_ticks_, "trig_hold_ticks");
        Bind(key_, "KEY");
        Bind(gate_, "GATE");
        Bind(trig_, "TRIG");
        Bind(event_count_out_, "EVENT_COUNT");
        Bind(source_count_out_, "SOURCE_COUNT");
        Bind(connected_out_, "CONNECTED");
        Bind(last_batch_count_out_, "LAST_BATCH_COUNT");

        key_ = 0;
        gate_ = 0;
        trig_ = 0;
        event_count_out_ = 0;
        source_count_out_ = 0;
        connected_out_ = 0;
        last_batch_count_out_ = 0;

#ifndef MAC_OS_X
        Notify(msg_fatal_error, "MidiInput is only available on macOS.");
        return;
#else
        CFStringRef client_name = CFSTR("Ikaros MidiInput");
        OSStatus status = MIDIClientCreate(client_name, nullptr, nullptr, &client_);
        if(status != noErr)
        {
            Notify(msg_fatal_error, "MidiInput could not create a CoreMIDI client.");
            return;
        }

        if(!CreateInputPort())
        {
            Notify(msg_fatal_error, "MidiInput could not create a CoreMIDI input port.");
            return;
        }

        const ItemCount source_count = MIDIGetNumberOfSources();
        source_count_out_ = static_cast<float>(source_count);
        if(source_count == 0)
        {
            Notify(msg_warning, "MidiInput found no MIDI sources.");
            return;
        }

        LogAvailableSources(source_count);

        const int requested_source = source_index_.as_int();
        if(requested_source >= 0)
        {
            if(requested_source >= static_cast<int>(source_count))
            {
                Notify(msg_fatal_error, "MidiInput source_index is out of range.");
                return;
            }

            ConnectSource(MIDIGetSource(requested_source), requested_source);
        }
        else
        {
            for(ItemCount i = 0; i < source_count; ++i)
                ConnectSource(MIDIGetSource(i), static_cast<int>(i));
        }

        connected_out_ = connected_sources_.empty() ? 0.0f : 1.0f;
#endif
    }

    void Tick() override
    {
        int latest_note = -1;
        int drained_note_count = 0;
        bool gate_high = false;

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if(!pending_note_ons_.empty())
            {
                latest_note = pending_note_ons_.back();
                drained_note_count = static_cast<int>(pending_note_ons_.size());
                pending_note_ons_.clear();
            }

            gate_high = !held_notes_.empty();
        }

        if(latest_note >= 0)
        {
            key_ = latest_note;
            event_count_ += drained_note_count;
            event_count_out_ = static_cast<float>(event_count_);
            last_batch_count_out_ = static_cast<float>(drained_note_count);
            trig_countdown_ = std::max(1, trig_hold_ticks_.as_int());
        }
        else
            last_batch_count_out_ = 0;

        gate_ = gate_high ? 1.0f : 0.0f;
        trig_ = trig_countdown_ > 0 ? 1.0f : 0.0f;
        if(trig_countdown_ > 0)
            --trig_countdown_;
    }

private:
    parameter source_index_;
    parameter trig_hold_ticks_;
    matrix key_;
    matrix gate_;
    matrix trig_;
    matrix event_count_out_;
    matrix source_count_out_;
    matrix connected_out_;
    matrix last_batch_count_out_;

    std::mutex queue_mutex_;
    std::deque<int> pending_note_ons_;
    std::unordered_set<int> held_notes_;
    int trig_countdown_ = 0;
    int event_count_ = 0;

#ifdef MAC_OS_X
    MIDIClientRef client_ = 0;
    MIDIPortRef input_port_ = 0;
    std::vector<MIDIEndpointRef> connected_sources_;
    bool using_protocol_port_ = false;
    Byte running_status_ = 0;

    static void ReadProc(const MIDIPacketList * packet_list, void * read_proc_ref_con, void *)
    {
        static_cast<MidiInput *>(read_proc_ref_con)->HandlePacketList(packet_list);
    }

    static std::string CFStringToStdString(CFStringRef value)
    {
        if(value == nullptr)
            return "";

        const char * c_string = CFStringGetCStringPtr(value, kCFStringEncodingUTF8);
        if(c_string != nullptr)
            return std::string(c_string);

        const CFIndex length = CFStringGetLength(value);
        const CFIndex max_size = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
        std::string result(static_cast<size_t>(max_size), '\0');
        if(CFStringGetCString(value, result.data(), max_size, kCFStringEncodingUTF8))
        {
            result.resize(std::strlen(result.c_str()));
            return result;
        }

        return "";
    }

    static std::string SourceName(MIDIEndpointRef source)
    {
        if(source == 0)
            return "Unknown";

        CFStringRef name = nullptr;
        const OSStatus status = MIDIObjectGetStringProperty(source, kMIDIPropertyName, &name);
        if(status != noErr || name == nullptr)
            return "Unknown";

        std::string result = CFStringToStdString(name);
        CFRelease(name);
        return result.empty() ? "Unknown" : result;
    }

    void LogAvailableSources(ItemCount source_count)
    {
        std::ostringstream message;
        message << "MidiInput found " << static_cast<int>(source_count) << " MIDI source";
        if(source_count != 1)
            message << "s";

        for(ItemCount i = 0; i < source_count; ++i)
            message << " [" << static_cast<int>(i) << ": " << SourceName(MIDIGetSource(i)) << "]";

        Notify(msg_warning, message.str());
    }

    bool CreateInputPort()
    {
        if(__builtin_available(macOS 11.0, *))
        {
            OSStatus status = MIDIInputPortCreateWithProtocol(
                client_,
                CFSTR("Input"),
                kMIDIProtocol_2_0,
                &input_port_,
                ^(const MIDIEventList * event_list, void *)
                {
                    HandleEventList(event_list);
                });

            if(status == noErr)
            {
                using_protocol_port_ = true;
                return true;
            }

            Notify(msg_warning, "MidiInput could not create a protocol-based CoreMIDI input port. Falling back to legacy packets.");
        }

        const OSStatus status = MIDIInputPortCreate(client_, CFSTR("Input"), &MidiInput::ReadProc, this, &input_port_);
        if(status == noErr)
        {
            using_protocol_port_ = false;
            return true;
        }

        return false;
    }

    void ConnectSource(MIDIEndpointRef source, int index)
    {
        if(source == 0)
            return;

        const OSStatus status = MIDIPortConnectSource(input_port_, source, nullptr);
        if(status != noErr)
        {
            Notify(msg_warning, "MidiInput could not connect to MIDI source " + std::to_string(index) + ".");
            return;
        }

        connected_sources_.push_back(source);
        Notify(msg_warning, "MidiInput connected to source " + std::to_string(index) + ": " + SourceName(source));
    }

    void HandlePacketList(const MIDIPacketList * packet_list)
    {
        const MIDIPacket * packet = &packet_list->packet[0];
        for(UInt32 i = 0; i < packet_list->numPackets; ++i)
        {
            ParseMidiBytes(packet->data, packet->length);
            packet = MIDIPacketNext(packet);
        }
    }

    void HandleEventList(const MIDIEventList * event_list)
    {
        const MIDIEventPacket * packet = &event_list->packet[0];
        for(UInt32 i = 0; i < event_list->numPackets; ++i)
        {
            UInt32 word_index = 0;
            while(word_index < packet->wordCount)
            {
                const UInt32 first_word = packet->words[word_index];
                const UInt32 message_type = (first_word >> 28) & 0x0F;

                if(message_type == 0x04 && word_index + 1 < packet->wordCount)
                {
                    ParseMidi2ChannelVoice(first_word, packet->words[word_index + 1]);
                    word_index += 2;
                }
                else
                {
                    ParseUMPWord(first_word);
                    word_index += 1;
                }
            }

            packet = MIDIEventPacketNext(packet);
        }
    }

    void ParseMidiBytes(const Byte * data, UInt16 length)
    {
        UInt16 index = 0;
        while(index < length)
        {
            Byte status = 0;

            if(data[index] & 0x80)
            {
                status = data[index++];

                // Ignore real-time bytes and clear running status on system common/status bytes.
                if(status >= 0xF8)
                    continue;

                if(status >= 0xF0)
                {
                    running_status_ = 0;
                    continue;
                }

                running_status_ = status;
            }
            else if(running_status_ != 0)
                status = running_status_;
            else
            {
                index += 1;
                continue;
            }

            const Byte message_type = status & 0xF0;
            const int data_size = (message_type == 0xC0 || message_type == 0xD0) ? 1 : 2;
            if(index + data_size > length)
                break;

            const Byte data1 = data[index];
            if(data1 & 0x80)
            {
                running_status_ = 0;
                continue;
            }

            Byte data2 = 0;
            if(data_size == 2)
            {
                data2 = data[index + 1];
                if(data2 & 0x80)
                {
                    running_status_ = 0;
                    continue;
                }
            }

            if(message_type == 0x90)
            {
                LogNoteOn(status, data1, data2, "MIDI1");
                if(data2 > 0)
                    QueueNoteOn(static_cast<int>(data1));
                else
                    QueueNoteOff(static_cast<int>(data1));
            }
            else if(message_type == 0x80)
                QueueNoteOff(static_cast<int>(data1));

            index += data_size;
        }
    }

    void ParseUMPWord(UInt32 word)
    {
        const UInt32 message_type = (word >> 28) & 0x0F;
        if(message_type != 0x02)
            return;

        const Byte status = static_cast<Byte>((word >> 16) & 0xFF);
        const Byte data1 = static_cast<Byte>((word >> 8) & 0x7F);
        const Byte data2 = static_cast<Byte>(word & 0x7F);
        if((status & 0xF0) == 0x80)
        {
            QueueNoteOff(static_cast<int>(data1));
            return;
        }

        if((status & 0xF0) != 0x90)
            return;

        LogNoteOn(status, data1, data2, "UMP1");
        if(data2 > 0)
            QueueNoteOn(static_cast<int>(data1));
        else
            QueueNoteOff(static_cast<int>(data1));
    }

    void ParseMidi2ChannelVoice(UInt32 first_word, UInt32 second_word)
    {
        const UInt32 status_nibble = (first_word >> 20) & 0x0F;
        const Byte channel = static_cast<Byte>((first_word >> 16) & 0x0F);
        const Byte note = static_cast<Byte>((first_word >> 8) & 0x7F);
        const uint16_t velocity = static_cast<uint16_t>((second_word >> 16) & 0xFFFF);
        const Byte status = static_cast<Byte>((status_nibble << 4) | channel);
        if(status_nibble == 0x8)
        {
            QueueNoteOff(static_cast<int>(note));
            return;
        }

        if(status_nibble != 0x9)
            return;

        LogNoteOn(status, note, static_cast<Byte>(velocity > 0 ? 127 : 0), "UMP2");
        if(velocity > 0)
            QueueNoteOn(static_cast<int>(note));
        else
            QueueNoteOff(static_cast<int>(note));
    }

    void LogNoteOn(Byte status, Byte data1, Byte data2, const char * source)
    {
        if((status & 0xF0) != 0x90)
            return;
        Notify(
            msg_warning,
            "MidiInput received " + std::string(source) +
            " status=0x" + ToHex(status) +
            " data1=" + std::to_string(static_cast<int>(data1)) +
            " data2=" + std::to_string(static_cast<int>(data2)));
    }

    static std::string ToHex(Byte value)
    {
        const char * digits = "0123456789ABCDEF";
        std::string result = "00";
        result[0] = digits[(value >> 4) & 0x0F];
        result[1] = digits[value & 0x0F];
        return result;
    }
#endif

    void QueueNoteOn(int note)
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        held_notes_.insert(note);
        pending_note_ons_.push_back(note);
    }

    void QueueNoteOff(int note)
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        held_notes_.erase(note);
    }
};

INSTALL_CLASS(MidiInput)
