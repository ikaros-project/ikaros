#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>


namespace ikaros::midi
{
    struct EventBatch
    {
        int latestNote = -1;
        std::uint64_t count = 0;
    };


    class SourceState
    {
    public:
        void noteOn(unsigned int group, unsigned int channel,
                    unsigned int note) noexcept;
        void noteOff(unsigned int group, unsigned int channel,
                     unsigned int note) noexcept;
        void clearChannel(unsigned int group, unsigned int channel) noexcept;
        void clear() noexcept;

        bool noteHeld(unsigned int group, unsigned int channel,
                      unsigned int note) const noexcept;
        bool anyNoteHeld() const noexcept;

    private:
        static constexpr std::size_t groupCount = 16;
        static constexpr std::size_t channelCount = 16;
        static constexpr std::size_t wordsPerChannel = 2;
        static constexpr std::size_t wordCount =
            groupCount * channelCount * wordsPerChannel;

        static std::size_t wordIndex(unsigned int group, unsigned int channel,
                                     unsigned int note) noexcept;

        std::array<std::atomic<std::uint64_t>, wordCount> activeNotes_{};
    };


    class EventState
    {
    public:
        EventState();

        SourceState & createSource();
        SourceState & fallbackSource() noexcept { return fallbackSource_; }

        void activate() noexcept;
        void deactivate() noexcept;
        bool beginCallback() noexcept;
        void endCallback() noexcept;
        void waitForCallbacks() const noexcept;

        void noteOn(SourceState & source, unsigned int group,
                    unsigned int channel, unsigned int note) noexcept;
        void noteOff(SourceState & source, unsigned int group,
                     unsigned int channel, unsigned int note) noexcept;
        void clearChannel(SourceState & source, unsigned int group,
                          unsigned int channel) noexcept;
        void clearSource(SourceState & source) noexcept;

        EventBatch consume() noexcept;
        bool anyNoteHeld() const noexcept;
        void clear() noexcept;

    private:
        static constexpr std::uint64_t noteMask = 0xFF;
        static constexpr unsigned int countShift = 8;

        void recordNoteOn(unsigned int note) noexcept;

        std::atomic<std::uint64_t> pendingEvents_{0};
        std::atomic<unsigned int> callbacksInProgress_{0};
        std::atomic<bool> acceptingCallbacks_{true};
        SourceState fallbackSource_;
        std::vector<std::unique_ptr<SourceState>> sources_;
    };


    std::size_t umpWordCount(std::uint32_t firstWord) noexcept;
    void parseUMP(std::span<const std::uint32_t> words, EventState & events,
                  SourceState & source) noexcept;
    void parseMidi1(std::span<const std::uint8_t> bytes, EventState & events,
                    SourceState & source) noexcept;
}
