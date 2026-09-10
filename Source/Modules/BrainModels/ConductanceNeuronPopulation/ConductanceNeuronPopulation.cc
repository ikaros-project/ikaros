#include <algorithm>
#include <cmath>

#include "ikaros.h"

using namespace ikaros;

class ConductanceNeuronPopulation: public Module
{
    enum Model
    {
        hodgkinHuxley,
        morrisLecar,
    };

    parameter populationSize;
    parameter model;
    parameter initialVoltage;
    parameter spikeThreshold;
    parameter excitatoryReversalPotential;
    parameter inhibitoryReversalPotential;
    parameter maximumInternalStep;

    parameter hhCapacitanceDensity;
    parameter hhSodiumConductance;
    parameter hhPotassiumConductance;
    parameter hhLeakConductance;
    parameter hhSodiumReversalPotential;
    parameter hhPotassiumReversalPotential;
    parameter hhLeakReversalPotential;

    parameter mlCapacitanceDensity;
    parameter mlCalciumConductance;
    parameter mlPotassiumConductance;
    parameter mlLeakConductance;
    parameter mlCalciumReversalPotential;
    parameter mlPotassiumReversalPotential;
    parameter mlLeakReversalPotential;
    parameter mlActivationMidpoint;
    parameter mlActivationSlope;
    parameter mlRecoveryMidpoint;
    parameter mlRecoverySlope;
    parameter mlRecoveryTimeConstant;

    matrix current;
    matrix excitatoryConductance;
    matrix inhibitoryConductance;
    matrix reset;
    matrix spikes;
    matrix spikeCount;
    matrix firingRate;
    matrix voltage;
    matrix recovery;

    matrix sodiumActivation;
    matrix sodiumInactivation;

    void Init()
    {
        BindParameters();
        BindPorts();

        const int size = populationSize.as_int();
        ValidateSize(current, size, "CURRENT");
        ValidateSize(excitatoryConductance, size, "EXCITATORY_CONDUCTANCE");
        ValidateSize(inhibitoryConductance, size, "INHIBITORY_CONDUCTANCE");
        ValidateSize(reset, size, "RESET");
        ValidateSize(spikes, size, "SPIKES");
        ValidateSize(spikeCount, size, "SPIKE_COUNT");
        ValidateSize(firingRate, size, "FIRING_RATE");
        ValidateSize(voltage, size, "VOLTAGE");
        ValidateSize(recovery, size, "RECOVERY");

        if(GetTickDuration() <= 0)
            throw exception("ConductanceNeuronPopulation: tick_duration must be positive.", path_);
        if(GetTickDuration() > 0.01)
            Warning("ConductanceNeuronPopulation uses binned spike output and sample-held inputs because tick_duration exceeds 10 ms.");

        sodiumActivation = matrix(size);
        sodiumInactivation = matrix(size);
        voltage = initialVoltage.as_float();
        spikes = 0.0f;
        spikeCount = 0.0f;
        firingRate = 0.0f;

        for(int i = 0; i < size; ++i)
            ResetState(i);
    }

    void Tick()
    {
        const double tickDuration = GetTickDuration();
        const int steps = std::max(1, static_cast<int>(std::ceil(
                                          tickDuration / maximumInternalStep.as_double())));
        const double dt = tickDuration / steps;

        spikes = 0.0f;
        spikeCount = 0.0f;

        for(int i = 0; i < voltage.size(); ++i)
        {
            if(!reset.empty() && reset(i) > 0.0f)
                ResetState(i);

            const double appliedCurrent = Value(current, i);
            const double excitatory = Value(excitatoryConductance, i);
            const double inhibitory = Value(inhibitoryConductance, i);

            for(int step = 0; step < steps; ++step)
            {
                const double previousVoltage = voltage(i);
                if(model.as_int() == hodgkinHuxley)
                    AdvanceHodgkinHuxley(i, appliedCurrent, excitatory, inhibitory, dt);
                else
                    AdvanceMorrisLecar(i, appliedCurrent, excitatory, inhibitory, dt);

                if(previousVoltage < spikeThreshold.as_double() && voltage(i) >= spikeThreshold.as_double())
                    spikeCount(i) += 1.0f;
            }

            spikes(i) = spikeCount(i) > 0.0f ? 1.0f : 0.0f;
            firingRate(i) = static_cast<float>(spikeCount(i) / tickDuration);
        }
    }

    void BindParameters()
    {
        Bind(populationSize, "population_size");
        Bind(model, "model");
        Bind(initialVoltage, "initial_voltage");
        Bind(spikeThreshold, "spike_threshold");
        Bind(excitatoryReversalPotential, "excitatory_reversal_potential");
        Bind(inhibitoryReversalPotential, "inhibitory_reversal_potential");
        Bind(maximumInternalStep, "maximum_internal_step");
        Bind(hhCapacitanceDensity, "hh_capacitance_density");
        Bind(hhSodiumConductance, "hh_sodium_conductance");
        Bind(hhPotassiumConductance, "hh_potassium_conductance");
        Bind(hhLeakConductance, "hh_leak_conductance");
        Bind(hhSodiumReversalPotential, "hh_sodium_reversal_potential");
        Bind(hhPotassiumReversalPotential, "hh_potassium_reversal_potential");
        Bind(hhLeakReversalPotential, "hh_leak_reversal_potential");
        Bind(mlCapacitanceDensity, "ml_capacitance_density");
        Bind(mlCalciumConductance, "ml_calcium_conductance");
        Bind(mlPotassiumConductance, "ml_potassium_conductance");
        Bind(mlLeakConductance, "ml_leak_conductance");
        Bind(mlCalciumReversalPotential, "ml_calcium_reversal_potential");
        Bind(mlPotassiumReversalPotential, "ml_potassium_reversal_potential");
        Bind(mlLeakReversalPotential, "ml_leak_reversal_potential");
        Bind(mlActivationMidpoint, "ml_activation_midpoint");
        Bind(mlActivationSlope, "ml_activation_slope");
        Bind(mlRecoveryMidpoint, "ml_recovery_midpoint");
        Bind(mlRecoverySlope, "ml_recovery_slope");
        Bind(mlRecoveryTimeConstant, "ml_recovery_time_constant");
    }

    void BindPorts()
    {
        Bind(current, "CURRENT");
        Bind(excitatoryConductance, "EXCITATORY_CONDUCTANCE");
        Bind(inhibitoryConductance, "INHIBITORY_CONDUCTANCE");
        Bind(reset, "RESET");
        Bind(spikes, "SPIKES");
        Bind(spikeCount, "SPIKE_COUNT");
        Bind(firingRate, "FIRING_RATE");
        Bind(voltage, "VOLTAGE");
        Bind(recovery, "RECOVERY");
    }

    void ValidateSize(const matrix & value, int expected, const std::string & name)
    {
        if(!value.empty() && value.size() != expected)
            throw exception("ConductanceNeuronPopulation: " + name + " must contain population_size elements.", path_);
    }

    float Value(const matrix & value, int index) const
    {
        return value.empty() ? 0.0f : value(index);
    }

    void ResetState(int index)
    {
        const double v = initialVoltage.as_double();
        voltage(index) = static_cast<float>(v);

        if(model.as_int() == hodgkinHuxley)
        {
            const double alphaM = 0.1 * VTrap(v + 40.0, 10.0);
            const double betaM = 4.0 * std::exp(-(v + 65.0) / 18.0);
            const double alphaH = 0.07 * std::exp(-(v + 65.0) / 20.0);
            const double betaH = 1.0 / (1.0 + std::exp(-(v + 35.0) / 10.0));
            const double alphaN = 0.01 * VTrap(v + 55.0, 10.0);
            const double betaN = 0.125 * std::exp(-(v + 65.0) / 80.0);
            sodiumActivation(index) = static_cast<float>(alphaM / (alphaM + betaM));
            sodiumInactivation(index) = static_cast<float>(alphaH / (alphaH + betaH));
            recovery(index) = static_cast<float>(alphaN / (alphaN + betaN));
        }
        else
        {
            sodiumActivation(index) = 0.0f;
            sodiumInactivation(index) = 0.0f;
            recovery(index) = static_cast<float>(MorrisLecarRecoverySteadyState(v));
        }
    }

    double VTrap(double x, double scale) const
    {
        if(std::abs(x / scale) < 1e-6)
            return scale * (1.0 + x / (2.0 * scale));
        return x / (1.0 - std::exp(-x / scale));
    }

    double SynapticCurrent(double v, double excitatory, double inhibitory) const
    {
        return excitatory * (excitatoryReversalPotential.as_double() - v) +
               inhibitory * (inhibitoryReversalPotential.as_double() - v);
    }

    void AdvanceHodgkinHuxley(int index, double appliedCurrent, double excitatory,
                              double inhibitory, double dt)
    {
        double v = voltage(index);
        double m = sodiumActivation(index);
        double h = sodiumInactivation(index);
        double n = recovery(index);
        RK4HodgkinHuxley(v, m, h, n, appliedCurrent, excitatory, inhibitory, dt);
        voltage(index) = static_cast<float>(v);
        sodiumActivation(index) = static_cast<float>(std::clamp(m, 0.0, 1.0));
        sodiumInactivation(index) = static_cast<float>(std::clamp(h, 0.0, 1.0));
        recovery(index) = static_cast<float>(std::clamp(n, 0.0, 1.0));
    }

    void RK4HodgkinHuxley(double & v, double & m, double & h, double & n,
                          double currentValue, double excitatory, double inhibitory, double dt)
    {
        double k1v, k1m, k1h, k1n;
        HodgkinHuxleyDerivatives(v, m, h, n, currentValue, excitatory, inhibitory, k1v, k1m, k1h, k1n);
        double k2v, k2m, k2h, k2n;
        HodgkinHuxleyDerivatives(v + 0.5 * dt * k1v, m + 0.5 * dt * k1m,
                                h + 0.5 * dt * k1h, n + 0.5 * dt * k1n,
                                currentValue, excitatory, inhibitory, k2v, k2m, k2h, k2n);
        double k3v, k3m, k3h, k3n;
        HodgkinHuxleyDerivatives(v + 0.5 * dt * k2v, m + 0.5 * dt * k2m,
                                h + 0.5 * dt * k2h, n + 0.5 * dt * k2n,
                                currentValue, excitatory, inhibitory, k3v, k3m, k3h, k3n);
        double k4v, k4m, k4h, k4n;
        HodgkinHuxleyDerivatives(v + dt * k3v, m + dt * k3m, h + dt * k3h, n + dt * k3n,
                                currentValue, excitatory, inhibitory, k4v, k4m, k4h, k4n);

        v += dt * (k1v + 2.0 * k2v + 2.0 * k3v + k4v) / 6.0;
        m += dt * (k1m + 2.0 * k2m + 2.0 * k3m + k4m) / 6.0;
        h += dt * (k1h + 2.0 * k2h + 2.0 * k3h + k4h) / 6.0;
        n += dt * (k1n + 2.0 * k2n + 2.0 * k3n + k4n) / 6.0;
    }

    void HodgkinHuxleyDerivatives(double v, double m, double h, double n, double appliedCurrent,
                                  double excitatory, double inhibitory, double & dv, double & dm,
                                  double & dh, double & dn)
    {
        const double sodium = hhSodiumConductance.as_double() * m * m * m * h *
                              (v - hhSodiumReversalPotential.as_double());
        const double potassium = hhPotassiumConductance.as_double() * n * n * n * n *
                                 (v - hhPotassiumReversalPotential.as_double());
        const double leak = hhLeakConductance.as_double() *
                            (v - hhLeakReversalPotential.as_double());
        dv = 1000.0 * (appliedCurrent + SynapticCurrent(v, excitatory, inhibitory) -
                       sodium - potassium - leak) / hhCapacitanceDensity.as_double();

        const double alphaM = 0.1 * VTrap(v + 40.0, 10.0);
        const double betaM = 4.0 * std::exp(-(v + 65.0) / 18.0);
        const double alphaH = 0.07 * std::exp(-(v + 65.0) / 20.0);
        const double betaH = 1.0 / (1.0 + std::exp(-(v + 35.0) / 10.0));
        const double alphaN = 0.01 * VTrap(v + 55.0, 10.0);
        const double betaN = 0.125 * std::exp(-(v + 65.0) / 80.0);
        dm = 1000.0 * (alphaM * (1.0 - m) - betaM * m);
        dh = 1000.0 * (alphaH * (1.0 - h) - betaH * h);
        dn = 1000.0 * (alphaN * (1.0 - n) - betaN * n);
    }

    void AdvanceMorrisLecar(int index, double appliedCurrent, double excitatory,
                            double inhibitory, double dt)
    {
        double v = voltage(index);
        double w = recovery(index);
        RK4MorrisLecar(v, w, appliedCurrent, excitatory, inhibitory, dt);
        voltage(index) = static_cast<float>(v);
        recovery(index) = static_cast<float>(std::clamp(w, 0.0, 1.0));
    }

    void RK4MorrisLecar(double & v, double & w, double currentValue,
                        double excitatory, double inhibitory, double dt)
    {
        double k1v, k1w;
        MorrisLecarDerivatives(v, w, currentValue, excitatory, inhibitory, k1v, k1w);
        double k2v, k2w;
        MorrisLecarDerivatives(v + 0.5 * dt * k1v, w + 0.5 * dt * k1w,
                              currentValue, excitatory, inhibitory, k2v, k2w);
        double k3v, k3w;
        MorrisLecarDerivatives(v + 0.5 * dt * k2v, w + 0.5 * dt * k2w,
                              currentValue, excitatory, inhibitory, k3v, k3w);
        double k4v, k4w;
        MorrisLecarDerivatives(v + dt * k3v, w + dt * k3w,
                              currentValue, excitatory, inhibitory, k4v, k4w);
        v += dt * (k1v + 2.0 * k2v + 2.0 * k3v + k4v) / 6.0;
        w += dt * (k1w + 2.0 * k2w + 2.0 * k3w + k4w) / 6.0;
    }

    void MorrisLecarDerivatives(double v, double w, double appliedCurrent, double excitatory,
                                double inhibitory, double & dv, double & dw)
    {
        const double activation = 0.5 * (1.0 + std::tanh(
                                                  (v - mlActivationMidpoint.as_double()) /
                                                  mlActivationSlope.as_double()));
        const double calcium = mlCalciumConductance.as_double() * activation *
                               (v - mlCalciumReversalPotential.as_double());
        const double potassium = mlPotassiumConductance.as_double() * w *
                                 (v - mlPotassiumReversalPotential.as_double());
        const double leak = mlLeakConductance.as_double() *
                            (v - mlLeakReversalPotential.as_double());
        dv = 1000.0 * (appliedCurrent + SynapticCurrent(v, excitatory, inhibitory) -
                       calcium - potassium - leak) / mlCapacitanceDensity.as_double();

        const double voltageFactor = std::cosh((v - mlRecoveryMidpoint.as_double()) /
                                               (2.0 * mlRecoverySlope.as_double()));
        dw = voltageFactor * (MorrisLecarRecoverySteadyState(v) - w) /
             mlRecoveryTimeConstant.as_double();
    }

    double MorrisLecarRecoverySteadyState(double v) const
    {
        return 0.5 * (1.0 + std::tanh((v - mlRecoveryMidpoint.as_double()) /
                                     mlRecoverySlope.as_double()));
    }
};

INSTALL_CLASS(ConductanceNeuronPopulation)
