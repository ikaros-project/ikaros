#include <algorithm>
#include <cmath>

#include "ikaros.h"

using namespace ikaros;

class IntegrateAndFirePopulation: public Module
{
    enum Model
    {
        lif,
        eif,
        adex,
        qif,
    };

    parameter populationSize;
    parameter model;
    parameter membraneTimeConstant;
    parameter membraneResistance;
    parameter restingPotential;
    parameter threshold;
    parameter resetPotential;
    parameter initialPotential;
    parameter refractoryPeriod;
    parameter slopeFactor;
    parameter adaptationTimeConstant;
    parameter subthresholdAdaptation;
    parameter spikeAdaptation;
    parameter quadraticGain;
    parameter maximumInternalStep;

    matrix input;
    matrix excitation;
    matrix inhibition;
    matrix reset;
    matrix spikes;
    matrix spikeCount;
    matrix firingRate;
    matrix voltage;
    matrix adaptation;

    matrix refractoryRemaining;

    void Init()
    {
        Bind(populationSize, "population_size");
        Bind(model, "model");
        Bind(membraneTimeConstant, "membrane_time_constant");
        Bind(membraneResistance, "membrane_resistance");
        Bind(restingPotential, "resting_potential");
        Bind(threshold, "threshold");
        Bind(resetPotential, "reset_potential");
        Bind(initialPotential, "initial_potential");
        Bind(refractoryPeriod, "refractory_period");
        Bind(slopeFactor, "slope_factor");
        Bind(adaptationTimeConstant, "adaptation_time_constant");
        Bind(subthresholdAdaptation, "subthreshold_adaptation");
        Bind(spikeAdaptation, "spike_adaptation");
        Bind(quadraticGain, "quadratic_gain");
        Bind(maximumInternalStep, "maximum_internal_step");

        Bind(input, "INPUT");
        Bind(excitation, "EXCITATION");
        Bind(inhibition, "INHIBITION");
        Bind(reset, "RESET");
        Bind(spikes, "SPIKES");
        Bind(spikeCount, "SPIKE_COUNT");
        Bind(firingRate, "FIRING_RATE");
        Bind(voltage, "VOLTAGE");
        Bind(adaptation, "ADAPTATION");

        const int size = populationSize.as_int();
        ValidateSize(input, size, "INPUT");
        ValidateSize(excitation, size, "EXCITATION");
        ValidateSize(inhibition, size, "INHIBITION");
        ValidateSize(reset, size, "RESET");
        ValidateSize(spikes, size, "SPIKES");
        ValidateSize(spikeCount, size, "SPIKE_COUNT");
        ValidateSize(firingRate, size, "FIRING_RATE");
        ValidateSize(voltage, size, "VOLTAGE");
        ValidateSize(adaptation, size, "ADAPTATION");

        if(GetTickDuration() <= 0)
            throw exception("IntegrateAndFirePopulation: tick_duration must be positive.", path_);
        if(GetTickDuration() > 0.01)
            Warning("IntegrateAndFirePopulation uses binned spike output because tick_duration exceeds 10 ms.");

        refractoryRemaining = matrix(size);
        refractoryRemaining = 0.0f;
        voltage = initialPotential.as_float();
        adaptation = 0.0f;
        spikes = 0.0f;
        spikeCount = 0.0f;
        firingRate = 0.0f;
    }

    void Tick()
    {
        const double tickDuration = GetTickDuration();
        const double maximumStep = maximumInternalStep.as_double();
        const int steps = std::max(1, static_cast<int>(std::ceil(tickDuration / maximumStep)));
        const double dt = tickDuration / steps;

        spikes = 0.0f;
        spikeCount = 0.0f;

        for(int i = 0; i < voltage.size(); ++i)
        {
            if(!reset.empty() && reset(i) > 0.0f)
            {
                voltage(i) = initialPotential.as_float();
                adaptation(i) = 0.0f;
                refractoryRemaining(i) = 0.0f;
            }

            const double current = Value(input, i) + Value(excitation, i) - Value(inhibition, i);

            for(int step = 0; step < steps; ++step)
                Advance(i, current, dt);

            spikes(i) = spikeCount(i) > 0.0f ? 1.0f : 0.0f;
            firingRate(i) = static_cast<float>(spikeCount(i) / tickDuration);
        }
    }

    void ValidateSize(const matrix & value, int expected, const std::string & name)
    {
        if(!value.empty() && value.size() != expected)
            throw exception("IntegrateAndFirePopulation: " + name + " must contain population_size elements.", path_);
    }

    float Value(const matrix & value, int index) const
    {
        return value.empty() ? 0.0f : value(index);
    }

    void Advance(int index, double current, double dt)
    {
        if(refractoryRemaining(index) > 0.0f)
        {
            refractoryRemaining(index) = static_cast<float>(std::max(0.0, refractoryRemaining(index) - dt));
            voltage(index) = resetPotential.as_float();
            AdvanceAdaptation(index, voltage(index), dt);
            return;
        }

        double v = voltage(index);
        double w = adaptation(index);

        if(model.as_int() == lif)
        {
            const double target = restingPotential.as_double() + membraneResistance.as_double() * current;
            const double decay = std::exp(-dt / membraneTimeConstant.as_double());
            v = target + (v - target) * decay;
        }
        else
        {
            RungeKutta(v, w, current, dt);
        }

        if(v >= threshold.as_double())
        {
            spikeCount(index) += 1.0f;
            v = resetPotential.as_double();
            refractoryRemaining(index) = refractoryPeriod.as_float();
            if(model.as_int() == adex)
                w += spikeAdaptation.as_double();
        }

        voltage(index) = static_cast<float>(v);
        adaptation(index) = model.as_int() == adex ? static_cast<float>(w) : 0.0f;
    }

    void RungeKutta(double & v, double & w, double current, double dt) const
    {
        double dv1;
        double dw1;
        Derivatives(v, w, current, dv1, dw1);

        double dv2;
        double dw2;
        Derivatives(v + 0.5 * dt * dv1, w + 0.5 * dt * dw1, current, dv2, dw2);

        double dv3;
        double dw3;
        Derivatives(v + 0.5 * dt * dv2, w + 0.5 * dt * dw2, current, dv3, dw3);

        double dv4;
        double dw4;
        Derivatives(v + dt * dv3, w + dt * dw3, current, dv4, dw4);

        v += dt * (dv1 + 2.0 * dv2 + 2.0 * dv3 + dv4) / 6.0;
        w += dt * (dw1 + 2.0 * dw2 + 2.0 * dw3 + dw4) / 6.0;
    }

    void Derivatives(double v, double w, double current, double & dv, double & dw) const
    {
        const double rest = restingPotential.as_double();
        const double resistance = membraneResistance.as_double();
        const double capacitance = 1000.0 * membraneTimeConstant.as_double() / resistance;
        double membraneCurrent = (rest - v) / resistance + current;

        if(model.as_int() == eif || model.as_int() == adex)
        {
            const double slope = slopeFactor.as_double();
            const double exponent = std::min(20.0, (v - threshold.as_double()) / slope);
            membraneCurrent += slope * std::exp(exponent) / resistance;
        }
        else if(model.as_int() == qif)
        {
            membraneCurrent += quadraticGain.as_double() * (v - rest) * (v - threshold.as_double());
        }

        if(model.as_int() == adex)
            membraneCurrent -= w;

        dv = 1000.0 * membraneCurrent / capacitance;
        dw = model.as_int() == adex
                 ? (subthresholdAdaptation.as_double() * (v - rest) / 1000.0 - w) /
                       adaptationTimeConstant.as_double()
                 : 0.0;
    }

    void AdvanceAdaptation(int index, double v, double dt)
    {
        if(model.as_int() != adex)
            return;

        const double target = subthresholdAdaptation.as_double() *
                              (v - restingPotential.as_double()) / 1000.0;
        const double decay = std::exp(-dt / adaptationTimeConstant.as_double());
        adaptation(index) = static_cast<float>(target + (adaptation(index) - target) * decay);
    }
};

INSTALL_CLASS(IntegrateAndFirePopulation)
