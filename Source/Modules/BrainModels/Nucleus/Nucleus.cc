#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>

#include "ikaros.h"


using namespace ikaros;


class Nucleus: public Module
{
    enum class BurstPhase
    {
        integrating,
        active,
        refractory
    };

    parameter   alpha;          // constant drive
    parameter   beta;           // excitation gain 
    parameter   gamma;           // inhibition gain
    parameter   delta;          // relative leak strength
    parameter   psi;            // shunting weight
    parameter   sigma;          // continuous noise amplitude
    parameter   randomSeed;     // random seed for noise
    parameter   theta;          // threshold for output
    parameter   timeConstant;   // state time constant
    parameter   legacyEpsilon;  // deprecated update rate
    parameter   scale_inputs;   // use average instead of sum of inputs
    parameter   activation_function; // output activation option
    parameter   burstDuration;  // active burst-envelope duration
    parameter   legacyBurstTime;// deprecated burst duration
    parameter   refractoryPeriod;
    parameter   initialState;
    parameter   resetLevel;
    parameter   burstLevel;
    parameter   homeostasis;
    parameter   homeostaticTarget;
    parameter   activityTimeConstant;
    parameter   homeostaticTimeConstant;
    parameter   initialHomeostaticGain;
    parameter   minimumHomeostaticGain;
    parameter   maximumHomeostaticGain;
    parameter   output_offset;  // offset for output, default is 0
    parameter   output_scale;   // scaling of output, default is 1

    matrix      excitation; 
    matrix      inhibition;
    matrix      shunting_inhibition;
    matrix      x;              // internal state
    matrix      output;
    matrix      homeostaticGainOutput;
    matrix      averageActivityOutput;

    double      homeostaticGain = 1;
    double      averageActivity = 1;

    BurstPhase burstPhase = BurstPhase::integrating;
    double burstEndTime = 0;
    double refractoryEndTime = 0;
    std::mt19937 gaussianGenerator;
    std::normal_distribution<float> gaussianDistribution;
    bool useLegacyEpsilon = false;
    bool useLegacyBurstTime = false;
    bool warnedNegativeShuntingInput = false;
    bool warnedNonFiniteInput = false;
    bool warnedNonFiniteResult = false;
    float lastFiniteState = 0;
    float lastFiniteOutput = 0;

    float
    TransformOutput(float activation) const
    {
        return output_offset.as_float() + output_scale.as_float() * activation;
    }


    void
    PublishHomeostaticState()
    {
        const double effectiveGain = homeostasis.as_bool() ? homeostaticGain : 1.0;
        homeostaticGainOutput(0) = static_cast<float>(effectiveGain);
        averageActivityOutput(0) = static_cast<float>(averageActivity);
    }


    void
    UpdateHomeostasis(double activity)
    {
        if(homeostasis.as_bool())
        {
            const double activityRelaxation = -std::expm1(
                -GetTickDuration() / activityTimeConstant.as_double());
            averageActivity += activityRelaxation * (activity - averageActivity);

            const double relativeError =
                (homeostaticTarget.as_double() - averageActivity) /
                homeostaticTarget.as_double();
            const double logGainStep =
                GetTickDuration() / homeostaticTimeConstant.as_double() * relativeError;
            homeostaticGain = std::clamp(
                homeostaticGain * std::exp(logGainStep),
                minimumHomeostaticGain.as_double(),
                maximumHomeostaticGain.as_double());
        }

        PublishHomeostaticState();
    }


    void
    IntegrateState(float E, float I, float S)
    {
        float & x_value = x(0);
        const double adaptiveGain = homeostasis.as_bool() ? homeostaticGain : 1.0;
        float inputDrive = alpha + beta * adaptiveGain * (1/(1+psi*S)) * E - gamma * I;
        float normalizedStep = useLegacyEpsilon
                                   ? legacyEpsilon.as_float() * GetTickDuration()
                                   : GetTickDuration() / timeConstant.as_float();
        float leakStrength = delta.as_float();
        float noiseVarianceFactor;

        if(std::abs(leakStrength) < 1e-6f)
        {
            x_value += normalizedStep * inputDrive;
            noiseVarianceFactor = normalizedStep;
        }
        else
        {
            float relaxation = -std::expm1(-leakStrength * normalizedStep);
            x_value += relaxation * (inputDrive / leakStrength - x_value);
            noiseVarianceFactor = -std::expm1(-2 * leakStrength * normalizedStep) /
                                  (2 * leakStrength);
        }

        x_value += sample_normal_distribution(
            gaussianGenerator, gaussianDistribution, 0,
            sigma.as_float() * std::sqrt(noiseVarianceFactor));
    }

    void Init() override
    {
        Bind(alpha, "alpha");
        Bind(beta, "beta");
        Bind(gamma, "gamma");
        Bind(delta, "delta");
        Bind(psi, "psi");
        Bind(sigma, "sigma");
        Bind(randomSeed, "seed");
        Bind(theta, "theta");
        const bool hasTimeConstant = KeyExists("time_constant");
        const bool hasLegacyEpsilon = KeyExists("epsilon");
        Bind(timeConstant, "time_constant");
        Bind(legacyEpsilon, "epsilon");

        useLegacyEpsilon = hasLegacyEpsilon && !hasTimeConstant;
        if(hasLegacyEpsilon && hasTimeConstant)
            Warning("Nucleus ignores deprecated epsilon when time_constant is explicitly set.");
        else if(hasLegacyEpsilon)
            Warning("Nucleus parameter epsilon is deprecated; use time_constant=1/epsilon instead.");


        Bind(scale_inputs, "scale_inputs");
        Bind(activation_function, "activation_function");

        const bool hasBurstDuration = KeyExists("burst_duration");
        const bool hasLegacyBurstTime = KeyExists("burst_time");
        Bind(burstDuration, "burst_duration");
        Bind(legacyBurstTime, "burst_time");
        Bind(refractoryPeriod, "refractory_period");
        Bind(initialState, "initial_state");
        Bind(resetLevel, "reset_level");
        Bind(burstLevel, "burst_level");
        Bind(homeostasis, "homeostasis");
        Bind(homeostaticTarget, "homeostatic_target");
        Bind(activityTimeConstant, "activity_time_constant");
        Bind(homeostaticTimeConstant, "homeostatic_time_constant");
        Bind(initialHomeostaticGain, "initial_homeostatic_gain");
        Bind(minimumHomeostaticGain, "minimum_homeostatic_gain");
        Bind(maximumHomeostaticGain, "maximum_homeostatic_gain");

        if(homeostaticTarget.as_double() <= 0)
            throw exception("Nucleus homeostatic_target must be greater than zero.");
        if(activityTimeConstant.as_double() <= 0)
            throw exception("Nucleus activity_time_constant must be greater than zero.");
        if(homeostaticTimeConstant.as_double() <= 0)
            throw exception("Nucleus homeostatic_time_constant must be greater than zero.");
        if(minimumHomeostaticGain.as_double() <= 0)
            throw exception("Nucleus minimum_homeostatic_gain must be greater than zero.");
        if(maximumHomeostaticGain.as_double() < minimumHomeostaticGain.as_double())
            throw exception("Nucleus maximum_homeostatic_gain must be at least minimum_homeostatic_gain.");
        if(initialHomeostaticGain.as_double() < minimumHomeostaticGain.as_double() ||
           initialHomeostaticGain.as_double() > maximumHomeostaticGain.as_double())
            throw exception("Nucleus initial_homeostatic_gain must lie within the configured gain limits.");

        useLegacyBurstTime = hasLegacyBurstTime && !hasBurstDuration;
        if(hasLegacyBurstTime && hasBurstDuration)
            Warning("Nucleus ignores deprecated burst_time when burst_duration is explicitly set.");
        else if(hasLegacyBurstTime)
            Warning("Nucleus parameter burst_time is deprecated; use burst_duration instead.");

        Bind(output_offset, "output_offset");
        Bind(output_scale, "output_scale");     

 
        Bind(excitation, "EXCITATION");
        Bind(inhibition, "INHIBITION");
        Bind(shunting_inhibition, "SHUNTING_INHIBITION");
        Bind(x, "X");
        Bind(output, "OUTPUT");
        Bind(homeostaticGainOutput, "HOMEOSTATIC_GAIN");
        Bind(averageActivityOutput, "AVERAGE_ACTIVITY");
        Bind(homeostaticGain, "homeostatic_gain");
        Bind(averageActivity, "average_activity");

        const int seed = randomSeed.as_int();
        if(seed < 0)
            gaussianGenerator.seed(std::random_device{}());
        else
            gaussianGenerator.seed(static_cast<std::mt19937::result_type>(seed));

        Reset();
    }


    void Reset() override
    {
        Component::Reset();
        burstPhase = BurstPhase::integrating;
        burstEndTime = 0;
        refractoryEndTime = 0;
        x(0) = initialState.as_float();
        output(0) = TransformOutput(0);
        homeostaticGain = initialHomeostaticGain.as_double();
        averageActivity = homeostaticTarget.as_double();
        PublishHomeostaticState();
        lastFiniteState = x(0);
        lastFiniteOutput = output(0);
    }

    
    void Tick() override
    {
        const double currentTime = GetTime();
        const double timingTolerance = GetTickDuration() * 1e-6;
        if(burstPhase == BurstPhase::active)
        {
            if(currentTime < burstEndTime - timingTolerance)
            {
                output(0) = TransformOutput(burstLevel.as_float());
                UpdateHomeostasis(burstLevel.as_double());
                lastFiniteState = x(0);
                lastFiniteOutput = output(0);
                return;
            }

            burstPhase = BurstPhase::refractory;
            refractoryEndTime = burstEndTime + refractoryPeriod.as_float();
        }

        if(burstPhase == BurstPhase::refractory &&
           currentTime >= refractoryEndTime - timingTolerance)
            burstPhase = BurstPhase::integrating;
    
        float E = 0;
        float I = 0;
        float S = 0;

        if(scale_inputs)
        {
            E = excitation.average();
            I = inhibition.average();
            S = shunting_inhibition.average();
        }
        else
        {
            E = excitation.sum();
            I = inhibition.sum();
            S = shunting_inhibition.sum();
        }

        if(!std::isfinite(E) || !std::isfinite(I) || !std::isfinite(S))
        {
            if(!warnedNonFiniteInput)
            {
                Warning("Nucleus received a non-finite input; retaining the last finite state and output.");
                warnedNonFiniteInput = true;
            }
            x(0) = lastFiniteState;
            output(0) = lastFiniteOutput;
            return;
        }

        if(S < 0)
        {
            if(!warnedNegativeShuntingInput)
            {
                Warning("Nucleus clamps negative SHUNTING_INHIBITION to zero.");
                warnedNegativeShuntingInput = true;
            }
            S = 0;
        }

        if(homeostasis.as_bool())
            homeostaticGain = std::clamp(
                homeostaticGain,
                minimumHomeostaticGain.as_double(),
                maximumHomeostaticGain.as_double());

        const bool wasAtOrBelowThreshold = x(0) <= theta.as_float();
        IntegrateState(E, I, S);
        float & x_value = x(0);

        if(!std::isfinite(x_value))
        {
            if(!warnedNonFiniteResult)
            {
                Warning("Nucleus update produced a non-finite state; retaining the last finite state and output.");
                warnedNonFiniteResult = true;
            }
            x_value = lastFiniteState;
            output(0) = lastFiniteOutput;
            return;
        }

        float o = 0;
        const float activationInput = std::max(0.0f, x_value - theta.as_float());

        switch(activation_function.as_int())
        {
            case 0: // unit-preserving soft saturation
                    o = (4.0f / std::numbers::pi_v<float>) * std::atan(activationInput);
                    break;
            case 1: // threshold
                    if(burstPhase == BurstPhase::integrating &&
                       wasAtOrBelowThreshold && x_value > theta)
                    {
                        o = burstLevel.as_float();
                        x_value = resetLevel.as_float();
                        burstPhase = BurstPhase::active;
                        double duration = useLegacyBurstTime
                                              ? legacyBurstTime.as_float()
                                              : burstDuration.as_float();
                        burstEndTime = currentTime + std::max(duration, GetTickDuration());
                    }

                    break;
            case 2: // ReLU
                    o = activationInput;
                    break;
            case 3: // tanh
                    o = tanh(activationInput);
                    break;
            case 4: // sigmoid
                    o = 1 / (1 + exp(-activationInput));
                    break;
            case 5: // linear
            default:
                    o = activationInput;
                    break;
        }

        float transformedOutput = TransformOutput(o);
        if(!std::isfinite(transformedOutput))
        {
            if(!warnedNonFiniteResult)
            {
                Warning("Nucleus update produced a non-finite output; retaining the last finite state and output.");
                warnedNonFiniteResult = true;
            }
            x_value = lastFiniteState;
            output(0) = lastFiniteOutput;
            return;
        }

        output(0) = transformedOutput;
        UpdateHomeostasis(o);
        lastFiniteState = x_value;
        lastFiniteOutput = output(0);
    }
};

INSTALL_CLASS(Nucleus)
