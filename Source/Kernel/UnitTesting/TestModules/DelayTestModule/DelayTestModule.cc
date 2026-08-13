#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "ikaros.h"

using namespace ikaros;

namespace
{
    static_assert(std::is_same_v<decltype(std::declval<range &>().index()),
                                 const std::vector<int> &>);
    static_assert(!std::is_convertible_v<range &, std::vector<int> &>);
    static_assert(!std::is_copy_constructible_v<CircularBuffer>);
    static_assert(!std::is_copy_assignable_v<CircularBuffer>);
    static_assert(std::is_move_constructible_v<CircularBuffer>);
    static_assert(std::is_move_assignable_v<CircularBuffer>);
    static_assert(!std::is_default_constructible_v<CircularBuffer>);
    static_assert(std::is_same_v<decltype(std::declval<const CircularBuffer &>().get(1)),
                                 const matrix &>);

    float
    ExpectedDelayedValue(int tick, int delay, int offset)
    {
        return tick > delay ? static_cast<float>(tick - delay + offset) : 0.0f;
    }


    void
    CheckDelayedValue(float actual,
                      int tick,
                      int delay,
                      int offset,
                      const std::string & location)
    {
        const float expected = ExpectedDelayedValue(tick, delay, offset);
        if(actual != expected)
            throw exception(location + " received " + std::to_string(actual) +
                            " instead of " + std::to_string(expected) +
                            " at tick " + std::to_string(tick));
    }


    void
    CheckRangeValues(const std::string & specification, const std::vector<int> & expected)
    {
        range parsed(specification);
        if(parsed.size() != static_cast<int>(expected.size()))
            throw exception("Range " + specification + " reported size " +
                            std::to_string(parsed.size()) + " instead of " +
                            std::to_string(expected.size()));

        std::vector<int> actual;
        for(auto index = parsed; index.more(); ++index)
            actual.push_back(index.index()[0]);
        if(actual != expected)
            throw exception("Range " + specification + " iterated over unexpected values");
    }


#if defined(__GNUC__) || defined(__clang__)
    __attribute__((noinline))
#endif
    std::uint64_t
    ConsumeRange(const range & value)
    {
        std::uint64_t result = static_cast<std::uint64_t>(value.rank()) + value.size();
        for(int index : value.index())
            result += static_cast<std::uint64_t>(index);
        return result;
    }
}


class DelaySequenceSource : public Module
{
    parameter offset;
    matrix output;

    void Init() override
    {
        Bind(offset, "offset");
        Bind(output, "OUTPUT");

        matrix sample(1);
        bool invalidSizeRejected = false;
        try
        {
            CircularBuffer invalid(sample, 0);
        }
        catch(const std::invalid_argument &)
        {
            invalidSizeRejected = true;
        }

        CircularBuffer history(sample, 2);
        bool zeroDelayRejected = false;
        bool excessiveDelayRejected = false;
        try
        {
            history.get(0);
        }
        catch(const std::out_of_range &)
        {
            zeroDelayRejected = true;
        }
        try
        {
            history.get(3);
        }
        catch(const std::out_of_range &)
        {
            excessiveDelayRejected = true;
        }

        if(!invalidSizeRejected || !zeroDelayRejected || !excessiveDelayRejected)
            throw exception("CircularBuffer accepted an invalid size or delay");
        if(history.get(1)(0) != 0.0f || history.get(2)(0) != 0.0f)
            throw exception("CircularBuffer static history was not initialized to zero");

        sample(0) = 1.0f;
        history.rotate(sample);
        sample(0) = 2.0f;
        history.rotate(sample);
        sample(0) = 3.0f;
        history.rotate(sample);
        if(history.get(1)(0) != 3.0f || history.get(2)(0) != 2.0f)
            throw exception("CircularBuffer lost its order while wrapping");

#ifndef NDEBUG
        matrix dynamicSample;
        dynamicSample.reserve({2, 2});
        dynamicSample.set_dynamic().set_fixed_capacity();
        dynamicSample.resize({1, 2});
        dynamicSample(0, 0) = 4.0f;
        dynamicSample(0, 1) = 5.0f;
        CircularBuffer dynamicHistory(dynamicSample, 2);
        for(int delay = 1; delay <= dynamicHistory.size(); ++delay)
        {
            const matrix & initialFrame = dynamicHistory.get(delay);
            if(initialFrame.shape() != dynamicSample.shape() ||
               initialFrame(0, 0) != 0.0f || initialFrame(0, 1) != 0.0f)
                throw exception("CircularBuffer dynamic history was not initialized to zero");
        }

        matrix::set_allocation_failure_countdown_for_testing(0);
        try
        {
            dynamicHistory.rotate(dynamicSample);
        }
        catch(...)
        {
            matrix::set_allocation_failure_countdown_for_testing(-1);
            throw exception("CircularBuffer allocated while rotating an unchanged dynamic shape");
        }
        matrix::set_allocation_failure_countdown_for_testing(-1);

        const matrix & latestDynamicSample = dynamicHistory.get(1);
        if(latestDynamicSample.shape() != dynamicSample.shape() ||
           latestDynamicSample(0, 0) != 4.0f || latestDynamicSample(0, 1) != 5.0f)
            throw exception("CircularBuffer did not copy an unchanged dynamic shape");
#endif
    }

    void Tick() override
    {
        output(0) = static_cast<float>(GetTick() + offset.as_int());
    }
};


class DelayVectorSource : public Module
{
    matrix output;

    void Init() override
    {
        Bind(output, "OUTPUT");
    }

    void Tick() override
    {
        output(0) = static_cast<float>(GetTick());
        output(1) = static_cast<float>(GetTick() + 100);
    }
};


class AsyncDelaySequenceSource : public Module
{
    parameter duration;
    matrix output;
    int publication_count = 0;

    void Init() override
    {
        Bind(duration, "duration");
        Bind(output, "OUTPUT");
    }

    void Tick() override
    {
        std::this_thread::sleep_for(std::chrono::duration<double>(duration.as_double()));
        output(0) = static_cast<float>(++publication_count);
    }
};


class AsyncDelayHistorySink : public Module
{
    matrix input;
    int last_publication = 0;
    bool reported = false;

    void Init() override
    {
        Bind(input, "INPUT");
        if(input.size() != 3)
            throw exception("AsyncDelayHistorySink expected three delayed values");
    }

    void Tick() override
    {
        const int latest_publication = static_cast<int>(
            input.rank() == 1 ? input(0) : input(0, 0));
        if(latest_publication == 0 || latest_publication == last_publication)
            return;

        for(int delay_index = 0; delay_index < 3; ++delay_index)
        {
            const float expected = static_cast<float>(
                std::max(0, latest_publication - delay_index));
            const float actual = input.rank() == 1
                                     ? input(delay_index)
                                     : input(delay_index, 0);
            if(actual != expected)
                throw exception("Asynchronous delay history advanced between publications");
        }

        last_publication = latest_publication;
        if(!reported && latest_publication >= 3)
        {
            reported = true;
            std::cout << path_ << " ASYNC DELAY HISTORY TEST OK" << std::endl;
        }
    }
};


class DelayWindowSink : public Module
{
    parameter secondDelay;
    parameter thirdDelay;
    matrix input;
    bool reported = false;

    void Init() override
    {
        Bind(secondDelay, "second_delay");
        Bind(thirdDelay, "third_delay");
        Bind(input, "INPUT");

        if(input.size() != 3)
            throw exception("DelayWindowSink expected three input values, got rank " +
                            std::to_string(input.rank()) + " and size " + std::to_string(input.size()));
    }

    void Tick() override
    {
        const int delays[] = {0, secondDelay.as_int(), thirdDelay.as_int()};
        const int tick = static_cast<int>(GetTick());

        for(int lane = 0; lane < 3; lane++)
        {
            const float actual = input.rank() == 1 ? input(lane) : input(lane, 0);
            CheckDelayedValue(actual, tick, delays[lane], 0,
                              "DelayWindowSink lane " + std::to_string(lane));
        }

        const int lastDelay = std::max(delays[1], delays[2]);
        if(!reported && tick >= lastDelay)
        {
            reported = true;
            std::cout << path_ << " DELAY WINDOW TEST OK" << std::endl;
        }
    }
};


class DelayFlatWindowSink : public Module
{
    matrix input;
    bool reported = false;

    void Init() override
    {
        Bind(input, "INPUT");
        if(input.rank() != 1 || input.size() != 6)
            throw exception("DelayFlatWindowSink expected input shape 6");
    }

    void Tick() override
    {
        const int tick = static_cast<int>(GetTick());
        for(int delay = 0; delay < 3; delay++)
            for(int channel = 0; channel < 2; channel++)
            {
                const int offset = channel == 0 ? 0 : 100;
                const int lane = 2 * delay + channel;
                CheckDelayedValue(input(lane), tick, delay, offset,
                                  "DelayFlatWindowSink lane " + std::to_string(lane));
            }

        if(!reported && tick >= 2)
        {
            reported = true;
            std::cout << path_ << " FLAT DELAY WINDOW TEST OK" << std::endl;
        }
    }
};


class DelayStackedWindowSink : public Module
{
    matrix input;
    bool reported = false;

    void Init() override
    {
        Bind(input, "INPUT");
        if(input.rank() != 3 || input.shape()[0] != 2 ||
           input.shape()[1] != 3 || input.shape()[2] != 1)
            throw exception("DelayStackedWindowSink expected input shape 2,3,1");
    }

    void Tick() override
    {
        const int tick = static_cast<int>(GetTick());
        for(int source = 0; source < 2; source++)
            for(int delay = 0; delay < 3; delay++)
            {
                const int offset = source == 0 ? 0 : 100;
                CheckDelayedValue(input(source, delay, 0), tick, delay, offset,
                                  "DelayStackedWindowSink source " + std::to_string(source) +
                                  " lane " + std::to_string(delay));
            }

        if(!reported && tick >= 2)
        {
            reported = true;
            std::cout << path_ << " STACKED DELAY WINDOW TEST OK" << std::endl;
        }
    }
};


class DelayedPropagationFailureSink : public Module
{
    matrix input;

    void Init() override
    {
        Bind(input, "INPUT");
    }

    void Tick() override
    {
        input.realloc(std::vector<int>{1, 1});
        std::cout << path_ << " PROPAGATION FAILURE ARMED" << std::endl;
    }

    void Stop() override
    {
        std::cout << path_ << " STOP_AFTER_PROPAGATION_FAILURE" << std::endl;
    }
};


class DelayPropagationBenchmarkSink : public Module
{
    matrix input;
    double checksum = 0.0;

    void Init() override
    {
        Bind(input, "INPUT");
    }

    void Tick() override
    {
        checksum += GetTick();
    }

    void Stop() override
    {
        std::cout << path_ << " DELAY PROPAGATION BENCHMARK CHECKSUM "
                  << checksum << std::endl;
    }
};


class RangeBenchmarkModule : public Module
{
    template<typename Function>
    double MeasureNanosecondsPerOperation(int operations, Function function)
    {
        const auto start = std::chrono::steady_clock::now();
        function();
        const auto elapsed = std::chrono::duration<double, std::nano>(
            std::chrono::steady_clock::now() - start).count();
        return elapsed / operations;
    }

    double MeasureTraversal(const range & prototype, int repetitions,
                            std::uint64_t & checksum)
    {
        range current = prototype;
        const int operations = prototype.size() * repetitions;
        return MeasureNanosecondsPerOperation(operations, [&]()
        {
            for(int repetition = 0; repetition < repetitions; ++repetition)
            {
                current.reset();
                for(; current.more(); ++current)
                    for(int index : current.index())
                        checksum += static_cast<std::uint64_t>(index);
            }
        });
    }

    void Init() override
    {
        constexpr int traversal_repetitions = 1000;
        constexpr int allocation_operations = 250000;
        std::uint64_t checksum = 0;

        const double traversal_1d_ns = MeasureTraversal(range(0, 1000),
                                                        10 * traversal_repetitions,
                                                        checksum);
        const double traversal_2d_ns = MeasureTraversal(range("[0:100][0:100]"),
                                                        traversal_repetitions,
                                                        checksum);
        const double traversal_3d_ns = MeasureTraversal(range("[0:25][0:20][0:20]"),
                                                        traversal_repetitions,
                                                        checksum);

        const double construction_1d_ns = MeasureNanosecondsPerOperation(allocation_operations, [&]()
        {
            for(int operation = 0; operation < allocation_operations; ++operation)
            {
                range value(0, 16);
                checksum += ConsumeRange(value);
            }
        });
        const double construction_3d_ns = MeasureNanosecondsPerOperation(allocation_operations, [&]()
        {
            for(int operation = 0; operation < allocation_operations; ++operation)
            {
                range value{{0, 4, 1}, {0, 8, 1}, {0, 16, 1}};
                checksum += ConsumeRange(value);
            }
        });

        const range copy_source_1d(0, 16);
        const range copy_source_3d("[0:4][0:8][0:16]");
        const double copy_1d_ns = MeasureNanosecondsPerOperation(allocation_operations, [&]()
        {
            for(int operation = 0; operation < allocation_operations; ++operation)
            {
                range value = copy_source_1d;
                ++value;
                checksum += ConsumeRange(value);
            }
        });
        const double copy_3d_ns = MeasureNanosecondsPerOperation(allocation_operations, [&]()
        {
            for(int operation = 0; operation < allocation_operations; ++operation)
            {
                range value = copy_source_3d;
                ++value;
                checksum += ConsumeRange(value);
            }
        });

        std::ostringstream output;
        output << std::fixed << std::setprecision(3)
               << path_ << " RANGE BENCHMARK"
               << " traversal_1d_ns=" << traversal_1d_ns
               << " traversal_2d_ns=" << traversal_2d_ns
               << " traversal_3d_ns=" << traversal_3d_ns
               << " construction_1d_ns=" << construction_1d_ns
               << " construction_3d_ns=" << construction_3d_ns
               << " copy_1d_ns=" << copy_1d_ns
               << " copy_3d_ns=" << copy_3d_ns
               << " checksum=" << checksum;
        std::cout << output.str() << std::endl;
    }
};


class RangeSizeTestModule : public Module
{
    void Init() override
    {
        CheckRangeValues("[0:5:2]", {0, 2, 4});
        CheckRangeValues("[1:6:2]", {1, 3, 5});
        CheckRangeValues("[0:6:2]", {0, 2, 4});
        CheckRangeValues("[2:3:5]", {2});
        CheckRangeValues("[3:3]", {});
        CheckRangeValues("[3:2]", {});
        CheckRangeValues("[0:5:-2]", {4, 2, 0});
        CheckRangeValues("[0:6:-2]", {4, 2, 0});
        CheckRangeValues("[][0:2]", {});
        CheckRangeValues("[0:2][][0:2]", {});
        CheckRangeValues("[0:2][]", {});

        range empty(0, 0, 0);
        if(empty.rank() != 1 || empty.size() != 0 || !empty.empty() || empty.more())
            throw exception("Zero-increment range was not empty");

        if(!range().empty() || !range(3, 3).empty() ||
           !range("[0:2][][0:2]").empty() || range(0, 1).empty())
            throw exception("Range emptiness did not follow generated cardinality");

        if(!range("[]").is_placeholder(0) || !range(0, 0, 0).is_placeholder(0) ||
           range(3, 3).is_placeholder(0) || range(0, 3, 0).is_placeholder(0) ||
           range(1, 1, 0).is_placeholder(0))
            throw exception("Range placeholder detection was confused with emptiness");

        for(const std::string & text : {"[0:0:0]", "[:0:0]", "[::0]"})
        {
            try
            {
                static_cast<void>(range(text));
                throw exception("Numeric range \"" + text + "\" was accepted as a placeholder");
            }
            catch(const std::invalid_argument &)
            {
            }
        }

        range explicitZeroStep(0, 3, 0);
        const range originalExplicitZeroStep = explicitZeroStep;
        explicitZeroStep.fill(range(0, 5));
        if(!explicitZeroStep.same_state(originalExplicitZeroStep) ||
           range(std::string(explicitZeroStep)).is_placeholder(0))
            throw exception("Explicit zero-step range was treated as a placeholder");

        const std::vector<std::string> explicitEmptyRangeTexts = {
            "[0:0]",
            "[:]",
            "[0:0:1]",
            "[::]",
        };
        for(const std::string & text : explicitEmptyRangeTexts)
        {
            range parsed(text);
            if(parsed != range(0, 0) || parsed.is_placeholder(0))
                throw exception("Explicit empty range \"" + text + "\" was parsed as a placeholder");
        }

        range explicitEmptyForRoundTrip(0, 0);
        range restoredExplicitEmpty{std::string(explicitEmptyForRoundTrip)};
        if(!restoredExplicitEmpty.same_state(explicitEmptyForRoundTrip) ||
           restoredExplicitEmpty.is_placeholder(0))
            throw exception("Explicit empty range text did not preserve its definition");

        range mixedEmptyRange("[][0:0][::]");
        range restoredMixedEmptyRange{std::string(mixedEmptyRange)};
        if(!restoredMixedEmptyRange.same_state(mixedEmptyRange) ||
           !restoredMixedEmptyRange.is_placeholder(0) ||
           restoredMixedEmptyRange.is_placeholder(1) ||
           restoredMixedEmptyRange.is_placeholder(2))
            throw exception("Mixed placeholder and empty range text did not preserve its definitions");

        try
        {
            static_cast<void>(range().is_placeholder(0));
            throw exception("Placeholder detection accepted an invalid dimension");
        }
        catch(const std::out_of_range &)
        {
        }

        range strippedEmpty = range("[0:2][][3]").strip();
        if(strippedEmpty.rank() != 2 || !strippedEmpty.empty() || strippedEmpty.size() != 0 ||
           strippedEmpty.start(0) != 0 || strippedEmpty.stop(0) != 2 ||
           strippedEmpty.start(1) != 0 || strippedEmpty.stop(1) != 0)
            throw exception("Stripping a range did not preserve its empty dimension");

        range strippedSingletons = range("[3][5]").strip();
        if(strippedSingletons.rank() != 1 || strippedSingletons.size() != 1 ||
           strippedSingletons.start(0) != 1 || strippedSingletons.stop(0) != 2)
            throw exception("Stripping singleton dimensions did not preserve one element");

        range strippedRankZero = range().strip();
        if(strippedRankZero.rank() != 0 || !strippedRankZero.empty())
            throw exception("Stripping a rank-zero range changed its cardinality");

        range advancedForStrip("[0:3][5]");
        ++advancedForStrip;
        range strippedAdvanced = advancedForStrip.strip();
        if(strippedAdvanced.rank() != 1 ||
           strippedAdvanced.index() != std::vector<int>{1})
            throw exception("Stripping a range reset a retained iteration cursor");

        range reverseForStrip("[0:5:-2][7]");
        ++reverseForStrip;
        range strippedReverse = reverseForStrip.strip();
        if(strippedReverse.index() != std::vector<int>{2})
            throw exception("Stripping a reverse range reset its iteration cursor");

        range exhaustedForStrip("[0:2][5]");
        while(exhaustedForStrip.more())
            ++exhaustedForStrip;
        range strippedExhausted = exhaustedForStrip.strip();
        if(strippedExhausted.more())
            throw exception("Stripping an exhausted range restarted iteration");

        range removedLeadingExhaustion("[5][0:2]");
        while(removedLeadingExhaustion.more())
            ++removedLeadingExhaustion;
        if(removedLeadingExhaustion.strip().more())
            throw exception("Removing the exhausted dimension restarted range iteration");

        range exhaustedSingletons("[3][5]");
        ++exhaustedSingletons;
        range strippedExhaustedSingletons = exhaustedSingletons.strip();
        if(strippedExhaustedSingletons.size() != 1 || strippedExhaustedSingletons.more())
            throw exception("Stripping exhausted singleton dimensions restarted iteration");

        const std::string rankZeroText = std::string(range());
        if(!rankZeroText.empty() || range(rankZeroText).rank() != 0)
            throw exception("Rank-zero range text did not preserve rank");

        const std::string emptyDimensionText = std::string(range("[]"));
        range restoredEmptyDimension(emptyDimensionText);
        if(emptyDimensionText != "[]" || restoredEmptyDimension.rank() != 1 ||
           !restoredEmptyDimension.empty())
            throw exception("Explicit empty range dimension text did not preserve rank");

        range exhausted(0, 2);
        while(exhausted.more())
            ++exhausted;
        if(exhausted.empty())
            throw exception("Range emptiness depended on the iteration cursor");

        range partiallyAdvanced("[0:2][1:4]");
        ++partiallyAdvanced;
        ++partiallyAdvanced;
        range * resetResult = &partiallyAdvanced.reset();
        if(resetResult != &partiallyAdvanced ||
           partiallyAdvanced.index() != std::vector<int>{0, 1})
            throw exception("Reset did not restart every forward range dimension");

        range reverseAdvanced("[0:5:-2][1:6:-2]");
        ++reverseAdvanced;
        reverseAdvanced.reset();
        if(reverseAdvanced.index() != std::vector<int>{4, 5})
            throw exception("Reset did not restart every reverse range dimension");

        range dimensionReset("[0:2][0:3]");
        ++dimensionReset;
        ++dimensionReset;
        ++dimensionReset;
        ++dimensionReset;
        dimensionReset.reset(1);
        if(dimensionReset.index() != std::vector<int>{1, 0})
            throw exception("Dimension-specific reset changed another range dimension");

        range rankZeroReset;
        if(&rankZeroReset.reset() != &rankZeroReset || rankZeroReset.rank() != 0)
            throw exception("Resetting a rank-zero range was not a no-op");

        range multidimensional("[0:5:2][1:6:2]");
        std::vector<std::vector<int>> actualIndices;
        for(auto index = multidimensional; index.more(); ++index)
            actualIndices.push_back(index.index());
        const std::vector<std::vector<int>> expectedIndices{
            {0, 1}, {0, 3}, {0, 5},
            {2, 1}, {2, 3}, {2, 5},
            {4, 1}, {4, 3}, {4, 5}
        };
        if(multidimensional.size() != 9 || actualIndices != expectedIndices)
            throw exception("Multidimensional stepped range iterated over unexpected values");

        range higherRank("[0:2][0:2]");
        range lowerRank("[0:2]");
        try
        {
            higherRank.extend(lowerRank);
            throw exception("Extending from a lower-rank range was not rejected");
        }
        catch(const std::invalid_argument &)
        {
        }

        range misalignedFirst(0, 6, 2);
        range misalignedSecond(1, 6, 2);
        misalignedFirst.extend(misalignedSecond);
        if(misalignedFirst.step(0) != 1 || !(misalignedSecond <= misalignedFirst) ||
           misalignedFirst.size() != 6)
            throw exception("Extending misaligned ranges did not cover both progressions");

        range differingFirst(0, 10, 4);
        range differingSecond(2, 10, 6);
        differingFirst.extend(differingSecond);
        if(differingFirst.step(0) != 2 || !(differingSecond <= differingFirst) ||
           differingFirst.start(0) != 0 || differingFirst.stop(0) != 10)
            throw exception("Extending ranges with different steps did not use a covering step");

        range reverseCover(0, 6, -2);
        range forwardOffset(1, 6, 2);
        reverseCover.extend(forwardOffset);
        if(reverseCover.step(0) != -1 || !(forwardOffset <= reverseCover) ||
           reverseCover.index() != std::vector<int>{5})
            throw exception("Extending a reverse range did not preserve its traversal direction");

        range unchangedExtension(0, 5);
        ++unchangedExtension;
        const range originalUnchangedExtension = unchangedExtension;
        unchangedExtension.extend(range(1, 3));
        if(!unchangedExtension.same_state(originalUnchangedExtension))
            throw exception("A definition-preserving extension reset the iteration cursor");

        range selfExtension(0, 3);
        while(selfExtension.more())
            ++selfExtension;
        const range originalSelfExtension = selfExtension;
        selfExtension.extend(selfExtension);
        if(!selfExtension.same_state(originalSelfExtension))
            throw exception("Self-extension restarted an exhausted range");

        range steppedContainedExtension(0, 6, 2);
        ++steppedContainedExtension;
        const range originalSteppedContainedExtension = steppedContainedExtension;
        steppedContainedExtension.extend(range(2, 5, 2));
        if(!steppedContainedExtension.same_state(originalSteppedContainedExtension))
            throw exception("Extending with a contained progression reset iteration");

        range partiallyChangedExtension("[0:3][0:3]");
        for(int i = 0; i < 4; ++i)
            ++partiallyChangedExtension;
        partiallyChangedExtension.extend(range("[0:3][0:5]"));
        if(partiallyChangedExtension.index() != std::vector<int>{1, 0})
            throw exception("Multidimensional extension reset an unchanged dimension");

        range placeholder;
        placeholder.extend(1);
        range offsetRange(1, 6, 2);
        placeholder.extend(offsetRange);
        if(placeholder != offsetRange)
            throw exception("Extending a placeholder did not adopt the supplied range");
        placeholder.extend(range("[]"));
        if(placeholder != offsetRange)
            throw exception("Extending with a placeholder changed an existing range");

        range emptyReceiver(100, 100);
        emptyReceiver.extend(offsetRange);
        if(emptyReceiver != offsetRange)
            throw exception("Extending an empty dimension did not adopt a populated dimension");

        range populatedReceiver = offsetRange;
        ++populatedReceiver;
        const range advancedPopulatedReceiver = populatedReceiver;
        populatedReceiver.extend(range(100, 100));
        if(!populatedReceiver.same_state(advancedPopulatedReceiver))
            throw exception("Extending with an empty dimension changed a populated dimension");

        range firstEmpty(5, 5);
        const range originalFirstEmpty = firstEmpty;
        firstEmpty.extend(range(10, 10));
        if(!firstEmpty.same_state(originalFirstEmpty))
            throw exception("Extending two empty dimensions created a populated dimension");

        range descendingEmptyReceiver(5, 4);
        descendingEmptyReceiver.extend(offsetRange);
        if(descendingEmptyReceiver != offsetRange)
            throw exception("Extending a descending empty dimension did not adopt a populated dimension");

        range multidimensionalEmpty("[100:100][0:2]");
        range multidimensionalPopulated("[1:3][0:2]");
        multidimensionalEmpty.extend(multidimensionalPopulated);
        if(multidimensionalEmpty != multidimensionalPopulated)
            throw exception("Multidimensional extension did not replace an empty dimension");

        range fillSource(0, 5);
        range fillPlaceholder("[]");
        fillPlaceholder.fill(fillSource);
        if(fillPlaceholder != fillSource)
            throw exception("Filling a range did not replace a zero-step placeholder");

        range explicitEmpty(3, 3);
        const range originalExplicitEmpty = explicitEmpty;
        explicitEmpty.fill(fillSource);
        if(!explicitEmpty.same_state(originalExplicitEmpty))
            throw exception("Filling a range replaced an explicitly empty dimension");

        range descendingEmpty(3, 2);
        const range originalDescendingEmpty = descendingEmpty;
        descendingEmpty.fill(fillSource);
        if(!descendingEmpty.same_state(originalDescendingEmpty))
            throw exception("Filling a range replaced a descending empty dimension");

        range incomplete("[][0:2]");
        try
        {
            incomplete.fill(lowerRank);
            throw exception("Filling from a lower-rank range was not rejected");
        }
        catch(const std::invalid_argument &)
        {
        }

        auto checkAtomicOverflow = [](range value, const auto & mutation,
                                      const std::string & operation)
        {
            const range original = value;
            bool rejected = false;
            try
            {
                mutation(value);
            }
            catch(const std::overflow_error &)
            {
                rejected = true;
            }
            if(!rejected)
                throw exception(operation + " did not reject an overflowing range");
            if(!value.same_state(original))
                throw exception(operation + " changed the range after rejecting the operation");
        };

        auto checkConstructionOverflow = [](const auto & construction,
                                            const std::string & description)
        {
            try
            {
                construction();
                throw exception(description + " was not rejected");
            }
            catch(const std::overflow_error &)
            {
            }
        };

        const int minimum = std::numeric_limits<int>::min();
        const int maximum = std::numeric_limits<int>::max();
        checkAtomicOverflow(range(0, 1), [=](range & value)
        {
            value.push(minimum, maximum, 1);
        }, "push()");
        checkAtomicOverflow(range(0, 1), [=](range & value)
        {
            value.push_front(minimum, maximum, 1);
        }, "push_front()");
        checkAtomicOverflow(range(0, 1), [=](range & value)
        {
            value.set(0, minimum, maximum, 1);
        }, "set()");

        range distantRange(0, maximum, 1);
        checkAtomicOverflow(range(minimum, -1, 1), [&](range & value)
        {
            value.extend(distantRange);
        }, "extend()");
        checkAtomicOverflow(range(minimum, 0, maximum), [](range & value)
        {
            static_cast<void>(value.trim());
        }, "trim()");

        checkConstructionOverflow([=]()
        {
            static_cast<void>(range(maximum - 1, maximum, 2));
        }, "A range with an overflowing positive terminal cursor");
        checkConstructionOverflow([=]()
        {
            static_cast<void>(range(minimum, minimum + 1, -2));
        }, "A range with an overflowing negative terminal cursor");
        checkConstructionOverflow([=]()
        {
            static_cast<void>(range("[" + std::to_string(maximum - 1) + ":" +
                                    std::to_string(maximum) + ":2]"));
        }, "A parsed range with an overflowing terminal cursor");
        checkAtomicOverflow(range(0, 2), [=](range & value)
        {
            value.push(maximum - 1, maximum, 2);
        }, "push() with an overflowing terminal cursor");

        try
        {
            static_cast<void>(range().tail());
            throw exception("Taking the tail of an empty range was not rejected");
        }
        catch(const std::out_of_range &)
        {
        }

        if(!range(3).tail().empty())
            throw exception("The tail of a rank-one range was not empty");
        if(range("[]").tail().rank() != 0)
            throw exception("The tail of a zero-cardinality rank-one range was not rank zero");

        range multidimensionalTailSource("[1:4][2:7:2][5]");
        ++multidimensionalTailSource;
        range multidimensionalTail = multidimensionalTailSource.tail();
        if(multidimensionalTail.rank() != 2 ||
           multidimensionalTail.start(0) != 2 ||
           multidimensionalTail.stop(0) != 7 ||
           multidimensionalTail.step(0) != 2 ||
           multidimensionalTail.start(1) != 5 ||
           multidimensionalTail.stop(1) != 6 ||
           multidimensionalTail.step(1) != 1 ||
           multidimensionalTail.index() != std::vector<int>{4, 5})
            throw exception("A multidimensional tail did not preserve the remaining dimensions");

        range streamed("[1:4][2:7:2]");
        ++streamed;
        std::ostringstream rangeOutput;
        std::ostream * returnedStream = &(rangeOutput << "prefix " << streamed << " suffix");
        if(returnedStream != &rangeOutput || rangeOutput.str() != "prefix (1, 4) suffix")
            throw exception("Range stream insertion did not write to the supplied stream");

        auto checkMalformedRange = [](const std::string & text)
        {
            try
            {
                static_cast<void>(range(text));
                throw exception("Malformed range \"" + text + "\" was not rejected");
            }
            catch(const std::invalid_argument &)
            {
            }
        };
        checkMalformedRange("[1x]");
        checkMalformedRange("[0:5junk]");
        checkMalformedRange("[0:5:2step]");
        checkMalformedRange("[1]x[2]");
        checkMalformedRange("[1e2]");

        range explicitlySigned("[+1:+4:+1]");
        if(explicitlySigned.rank() != 1 || explicitlySigned.start(0) != 1 ||
           explicitlySigned.stop(0) != 4 || explicitlySigned.step(0) != 1)
            throw exception("Strict range parsing rejected valid explicitly signed integers");
        if(range("[ +1 : +4 : +1 ]") != explicitlySigned)
            throw exception("Strict range parsing rejected surrounding field whitespace");

        range equalDefinition(0, 5, 2);
        range advancedDefinition = equalDefinition;
        ++advancedDefinition;
        if(equalDefinition != advancedDefinition || equalDefinition == range(0, 6, 2))
            throw exception("Range equality did not compare definitions only");
        if(equalDefinition.same_state(advancedDefinition) ||
           !advancedDefinition.same_state(advancedDefinition))
            throw exception("Range state comparison did not include the iteration cursor");

        if(!(range(0, 6, 2) <= range(0, 6)) ||
           range(0, 6, 3) <= range(0, 6, 2) ||
           range(1, 6, 2) <= range(0, 6, 2))
            throw exception("Range subset comparison did not account for stepped membership");
        if(!(range(0, 6, -2) <= range(0, 6, 2)) ||
           !(range(2, 3, -100) <= range(0, 6, 2)))
            throw exception("Range subset comparison depended on traversal direction");

        range subset2D("[0:4:2][1:4:2]");
        range superset2D("[0:5][0:5]");
        if(!(subset2D <= superset2D) || subset2D <= range(0, 5))
            throw exception("Range subset comparison did not enforce compatible ranks");
        if(!(range() <= range(0, 5)) || !(range("[][0:2]") <= range(0, 5)) ||
           range(0, 5) <= range())
            throw exception("Range subset comparison did not treat empty ranges as empty sets");

        range prefix(0, 3);
        range * prefixResult = &(++prefix);
        if(prefixResult != &prefix || prefix.index()[0] != 1)
            throw exception("Prefix range increment did not return the advanced range");

        range postfix(0, 3);
        std::vector<int> postfixResult = postfix++;
        if(postfixResult != std::vector<int>{1} || postfix.index()[0] != 1)
            throw exception("Postfix range increment compatibility changed");

        std::cout << path_ << " RANGE SIZE TEST OK" << std::endl;
    }
};


class LargeFlattenSource : public Module
{
};


class FlattenOverflowSink : public Module
{
};


INSTALL_CLASS(DelaySequenceSource)
INSTALL_CLASS(DelayVectorSource)
INSTALL_CLASS(AsyncDelaySequenceSource)
INSTALL_CLASS(AsyncDelayHistorySink)
INSTALL_CLASS(DelayWindowSink)
INSTALL_CLASS(DelayFlatWindowSink)
INSTALL_CLASS(DelayStackedWindowSink)
INSTALL_CLASS(DelayedPropagationFailureSink)
INSTALL_CLASS(DelayPropagationBenchmarkSink)
INSTALL_CLASS(RangeBenchmarkModule)
INSTALL_CLASS(RangeSizeTestModule)
INSTALL_CLASS(LargeFlattenSource)
INSTALL_CLASS(FlattenOverflowSink)
