#include "MidiProtocol.h"

#include <algorithm>
#include <limits>
#include <thread>


namespace ikaros::midi
{
    namespace
    {
        constexpr std::array<std::size_t, 16> umpWordCounts
        {
            1, 1, 1, 2, 2, 4, 1, 1,
            2, 2, 2, 3, 3, 4, 4, 4,
        };


        constexpr unsigned int
        umpMessageType(std::uint32_t word) noexcept
        {
            return (word >> 28) & 0x0F;
        }


        constexpr unsigned int
        umpGroup(std::uint32_t word) noexcept
        {
            return (word >> 24) & 0x0F;
        }


        void
        parseChannelVoice1(std::uint32_t word, EventState & events,
                           SourceState & source) noexcept
        {
            const unsigned int group = umpGroup(word);
            const unsigned int status = (word >> 20) & 0x0F;
            const unsigned int channel = (word >> 16) & 0x0F;
            const unsigned int data1 = (word >> 8) & 0x7F;
            const unsigned int data2 = word & 0x7F;

            if(status == 0x08)
                events.noteOff(source, group, channel, data1);
            else if(status == 0x09)
            {
                if(data2 == 0)
                    events.noteOff(source, group, channel, data1);
                else
                    events.noteOn(source, group, channel, data1);
            }
            else if(status == 0x0B && (data1 == 120 || data1 == 123))
                events.clearChannel(source, group, channel);
        }


        void
        parseChannelVoice2(std::uint32_t firstWord, std::uint32_t secondWord,
                           EventState & events, SourceState & source) noexcept
        {
            const unsigned int group = umpGroup(firstWord);
            const unsigned int status = (firstWord >> 20) & 0x0F;
            const unsigned int channel = (firstWord >> 16) & 0x0F;
            const unsigned int note = (firstWord >> 8) & 0x7F;

            if(status == 0x08)
                events.noteOff(source, group, channel, note);
            else if(status == 0x09)
            {
                const std::uint16_t velocity =
                    static_cast<std::uint16_t>(secondWord >> 16);
                if(velocity == 0)
                    events.noteOff(source, group, channel, note);
                else
                    events.noteOn(source, group, channel, note);
            }
            else if(status == 0x0B && (note == 120 || note == 123))
                events.clearChannel(source, group, channel);
        }
    }


    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
                  "MidiInput requires lock-free 64-bit atomics");


    std::size_t
    SourceState::wordIndex(unsigned int group, unsigned int channel,
                           unsigned int note) noexcept
    {
        return ((group * channelCount + channel) * wordsPerChannel) + note / 64;
    }


    void
    SourceState::noteOn(unsigned int group, unsigned int channel,
                        unsigned int note) noexcept
    {
        if(group >= groupCount || channel >= channelCount || note >= 128)
            return;

        activeNotes_[wordIndex(group, channel, note)].fetch_or(
            std::uint64_t{1} << (note % 64), std::memory_order_relaxed);
    }


    void
    SourceState::noteOff(unsigned int group, unsigned int channel,
                         unsigned int note) noexcept
    {
        if(group >= groupCount || channel >= channelCount || note >= 128)
            return;

        activeNotes_[wordIndex(group, channel, note)].fetch_and(
            ~(std::uint64_t{1} << (note % 64)), std::memory_order_relaxed);
    }


    void
    SourceState::clearChannel(unsigned int group, unsigned int channel) noexcept
    {
        if(group >= groupCount || channel >= channelCount)
            return;

        const std::size_t firstWord =
            (group * channelCount + channel) * wordsPerChannel;
        activeNotes_[firstWord].store(0, std::memory_order_relaxed);
        activeNotes_[firstWord + 1].store(0, std::memory_order_relaxed);
    }


    void
    SourceState::clear() noexcept
    {
        for(auto & word : activeNotes_)
            word.store(0, std::memory_order_relaxed);
    }


    bool
    SourceState::noteHeld(unsigned int group, unsigned int channel,
                          unsigned int note) const noexcept
    {
        if(group >= groupCount || channel >= channelCount || note >= 128)
            return false;

        return (activeNotes_[wordIndex(group, channel, note)].load(
                    std::memory_order_relaxed) &
                (std::uint64_t{1} << (note % 64))) != 0;
    }


    bool
    SourceState::anyNoteHeld() const noexcept
    {
        return std::any_of(activeNotes_.begin(), activeNotes_.end(),
                           [](const auto & word)
                           {
                               return word.load(std::memory_order_relaxed) != 0;
                           });
    }


    EventState::EventState()
    {
        sources_.reserve(8);
    }


    SourceState &
    EventState::createSource()
    {
        sources_.push_back(std::make_unique<SourceState>());
        return *sources_.back();
    }


    void
    EventState::activate() noexcept
    {
        acceptingCallbacks_.store(true, std::memory_order_release);
    }


    void
    EventState::deactivate() noexcept
    {
        acceptingCallbacks_.store(false, std::memory_order_release);
    }


    bool
    EventState::beginCallback() noexcept
    {
        if(!acceptingCallbacks_.load(std::memory_order_acquire))
            return false;

        callbacksInProgress_.fetch_add(1, std::memory_order_acq_rel);
        if(acceptingCallbacks_.load(std::memory_order_acquire))
            return true;

        callbacksInProgress_.fetch_sub(1, std::memory_order_release);
        return false;
    }


    void
    EventState::endCallback() noexcept
    {
        callbacksInProgress_.fetch_sub(1, std::memory_order_release);
    }


    void
    EventState::waitForCallbacks() const noexcept
    {
        while(callbacksInProgress_.load(std::memory_order_acquire) != 0)
            std::this_thread::yield();
    }


    void
    EventState::recordNoteOn(unsigned int note) noexcept
    {
        if(note >= 128)
            return;

        std::uint64_t current = pendingEvents_.load(std::memory_order_relaxed);
        for(;;)
        {
            const std::uint64_t count = current >> countShift;
            const std::uint64_t nextCount =
                count == (std::numeric_limits<std::uint64_t>::max() >> countShift)
                    ? count
                    : count + 1;
            const std::uint64_t desired =
                (nextCount << countShift) | static_cast<std::uint64_t>(note + 1);
            if(pendingEvents_.compare_exchange_weak(
                   current, desired, std::memory_order_release,
                   std::memory_order_relaxed))
                return;
        }
    }


    void
    EventState::noteOn(SourceState & source, unsigned int group,
                       unsigned int channel, unsigned int note) noexcept
    {
        if(group >= 16 || channel >= 16 || note >= 128)
            return;

        source.noteOn(group, channel, note);
        recordNoteOn(note);
    }


    void
    EventState::noteOff(SourceState & source, unsigned int group,
                        unsigned int channel, unsigned int note) noexcept
    {
        source.noteOff(group, channel, note);
    }


    void
    EventState::clearChannel(SourceState & source, unsigned int group,
                             unsigned int channel) noexcept
    {
        source.clearChannel(group, channel);
    }


    void
    EventState::clearSource(SourceState & source) noexcept
    {
        source.clear();
    }


    EventBatch
    EventState::consume() noexcept
    {
        const std::uint64_t pending =
            pendingEvents_.exchange(0, std::memory_order_acquire);
        if(pending == 0)
            return {};

        return {
            static_cast<int>((pending & noteMask) - 1),
            pending >> countShift,
        };
    }


    bool
    EventState::anyNoteHeld() const noexcept
    {
        if(fallbackSource_.anyNoteHeld())
            return true;

        return std::any_of(sources_.begin(), sources_.end(),
                           [](const auto & source)
                           {
                               return source->anyNoteHeld();
                           });
    }


    void
    EventState::clear() noexcept
    {
        pendingEvents_.store(0, std::memory_order_relaxed);
        fallbackSource_.clear();
        for(auto & source : sources_)
            source->clear();
    }


    std::size_t
    umpWordCount(std::uint32_t firstWord) noexcept
    {
        return umpWordCounts[umpMessageType(firstWord)];
    }


    void
    parseUMP(std::span<const std::uint32_t> words, EventState & events,
             SourceState & source) noexcept
    {
        std::size_t index = 0;
        while(index < words.size())
        {
            const std::uint32_t firstWord = words[index];
            const unsigned int messageType = umpMessageType(firstWord);
            const std::size_t messageWordCount = umpWordCounts[messageType];
            if(messageWordCount > words.size() - index)
                return;

            if(messageType == 0x01)
            {
                const unsigned int status = (firstWord >> 16) & 0xFF;
                if(status == 0xFF)
                    events.clearSource(source);
            }
            else if(messageType == 0x02)
                parseChannelVoice1(firstWord, events, source);
            else if(messageType == 0x04)
                parseChannelVoice2(firstWord, words[index + 1], events, source);

            index += messageWordCount;
        }
    }


    void
    parseMidi1(std::span<const std::uint8_t> bytes, EventState & events,
               SourceState & source) noexcept
    {
        std::size_t index = 0;
        while(index < bytes.size())
        {
            const std::uint8_t statusByte = bytes[index++];
            if((statusByte & 0x80) == 0)
                continue;

            if(statusByte >= 0xF0)
            {
                if(statusByte == 0xFF)
                    events.clearSource(source);
                if(statusByte == 0xF0)
                    return;
                continue;
            }

            const unsigned int status = statusByte >> 4;
            const unsigned int channel = statusByte & 0x0F;
            const std::size_t dataSize =
                status == 0x0C || status == 0x0D ? 1 : 2;
            if(dataSize > bytes.size() - index)
                return;

            const std::uint8_t data1 = bytes[index];
            const std::uint8_t data2 = dataSize == 2 ? bytes[index + 1] : 0;
            if((data1 & 0x80) != 0 || (data2 & 0x80) != 0)
                continue;
            index += dataSize;

            if(status == 0x08)
                events.noteOff(source, 0, channel, data1);
            else if(status == 0x09)
            {
                if(data2 == 0)
                    events.noteOff(source, 0, channel, data1);
                else
                    events.noteOn(source, 0, channel, data1);
            }
            else if(status == 0x0B && (data1 == 120 || data1 == 123))
                events.clearChannel(source, 0, channel);
        }
    }
}
