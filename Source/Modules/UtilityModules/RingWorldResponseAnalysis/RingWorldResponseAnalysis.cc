#include "ikaros.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>
#include <string>
#include <vector>

using namespace ikaros;

class RingWorldResponseAnalysis: public Module
{
    parameter responseCount_;
    parameter maxSamplingWindows_;
    parameter latencyThreshold_;
    parameter criterionWindow_;
    parameter criterionResponse_;
    parameter criterionMeasurement_;
    parameter criterionOperator_;
    parameter criterionValueParameter_;
    parameter historyLength_;
    parameter historyAggregate_;
    parameter consecutiveRequired_;

    matrix responses_;
    matrix sampleWindows_;
    matrix trialIndex_;
    matrix untilActive_;
    matrix untilRepetition_;
    matrix latency_;
    matrix integral_;
    matrix maximum_;
    matrix criterionValue_;
    matrix criterionPass_;
    matrix criterionMet_;
    matrix evaluation_;
    matrix summary_;

    std::vector<float> previousResponse_;
    std::vector<float> previousWindow_;
    std::vector<double> windowElapsed_;
    std::deque<double> history_;
    int consecutivePasses_ = 0;
    int lastUntilRepetition_ = 0;


    double
    selectedMeasurement() const
    {
        const int window = criterionWindow_.as_int();
        const int response = criterionResponse_.as_int();
        const std::string measurement = criterionMeasurement_.as_string();
        if(measurement == "latency")
            return latency_(window, response);
        if(measurement == "integral")
            return integral_(window, response);
        return maximum_(window, response);
    }


    double
    aggregateHistory() const
    {
        const std::string aggregate = historyAggregate_.as_string();
        if(aggregate == "minimum")
            return *std::min_element(history_.begin(), history_.end());
        if(aggregate == "maximum")
            return *std::max_element(history_.begin(), history_.end());
        double sum = 0.0;
        for(double value: history_)
            sum += value;
        return sum / double(history_.size());
    }


    bool
    compare(double value) const
    {
        const double threshold = criterionValueParameter_.as_float();
        const std::string operation = criterionOperator_.as_string();
        if(operation == "less")
            return value < threshold;
        if(operation == "less_equal")
            return value <= threshold;
        if(operation == "greater")
            return value > threshold;
        return value >= threshold;
    }


    void
    evaluateCriterion()
    {
        evaluation_(0) = 1.0f;
        const double measurement = selectedMeasurement();
        if(!std::isfinite(measurement) || measurement < 0.0)
        {
            consecutivePasses_ = 0;
            criterionPass_(0) = 0.0f;
            criterionMet_(0) = 0.0f;
            Warning("RingWorldResponseAnalysis: criterion measurement is unavailable or non-finite.");
            return;
        }

        history_.push_back(measurement);
        while(int(history_.size()) > historyLength_.as_int())
            history_.pop_front();
        if(int(history_.size()) < historyLength_.as_int())
        {
            criterionPass_(0) = 0.0f;
            criterionMet_(0) = 0.0f;
            return;
        }

        const double value = aggregateHistory();
        criterionValue_(0) = float(value);
        const bool pass = compare(value);
        criterionPass_(0) = pass ? 1.0f : 0.0f;
        consecutivePasses_ = pass ? consecutivePasses_ + 1 : 0;
        criterionMet_(0) = consecutivePasses_ >= consecutiveRequired_.as_int() ? 1.0f : 0.0f;
    }


    void Init() override
    {
        Bind(responseCount_, "response_count");
        Bind(maxSamplingWindows_, "max_sampling_windows");
        Bind(latencyThreshold_, "latency_threshold");
        Bind(criterionWindow_, "criterion_window");
        Bind(criterionResponse_, "criterion_response");
        Bind(criterionMeasurement_, "criterion_measurement");
        Bind(criterionOperator_, "criterion_operator");
        Bind(criterionValueParameter_, "criterion_value");
        Bind(historyLength_, "history_length");
        Bind(historyAggregate_, "history_aggregate");
        Bind(consecutiveRequired_, "consecutive");
        Bind(responses_, "RESPONSES");
        Bind(sampleWindows_, "SAMPLE_WINDOWS");
        Bind(trialIndex_, "TRIAL_INDEX");
        Bind(untilActive_, "UNTIL_ACTIVE");
        Bind(untilRepetition_, "UNTIL_REPETITION");
        Bind(latency_, "LATENCY");
        Bind(integral_, "INTEGRAL");
        Bind(maximum_, "MAXIMUM");
        Bind(criterionValue_, "CRITERION_VALUE");
        Bind(criterionPass_, "CRITERION_PASS");
        Bind(criterionMet_, "CRITERION_MET");
        Bind(evaluation_, "EVALUATION");
        Bind(summary_, "SUMMARY");

        if(criterionWindow_.as_int() >= maxSamplingWindows_.as_int() ||
           criterionResponse_.as_int() >= responseCount_.as_int())
            throw exception("RingWorldResponseAnalysis: criterion indices are outside the configured shapes.", path_);
        if(historyLength_.as_int() < 1 || consecutiveRequired_.as_int() < 1)
            throw exception("RingWorldResponseAnalysis: history_length and consecutive must be positive.", path_);

        previousResponse_.assign(responseCount_.as_int(), 0.0f);
        previousWindow_.assign(maxSamplingWindows_.as_int(), 0.0f);
        windowElapsed_.assign(maxSamplingWindows_.as_int(), 0.0);
        latency_.set(-1.0f);
        maximum_.set(-std::numeric_limits<float>::infinity());
        summary_.set_labels(0, "Latency", "Integral", "Maximum", "Criterion value", "Criterion pass");
    }


    void Tick() override
    {
        evaluation_(0) = 0.0f;
        const int repetition = int(untilRepetition_(0));
        if(untilActive_(0) > 0.5f && (lastUntilRepetition_ == 0 || repetition < lastUntilRepetition_))
        {
            history_.clear();
            consecutivePasses_ = 0;
            criterionMet_(0) = 0.0f;
        }
        if(untilActive_(0) > 0.5f)
            lastUntilRepetition_ = repetition;
        else
            lastUntilRepetition_ = 0;

        const double dt = GetTickDuration();
        for(int window = 0; window < maxSamplingWindows_.as_int(); ++window)
        {
            const bool active = sampleWindows_(window) > 0.5f;
            const bool wasActive = previousWindow_[window] > 0.5f;
            if(active && !wasActive)
            {
                windowElapsed_[window] = 0.0;
                for(int response = 0; response < responseCount_.as_int(); ++response)
                {
                    latency_(window, response) = responses_(response) >= latencyThreshold_.as_float() ? 0.0f : -1.0f;
                    integral_(window, response) = 0.0f;
                    maximum_(window, response) = responses_(response);
                }
            }
            else if(active)
            {
                windowElapsed_[window] += dt;
                for(int response = 0; response < responseCount_.as_int(); ++response)
                {
                    integral_(window, response) += float(0.5 * dt * (previousResponse_[response] + responses_(response)));
                    maximum_(window, response) = std::max(maximum_(window, response), responses_(response));
                    if(latency_(window, response) < 0.0f && previousResponse_[response] < latencyThreshold_.as_float() &&
                       responses_(response) >= latencyThreshold_.as_float())
                    {
                        const double denominator = responses_(response) - previousResponse_[response];
                        const double fraction = denominator == 0.0 ? 1.0 :
                            (latencyThreshold_.as_float() - previousResponse_[response]) / denominator;
                        latency_(window, response) = float(windowElapsed_[window] - dt + fraction * dt);
                    }
                }
            }
            if(!active && wasActive && window == criterionWindow_.as_int() && untilActive_(0) > 0.5f)
                evaluateCriterion();
            previousWindow_[window] = active ? 1.0f : 0.0f;
        }
        for(int response = 0; response < responseCount_.as_int(); ++response)
            previousResponse_[response] = responses_(response);
        const int criterionWindow = criterionWindow_.as_int();
        const int criterionResponse = criterionResponse_.as_int();
        summary_(0) = latency_(criterionWindow, criterionResponse);
        summary_(1) = integral_(criterionWindow, criterionResponse);
        summary_(2) = maximum_(criterionWindow, criterionResponse);
        summary_(3) = criterionValue_(0);
        summary_(4) = criterionPass_(0);
    }
};

INSTALL_CLASS(RingWorldResponseAnalysis)
