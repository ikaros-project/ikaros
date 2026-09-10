#include <algorithm>
#include <cmath>
#include <random>

#include "ikaros.h"

using namespace ikaros;

class SpikeGenerator: public Module
{
    enum Mode
    {
        poisson,
        regular,
        triggered,
    };

    enum TriggerMode
    {
        risingEdge,
        level,
    };

    parameter populationSize;
    parameter mode;
    parameter firingRate;
    parameter initialPhase;
    parameter refractoryPeriod;
    parameter triggerMode;
    parameter seed;

    matrix rate;
    matrix trigger;
    matrix enable;
    matrix reset;
    matrix spikes;
    matrix spikeCount;
    matrix outputFiringRate;
    matrix effectiveRate;
    matrix phase;

    matrix previousTrigger;
    std::mt19937 randomGenerator;

    void Init()
    {
        Bind(populationSize, "population_size");
        Bind(mode, "mode");
        Bind(firingRate, "firing_rate");
        Bind(initialPhase, "initial_phase");
        Bind(refractoryPeriod, "refractory_period");
        Bind(triggerMode, "trigger_mode");
        Bind(seed, "seed");

        Bind(rate, "RATE");
        Bind(trigger, "TRIGGER");
        Bind(enable, "ENABLE");
        Bind(reset, "RESET");
        Bind(spikes, "SPIKES");
        Bind(spikeCount, "SPIKE_COUNT");
        Bind(outputFiringRate, "FIRING_RATE");
        Bind(effectiveRate, "EFFECTIVE_RATE");
        Bind(phase, "PHASE");

        const int size = populationSize.as_int();
        ValidateSize(rate, size, "RATE");
        ValidateSize(trigger, size, "TRIGGER");
        ValidateSize(enable, size, "ENABLE");
        ValidateSize(reset, size, "RESET");
        ValidateSize(spikes, size, "SPIKES");
        ValidateSize(spikeCount, size, "SPIKE_COUNT");
        ValidateSize(outputFiringRate, size, "FIRING_RATE");
        ValidateSize(effectiveRate, size, "EFFECTIVE_RATE");
        ValidateSize(phase, size, "PHASE");

        if(GetTickDuration() <= 0)
            throw exception("SpikeGenerator: tick_duration must be positive.", path_);

        if(seed.as_int() < 0)
            randomGenerator.seed(std::random_device{}());
        else
            randomGenerator.seed(static_cast<std::mt19937::result_type>(seed.as_int()));

        previousTrigger = matrix(size);
        previousTrigger = 0.0f;
        spikes = 0.0f;
        spikeCount = 0.0f;
        outputFiringRate = 0.0f;
        effectiveRate = 0.0f;
        phase = initialPhase.as_float();
    }

    void Tick()
    {
        const double tickDuration = GetTickDuration();
        const double parameterRate = firingRate.as_double() / tickDuration;
        spikes = 0.0f;
        spikeCount = 0.0f;
        outputFiringRate = 0.0f;

        for(int i = 0; i < spikes.size(); ++i)
        {
            const bool resetRequested = !reset.empty() && reset(i) > 0.0f;
            if(resetRequested)
            {
                phase(i) = initialPhase.as_float();
                previousTrigger(i) = 0.0f;
            }

            const double rateHz = std::max(0.0, rate.empty() ? parameterRate : double(rate(i)));
            effectiveRate(i) = static_cast<float>(rateHz);
            const bool enabled = enable.empty() || enable(i) > 0.0f;

            if(enabled && !resetRequested)
            {
                if(mode.as_int() == poisson)
                    spikeCount(i) = static_cast<float>(PoissonCount(rateHz, tickDuration));
                else if(mode.as_int() == regular)
                    spikeCount(i) = static_cast<float>(RegularCount(i, rateHz, tickDuration));
                else
                    spikeCount(i) = static_cast<float>(TriggeredCount(i));
            }

            if(mode.as_int() == triggered && !resetRequested)
                previousTrigger(i) = trigger.empty() ? 0.0f : trigger(i);

            spikes(i) = spikeCount(i) > 0.0f ? 1.0f : 0.0f;
            outputFiringRate(i) = static_cast<float>(spikeCount(i) / tickDuration);
        }
    }

    void ValidateSize(const matrix & value, int expected, const std::string & name)
    {
        if(!value.empty() && value.size() != expected)
            throw exception("SpikeGenerator: " + name + " must contain population_size elements.", path_);
    }

    int PoissonCount(double rateHz, double tickDuration)
    {
        if(rateHz <= 0.0)
            return 0;

        std::poisson_distribution<int> distribution(rateHz * tickDuration);
        int count = distribution(randomGenerator);
        const double refractory = refractoryPeriod.as_double();
        if(refractory > 0.0)
        {
            const int maximumCount = 1 + static_cast<int>(std::floor(tickDuration / refractory));
            count = std::min(count, maximumCount);
        }
        return count;
    }

    int RegularCount(int index, double rateHz, double tickDuration)
    {
        if(rateHz <= 0.0)
            return 0;

        const double advancedPhase = phase(index) + rateHz * tickDuration;
        const int count = static_cast<int>(std::floor(advancedPhase));
        phase(index) = static_cast<float>(advancedPhase - count);
        return count;
    }

    int TriggeredCount(int index) const
    {
        if(trigger.empty())
            return 0;
        if(triggerMode.as_int() == level)
            return trigger(index) > 0.0f ? 1 : 0;
        return trigger(index) > 0.0f && previousTrigger(index) <= 0.0f ? 1 : 0;
    }
};

INSTALL_CLASS(SpikeGenerator)
