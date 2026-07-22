#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <thread>
#include <vector>

#include "ikaros.h"
#include "Modules/IOModules/MidiInput/MidiProtocol.h"

using namespace ikaros;

namespace
{
    void
    require(bool condition, const std::string & message)
    {
        if(!condition)
            throw exception("MidiProtocolTestModule: " + message);
    }


    constexpr std::uint32_t
    umpMidi1(unsigned int group, unsigned int status, unsigned int channel,
             unsigned int data1, unsigned int data2)
    {
        return (0x02u << 28) | ((group & 0x0F) << 24) |
               ((status & 0x0F) << 20) | ((channel & 0x0F) << 16) |
               ((data1 & 0x7F) << 8) | (data2 & 0x7F);
    }


    constexpr std::array<std::uint32_t, 2>
    umpMidi2(unsigned int group, unsigned int status, unsigned int channel,
             unsigned int note, std::uint16_t velocity)
    {
        return {
            (0x04u << 28) | ((group & 0x0F) << 24) |
                ((status & 0x0F) << 20) | ((channel & 0x0F) << 16) |
                ((note & 0x7F) << 8),
            static_cast<std::uint32_t>(velocity) << 16,
        };
    }


    void
    testUMPWordCounts()
    {
        constexpr std::array<std::size_t, 16> expected
        {
            1, 1, 1, 2, 2, 4, 1, 1,
            2, 2, 2, 3, 3, 4, 4, 4,
        };
        for(std::size_t type = 0; type < expected.size(); ++type)
            require(midi::umpWordCount(static_cast<std::uint32_t>(type << 28)) ==
                        expected[type],
                    "UMP message width table is incorrect");
    }


    void
    testMultiwordFraming()
    {
        midi::EventState events;
        midi::SourceState & source = events.createSource();
        const std::array<std::uint32_t, 7> words
        {
            0x30000000,
            umpMidi1(0, 9, 0, 60, 100),
            0x50000000,
            umpMidi1(0, 9, 0, 61, 100),
            0x40903E00,
            0x20000000,
            umpMidi1(0, 9, 0, 64, 100),
        };

        midi::parseUMP(words, events, source);
        const midi::EventBatch batch = events.consume();
        require(batch.count == 1 && batch.latestNote == 64,
                "multiword UMP payloads were interpreted as message headers");
        require(!source.noteHeld(0, 0, 60) && !source.noteHeld(0, 0, 61) &&
                    !source.noteHeld(0, 0, 63) && source.noteHeld(0, 0, 64),
                "multiword UMP framing changed held-note state");
    }


    void
    testTruncatedUMP()
    {
        midi::EventState events;
        midi::SourceState & source = events.createSource();
        const std::array<std::uint32_t, 2> words
        {
            0x50000000,
            umpMidi1(0, 9, 0, 60, 100),
        };
        midi::parseUMP(words, events, source);
        require(events.consume().count == 0 && !source.anyNoteHeld(),
                "truncated UMP message exposed a payload word");
    }


    void
    testMidi1Messages()
    {
        midi::EventState events;
        midi::SourceState & source = events.createSource();
        const std::array<std::uint32_t, 4> words
        {
            umpMidi1(3, 9, 5, 60, 100),
            umpMidi1(3, 9, 5, 61, 0),
            umpMidi1(3, 8, 5, 60, 64),
            umpMidi1(3, 9, 5, 62, 100),
        };
        midi::parseUMP(words, events, source);
        const midi::EventBatch batch = events.consume();
        require(batch.count == 2 && batch.latestNote == 62,
                "MIDI 1 UMP note-on events were decoded incorrectly");
        require(!source.noteHeld(3, 5, 60) &&
                    !source.noteHeld(3, 5, 61) &&
                    source.noteHeld(3, 5, 62),
                "MIDI 1 UMP note-off state was incorrect");
    }


    void
    testMidi2Messages()
    {
        midi::EventState events;
        midi::SourceState & source = events.createSource();
        const auto noteOn = umpMidi2(4, 9, 6, 70, 0x8000);
        const auto zeroVelocity = umpMidi2(4, 9, 6, 71, 0);
        const auto noteOff = umpMidi2(4, 8, 6, 70, 0x4000);

        midi::parseUMP(noteOn, events, source);
        midi::parseUMP(zeroVelocity, events, source);
        midi::parseUMP(noteOff, events, source);
        const midi::EventBatch batch = events.consume();
        require(batch.count == 1 && batch.latestNote == 70,
                "MIDI 2 note-on velocity handling was incorrect");
        require(!source.noteHeld(4, 6, 70) &&
                    !source.noteHeld(4, 6, 71),
                "MIDI 2 note-off state was incorrect");
    }


    void
    testIndependentNoteIdentities()
    {
        midi::EventState events;
        midi::SourceState & first = events.createSource();
        midi::SourceState & second = events.createSource();

        events.noteOn(first, 0, 1, 60);
        events.noteOn(first, 0, 2, 60);
        events.noteOn(second, 0, 1, 60);
        events.noteOff(first, 0, 1, 60);
        require(events.anyNoteHeld() && first.noteHeld(0, 2, 60) &&
                    second.noteHeld(0, 1, 60),
                "note-off cleared the same pitch on another channel or source");

        events.noteOff(first, 0, 2, 60);
        require(events.anyNoteHeld(),
                "note-off cleared the same pitch on another source");
        events.noteOff(second, 0, 1, 60);
        require(!events.anyNoteHeld(),
                "GATE remained high after every independent note was released");
        events.consume();
    }


    void
    testResetMessages()
    {
        midi::EventState events;
        midi::SourceState & source = events.createSource();
        events.noteOn(source, 0, 1, 60);
        events.noteOn(source, 0, 2, 61);

        const std::array<std::uint32_t, 1> allNotesOff
        {
            umpMidi1(0, 0x0B, 1, 123, 0),
        };
        midi::parseUMP(allNotesOff, events, source);
        require(!source.noteHeld(0, 1, 60) && source.noteHeld(0, 2, 61),
                "All Notes Off did not clear exactly one channel");

        const std::array<std::uint32_t, 1> systemReset{0x10FF0000};
        midi::parseUMP(systemReset, events, source);
        require(!source.anyNoteHeld(),
                "UMP System Reset did not clear held notes");
        events.consume();
    }


    void
    testLegacyPackets()
    {
        midi::EventState events;
        midi::SourceState & source = events.createSource();
        const std::array<std::uint8_t, 8> bytes
        {
            0x90, 60, 100,
            61, 100,
            0x90, 62, 100,
        };
        midi::parseMidi1(bytes, events, source);
        const midi::EventBatch batch = events.consume();
        require(batch.count == 2 && batch.latestNote == 62 &&
                    source.noteHeld(0, 0, 60) &&
                    !source.noteHeld(0, 0, 61) &&
                    source.noteHeld(0, 0, 62),
                "legacy packets incorrectly accepted running status");

        const std::array<std::uint8_t, 4> allSoundOff{0xB0, 120, 0, 0xFF};
        midi::parseMidi1(allSoundOff, events, source);
        require(!source.anyNoteHeld(),
                "legacy reset messages did not clear held notes");
    }


    void
    testConcurrentAccumulation()
    {
        midi::EventState events;
        midi::SourceState & source = events.createSource();
        constexpr int threadCount = 4;
        constexpr int eventsPerThread = 10000;
        std::vector<std::thread> threads;
        threads.reserve(threadCount);
        for(int thread = 0; thread < threadCount; ++thread)
            threads.emplace_back([&events, &source, thread]
            {
                for(int event = 0; event < eventsPerThread; ++event)
                    events.noteOn(source, 0, static_cast<unsigned int>(thread),
                                  static_cast<unsigned int>(event % 128));
            });
        for(auto & thread : threads)
            thread.join();

        const midi::EventBatch batch = events.consume();
        require(batch.count == threadCount * eventsPerThread &&
                    batch.latestNote >= 0 && batch.latestNote < 128,
                "concurrent note-on events were lost");
    }


    void
    testCallbackShutdown()
    {
        midi::EventState events;
        require(events.beginCallback(),
                "active event state rejected a callback");
        events.endCallback();
        events.deactivate();
        require(!events.beginCallback(),
                "deactivated event state accepted a callback");
        events.waitForCallbacks();
        events.activate();
        require(events.beginCallback(),
                "reactivated event state rejected a callback");
        events.endCallback();
    }
}


class MidiProtocolTestModule : public Module
{
    void
    Init() override
    {
        testUMPWordCounts();
        testMultiwordFraming();
        testTruncatedUMP();
        testMidi1Messages();
        testMidi2Messages();
        testIndependentNoteIdentities();
        testResetMessages();
        testLegacyPackets();
        testConcurrentAccumulation();
        testCallbackShutdown();
        std::cout << "MIDI PROTOCOL TEST OK" << std::endl;
    }
};

INSTALL_CLASS(MidiProtocolTestModule)
