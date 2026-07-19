#include <algorithm>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
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
        if(empty.size() != 0 || empty.more())
            throw exception("Zero-increment range was not empty");

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
        checkAtomicOverflow(range(minimum, -1, 1), [&](range & value)
        {
            value |= distantRange;
        }, "operator|=()");

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

INSTALL_CLASS(DelaySequenceSource)
INSTALL_CLASS(DelayVectorSource)
INSTALL_CLASS(DelayWindowSink)
INSTALL_CLASS(DelayFlatWindowSink)
INSTALL_CLASS(DelayStackedWindowSink)
INSTALL_CLASS(DelayedPropagationFailureSink)
INSTALL_CLASS(DelayPropagationBenchmarkSink)
INSTALL_CLASS(RangeSizeTestModule)
