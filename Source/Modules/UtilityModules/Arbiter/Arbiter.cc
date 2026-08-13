#include "ikaros.h"

#include <cmath>
#include <limits>

using namespace ikaros;

class Arbiter : public Module
{
    parameter metric_;
    parameter arbitration_;
    parameter softmaxExponent_;
    parameter hysteresisThreshold_;
    parameter switchTime_;
    parameter alpha_;
    parameter debug_;

    matrix input_;
    matrix valueIn_;
    matrix amplitudes_;
    matrix arbitrationState_;
    matrix smoothed_;
    matrix normalized_;
    matrix output_;

    int winner_ = 0;
    int candidateCount_ = 0;

public:
    void Init() override
    {
        Bind(metric_, "metric");
        Bind(arbitration_, "arbitration");
        Bind(softmaxExponent_, "softmax_exponent");
        Bind(hysteresisThreshold_, "hysteresis_threshold");
        Bind(switchTime_, "switch_time");
        Bind(alpha_, "alpha");
        Bind(debug_, "debug");

        Bind(input_, "INPUT");
        Bind(valueIn_, "VALUE");
        Bind(amplitudes_, "AMPLITUDES");
        Bind(arbitrationState_, "ARBITRATION");
        Bind(smoothed_, "SMOOTHED");
        Bind(normalized_, "NORMALIZED");
        Bind(output_, "OUTPUT");

        if(input_.rank() < 2)
            throw exception("Arbiter: INPUT must have candidate index in dimension 0 and at least one value dimension.", path_);

        candidateCount_ = input_.shape(0);
        if(candidateCount_ <= 0)
            throw exception("Arbiter: INPUT must contain at least one candidate.", path_);

        if(valueIn_.connected() && valueIn_.size() != candidateCount_)
            throw exception("Arbiter: VALUE must contain one scalar per INPUT candidate.", path_);
    }

    void Tick() override
    {
        CalculateAmplitudes();
        Arbitrate();
        Smooth();
        Normalize();
        MixOutput();
    }

private:
    float CandidateNorm(matrix candidate) const
    {
        const float * data = candidate.data();
        float norm = 0.0f;

        if(metric_.as_int() == 0)
        {
            for(int i = 0; i < candidate.size(); ++i)
                norm += std::fabs(data[i]);
            return norm;
        }

        for(int i = 0; i < candidate.size(); ++i)
            norm += data[i] * data[i];
        return std::sqrt(norm);
    }

    void CalculateAmplitudes()
    {
        if(valueIn_.connected())
        {
            for(int i = 0; i < candidateCount_; ++i)
                amplitudes_(i) = valueIn_.data()[i];
            return;
        }

        int index = 0;
        for(matrix candidate : input_)
            amplitudes_(index++) = CandidateNorm(candidate);
    }

    int MaxAmplitudeIndex() const
    {
        int winner = 0;
        float value = amplitudes_(0);

        for(int i = 1; i < candidateCount_; ++i)
        {
            if(amplitudes_(i) > value)
            {
                winner = i;
                value = amplitudes_(i);
            }
        }

        return winner;
    }

    void SelectCandidate(int index)
    {
        arbitrationState_.set(0.0f);
        arbitrationState_(index) = amplitudes_(index);
    }

    void Arbitrate()
    {
        switch(arbitration_.as_int())
        {
            case 0:
                SelectCandidate(MaxAmplitudeIndex());
                winner_ = 0;
                break;

            case 1:
            {
                const int candidate = MaxAmplitudeIndex();
                if(winner_ < 0 || winner_ >= candidateCount_)
                    winner_ = 0;
                if(amplitudes_(candidate) > amplitudes_(winner_) + hysteresisThreshold_.as_float() || amplitudes_(winner_) == 0.0f)
                    winner_ = candidate;
                SelectCandidate(winner_);
                break;
            }

            case 2:
                for(int i = 0; i < candidateCount_; ++i)
                    arbitrationState_(i) = std::pow(amplitudes_(i), softmaxExponent_.as_float());
                winner_ = 0;
                break;

            case 3:
                for(int i = candidateCount_ - 1; i >= 0; --i)
                    if(amplitudes_(i) > 0.0f || i == 0)
                    {
                        SelectCandidate(i);
                        break;
                    }
                winner_ = 0;
                break;

            default:
                arbitrationState_.copy(amplitudes_);
                winner_ = 0;
                break;
        }
    }

    void Smooth()
    {
        float a = alpha_.as_float();
        if(switchTime_.as_int() != 0)
            a = 1.0f / switchTime_.as_float();

        if(switchTime_.as_int() > 0 || alpha_.as_float() != 1.0f)
        {
            smoothed_.scale(1.0f - a);
            smoothed_.multiply_and_accumulate(arbitrationState_, a);
        }
        else
            smoothed_.copy(arbitrationState_);
    }

    void Normalize()
    {
        normalized_.copy(smoothed_);

        const float sum = normalized_.sum();
        if(sum <= std::numeric_limits<float>::epsilon())
        {
            normalized_.set(0.0f);
            return;
        }

        normalized_.scale(1.0f / sum);
    }

    void MixOutput()
    {
        output_.set(0.0f);

        int index = 0;
        for(matrix candidate : input_)
            output_.multiply_and_accumulate(candidate, normalized_(index++));

        if(debug_.as_bool())
        {
            input_.print("INPUT");
            if(valueIn_.connected())
                valueIn_.print("VALUE");
            amplitudes_.print("AMPLITUDES");
            arbitrationState_.print("ARBITRATION");
            smoothed_.print("SMOOTHED");
            normalized_.print("NORMALIZED");
            output_.print("OUTPUT");
        }
    }
};

INSTALL_CLASS(Arbiter)
