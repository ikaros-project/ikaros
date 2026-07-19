#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "ikaros.h"

using namespace ikaros;

namespace
{
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
