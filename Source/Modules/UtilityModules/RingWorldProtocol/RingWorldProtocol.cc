#include "ikaros.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

using namespace ikaros;

namespace
{
    constexpr int stimulusColumns = 14;
    constexpr int angleColumn = 0;
    constexpr int rewardColumn = 1;
    constexpr int punishmentColumn = 2;
    constexpr int intensityColumn = 3;
    constexpr int redColumn = 4;
    constexpr int firstSymbolColumn = 7;

    const std::vector<std::string> symbolNames = {
        "horizontal", "vertical", "diagonal_minus_45", "diagonal_plus_45",
        "arrow_left", "arrow_right", "dot",
    };


    struct Stimulus
    {
        float values[stimulusColumns] = {};
    };


    struct Presentation
    {
        Stimulus stimulus;
        double onset = 0.0;
        double duration = 0.0;
        double probability = 1.0;
        bool present = true;
    };


    struct SamplingWindow
    {
        double onset = 0.0;
        double duration = 0.0;
    };


    struct Trial
    {
        std::string name;
        double duration = 0.0;
        double interTrialInterval = 0.0;
        std::vector<Presentation> presentations;
        std::vector<Stimulus> context;
        std::vector<SamplingWindow> sampling;
        int untilId = -1;
        int untilRepetition = -1;
        int untilMinimum = 0;
    };


    const dictionary &
    require_dictionary(const value & item, const std::string & path)
    {
        if(!item.is_dictionary())
            throw std::runtime_error(path + " must be an object.");
        return item.as_dictionary();
    }


    const list &
    require_list(const value & item, const std::string & path)
    {
        if(!item.is_list())
            throw std::runtime_error(path + " must be a list.");
        return item.as_list();
    }


    double
    require_number(const value & item, const std::string & path)
    {
        if(!item.is_number())
            throw std::runtime_error(path + " must be a number.");
        const double result = item.as_double();
        if(!std::isfinite(result))
            throw std::runtime_error(path + " must be finite.");
        return result;
    }


    std::string
    require_string(const value & item, const std::string & path)
    {
        if(!item.is_string())
            throw std::runtime_error(path + " must be a string.");
        return item.as_string();
    }
}


class RingWorldProtocol: public Module
{
    parameter filename_;
    parameter maxStimuli_;
    parameter maxSamplingWindows_;

    matrix criterionMet_;
    matrix stimuli_;
    matrix activeStimuli_;
    matrix sampleWindows_;
    matrix protocolTimeOutput_;
    matrix trialTimeOutput_;
    matrix trialIndexOutput_;
    matrix trialActiveOutput_;
    matrix completedOutput_;
    matrix untilActiveOutput_;
    matrix untilRepetitionOutput_;

    dictionary root_;
    dictionary defaults_;
    std::unordered_map<std::string, dictionary> stimulusTemplates_;
    std::unordered_map<std::string, dictionary> trialTemplates_;
    std::vector<Stimulus> globalContext_;
    std::vector<Trial> trials_;
    std::mt19937 generator_;

    int trialIndex_ = 0;
    double protocolTime_ = 0.0;
    double trialStartTime_ = 0.0;
    bool completed_ = false;
    int nextUntilId_ = 0;


    double
    randomValue(const value & item, const std::string & path)
    {
        if(item.is_number())
            return require_number(item, path);

        const dictionary & specification = require_dictionary(item, path);
        if(specification.contains_non_null("min") && specification.contains_non_null("max"))
        {
            const double minimum = require_number(specification["min"], path + ".min");
            const double maximum = require_number(specification["max"], path + ".max");
            if(minimum > maximum)
                throw std::runtime_error(path + ".min must not exceed max.");
            return std::uniform_real_distribution<double>(minimum, maximum)(generator_);
        }
        if(specification.contains_non_null("uniform"))
            return randomValue(specification["uniform"], path + ".uniform");
        if(specification.contains_non_null("choice"))
        {
            const list & choices = require_list(specification["choice"], path + ".choice");
            if(choices.empty())
                throw std::runtime_error(path + ".choice must not be empty.");
            const size_t index = std::uniform_int_distribution<size_t>(0, choices.size() - 1)(generator_);
            return require_number(choices[index], path + ".choice");
        }
        if(specification.contains_non_null("normal"))
        {
            const dictionary & normal = require_dictionary(specification["normal"], path + ".normal");
            const double mean = require_number(normal["mean"], path + ".normal.mean");
            const double deviation = require_number(normal["standard_deviation"], path + ".normal.standard_deviation");
            if(deviation < 0.0)
                throw std::runtime_error(path + ".normal.standard_deviation must be non-negative.");
            double result = std::normal_distribution<double>(mean, deviation)(generator_);
            if(normal.contains_non_null("min"))
                result = std::max(result, require_number(normal["min"], path + ".normal.min"));
            if(normal.contains_non_null("max"))
                result = std::min(result, require_number(normal["max"], path + ".normal.max"));
            return result;
        }
        throw std::runtime_error(path + " is not a supported random value.");
    }


    double
    property(const dictionary & object, const std::string & name, double fallback,
             const std::string & path)
    {
        if(!object.contains_non_null(name))
            return fallback;
        return randomValue(object[name], path + "." + name);
    }


    Stimulus
    resolveStimulus(const dictionary & overrides, const std::string & path)
    {
        dictionary merged = defaults_.copy();
        if(overrides.contains_non_null("stimulus"))
        {
            const std::string name = require_string(overrides["stimulus"], path + ".stimulus");
            auto iterator = stimulusTemplates_.find(name);
            if(iterator == stimulusTemplates_.end())
                throw std::runtime_error(path + ".stimulus references unknown stimulus " + name + ".");
            merged.merge(iterator->second, true);
        }
        merged.merge(overrides, true);

        Stimulus stimulus;
        stimulus.values[angleColumn] = float(property(merged, "angle", 0.0, path));
        stimulus.values[rewardColumn] = float(property(merged, "reward", 0.0, path));
        stimulus.values[punishmentColumn] = float(property(merged, "punishment", 0.0, path));
        stimulus.values[intensityColumn] = float(property(merged, "intensity", 1.0, path));

        if(stimulus.values[intensityColumn] < 0.0f || stimulus.values[intensityColumn] > 1.0f)
            throw std::runtime_error(path + ".intensity must be between 0 and 1.");

        if(merged.contains_non_null("rgb"))
        {
            const list & rgb = require_list(merged["rgb"], path + ".rgb");
            if(rgb.size() != 3)
                throw std::runtime_error(path + ".rgb must contain three values.");
            for(int channel = 0; channel < 3; ++channel)
            {
                stimulus.values[redColumn + channel] = float(randomValue(rgb[channel], path + ".rgb"));
                if(stimulus.values[redColumn + channel] < 0.0f || stimulus.values[redColumn + channel] > 1.0f)
                    throw std::runtime_error(path + ".rgb values must be between 0 and 1.");
            }
        }
        else
            stimulus.values[redColumn] = stimulus.values[redColumn + 1] = stimulus.values[redColumn + 2] = 1.0f;

        if(merged.contains_non_null("components"))
        {
            const list & components = require_list(merged["components"], path + ".components");
            for(const value & component: components)
            {
                const std::string name = require_string(component, path + ".components");
                auto iterator = std::find(symbolNames.begin(), symbolNames.end(), name);
                if(iterator == symbolNames.end())
                    throw std::runtime_error(path + ".components contains unknown component " + name + ".");
                stimulus.values[firstSymbolColumn + int(iterator - symbolNames.begin())] = 1.0f;
            }
        }
        return stimulus;
    }


    std::vector<Stimulus>
    resolveContext(const dictionary & owner, const std::string & path)
    {
        std::vector<Stimulus> result;
        if(!owner.contains_non_null("context"))
            return result;
        const dictionary & context = require_dictionary(owner["context"], path + ".context");
        for(const auto & [name, item]: context)
        {
            const dictionary & entry = require_dictionary(item, path + ".context." + name);
            if(entry.contains_non_null("enabled") && !entry["enabled"].as_bool())
                continue;
            const double probability = property(entry, "probability", 1.0, path + ".context." + name);
            if(probability < 0.0 || probability > 1.0)
                throw std::runtime_error(path + ".context." + name + ".probability must be between 0 and 1.");
            if(std::uniform_real_distribution<double>(0.0, 1.0)(generator_) <= probability)
                result.push_back(resolveStimulus(entry, path + ".context." + name));
        }
        return result;
    }


    Trial
    resolveTrial(const dictionary & invocation, const std::vector<Stimulus> & inheritedContext,
                 const std::string & path)
    {
        dictionary trial;
        if(invocation.contains_non_null("template"))
        {
            const std::string name = require_string(invocation["template"], path + ".template");
            auto iterator = trialTemplates_.find(name);
            if(iterator == trialTemplates_.end())
                throw std::runtime_error(path + ".template references unknown trial " + name + ".");
            trial = iterator->second.copy();
            if(!trial.contains_non_null("name"))
                trial["name"] = name;
        }
        trial.merge(invocation, true);

        Trial result;
        result.name = trial.contains_non_null("name") ? require_string(trial["name"], path + ".name") : "trial_" + std::to_string(trials_.size());
        result.duration = property(trial, "duration", -1.0, path);
        result.interTrialInterval = property(trial, "inter_trial_interval", 0.0, path);
        if(result.duration < 0.0 || result.interTrialInterval < 0.0)
            throw std::runtime_error(path + " duration and inter_trial_interval must be non-negative.");

        result.context = inheritedContext;
        std::vector<Stimulus> localContext = resolveContext(trial, path);
        result.context.insert(result.context.end(), localContext.begin(), localContext.end());

        if(trial.contains_non_null("stimuli"))
        {
            const list & presentations = require_list(trial["stimuli"], path + ".stimuli");
            double precedingOnset = 0.0;
            for(size_t index = 0; index < presentations.size(); ++index)
            {
                const dictionary & item = require_dictionary(presentations[index], path + ".stimuli");
                Presentation presentation;
                if(item.contains_non_null("onset"))
                    presentation.onset = randomValue(item["onset"], path + ".stimuli.onset");
                else if(item.contains_non_null("inter_stimulus_interval"))
                    presentation.onset = precedingOnset + randomValue(item["inter_stimulus_interval"], path + ".stimuli.inter_stimulus_interval");
                else if(index > 0)
                    throw std::runtime_error(path + ".stimuli requires onset or inter_stimulus_interval after the first presentation.");
                presentation.duration = property(item, "duration", -1.0, path + ".stimuli");
                presentation.probability = property(item, "probability", 1.0, path + ".stimuli");
                if(presentation.onset < 0.0 || presentation.duration < 0.0 ||
                   presentation.onset + presentation.duration > result.duration)
                    throw std::runtime_error(path + ".stimuli presentation lies outside its trial.");
                if(presentation.probability < 0.0 || presentation.probability > 1.0)
                    throw std::runtime_error(path + ".stimuli probability must be between 0 and 1.");
                presentation.present = std::uniform_real_distribution<double>(0.0, 1.0)(generator_) <= presentation.probability;
                presentation.stimulus = resolveStimulus(item, path + ".stimuli");
                result.presentations.push_back(presentation);
                precedingOnset = presentation.onset;
            }
        }

        if(trial.contains_non_null("sampling"))
        {
            const list & windows = require_list(trial["sampling"], path + ".sampling");
            if(int(windows.size()) > maxSamplingWindows_.as_int())
                throw std::runtime_error(path + ".sampling exceeds max_sampling_windows.");
            for(const value & windowValue: windows)
            {
                const dictionary & window = require_dictionary(windowValue, path + ".sampling");
                if(window.contains_non_null("relative_to"))
                    throw std::runtime_error(path + ".sampling.relative_to is not supported in the first implementation.");
                SamplingWindow resolved;
                resolved.onset = property(window, "onset", 0.0, path + ".sampling");
                resolved.duration = property(window, "duration", -1.0, path + ".sampling");
                if(resolved.onset < 0.0 || resolved.duration < 0.0 || resolved.onset + resolved.duration > result.duration)
                    throw std::runtime_error(path + ".sampling window lies outside its trial.");
                result.sampling.push_back(resolved);
            }
        }
        return result;
    }


    void
    expandProtocol(const list & protocol, const std::vector<Stimulus> & inheritedContext,
                   const std::string & path)
    {
        for(size_t index = 0; index < protocol.size(); ++index)
        {
            const dictionary & item = require_dictionary(protocol[index], path);
            if(item.contains_non_null("trial"))
                trials_.push_back(resolveTrial(require_dictionary(item["trial"], path + ".trial"), inheritedContext, path + ".trial"));
            else if(item.contains_non_null("repeat"))
            {
                const int count = int(require_number(item["repeat"], path + ".repeat"));
                if(count < 0 || double(count) != require_number(item["repeat"], path + ".repeat"))
                    throw std::runtime_error(path + ".repeat must be a non-negative integer.");
                std::vector<Stimulus> context = inheritedContext;
                std::vector<Stimulus> local = resolveContext(item, path);
                context.insert(context.end(), local.begin(), local.end());
                const list & nested = require_list(item["protocol"], path + ".protocol");
                for(int repetition = 0; repetition < count; ++repetition)
                    expandProtocol(nested, context, path + ".protocol");
            }
            else if(item.contains_non_null("until"))
            {
                const dictionary & until = require_dictionary(item["until"], path + ".until");
                const int maximum = int(require_number(until["maximum_repetitions"], path + ".until.maximum_repetitions"));
                const int minimum = until.contains_non_null("minimum_repetitions") ?
                    int(require_number(until["minimum_repetitions"], path + ".until.minimum_repetitions")) : 1;
                if(maximum <= 0 || minimum < 0 || minimum > maximum)
                    throw std::runtime_error(path + ".until repetition bounds are invalid.");
                if(!until.contains_non_null("criterion"))
                    throw std::runtime_error(path + ".until.criterion is required.");
                const list & nested = require_list(until["protocol"], path + ".until.protocol");
                const int untilId = nextUntilId_++;
                for(int repetition = 0; repetition < maximum; ++repetition)
                {
                    const size_t start = trials_.size();
                    expandProtocol(nested, inheritedContext, path + ".until.protocol");
                    if(trials_.size() != start + 1)
                        throw std::runtime_error(path + ".until must resolve to exactly one trial per repetition in the first implementation.");
                    trials_.back().untilId = untilId;
                    trials_.back().untilRepetition = repetition;
                    trials_.back().untilMinimum = minimum;
                }
            }
            else
                throw std::runtime_error(path + " contains a protocol item not supported in the first implementation.");
        }
    }


    void
    loadProtocol()
    {
        std::filesystem::path resolvedFilename;
        const std::filesystem::path configuredFilename(filename_.as_string());
        const std::filesystem::path modelRelative = configuredFilename.is_absolute() ? configuredFilename :
            std::filesystem::path(kernel().GetOptionFullPath()).parent_path() / configuredFilename;
        if(!kernel().SanitizeReadPath(modelRelative, resolvedFilename) &&
           !kernel().SanitizeReadPath(configuredFilename, resolvedFilename))
            throw std::runtime_error("filename must resolve inside the project directory or UserData.");
        if(!std::filesystem::exists(resolvedFilename))
            throw std::runtime_error("could not find protocol file at " + resolvedFilename.string() + ".");
        root_.load_json(resolvedFilename.string());
        if(!root_.contains_non_null("version") || root_["version"].as_int() != 1)
            throw std::runtime_error("version must be 1.");
        if(!root_.contains_non_null("units"))
            throw std::runtime_error("units is required.");

        const dictionary & units = require_dictionary(root_["units"], "units");
        const std::pair<const char *, const char *> requiredUnits[] = {
            {"time", "seconds"}, {"angle", "degrees"}, {"color", "normalized"},
            {"intensity", "normalized"}, {"reinforcement", "normalized"},
        };
        for(const auto & [name, expected]: requiredUnits)
            if(!units.contains_non_null(name) || units[name].as_string() != expected)
                throw std::runtime_error(std::string("units.") + name + " must be " + expected + ".");

        defaults_ = root_.contains_non_null("defaults") ? require_dictionary(root_["defaults"], "defaults").copy() : dictionary();
        if(root_.contains_non_null("stimului"))
            for(const auto & [name, item]: require_dictionary(root_["stimului"], "stimului"))
                stimulusTemplates_[name] = require_dictionary(item, "stimului." + name).copy();
        if(root_.contains_non_null("trials"))
            for(const auto & [name, item]: require_dictionary(root_["trials"], "trials"))
                trialTemplates_[name] = require_dictionary(item, "trials." + name).copy();

        const uint32_t seed = root_.contains_non_null("seed") ? uint32_t(root_["seed"].as_int()) : 0;
        generator_.seed(seed);
        globalContext_ = resolveContext(root_, "root");
        expandProtocol(require_list(root_["protocol"], "protocol"), {}, "protocol");
        if(trials_.empty())
            throw std::runtime_error("protocol must resolve to at least one trial.");
    }


    void
    writeStimulusRow(int row, const Stimulus & stimulus)
    {
        if(row >= stimuli_.rows())
            throw std::runtime_error("Active stimuli exceed max_stimuli.");
        for(int column = 0; column < stimulusColumns; ++column)
            stimuli_(row, column) = stimulus.values[column];
    }


    void Init() override
    {
        Bind(filename_, "filename");
        Bind(maxStimuli_, "max_stimuli");
        Bind(maxSamplingWindows_, "max_sampling_windows");
        Bind(criterionMet_, "CRITERION_MET");
        Bind(stimuli_, "STIMULI");
        Bind(activeStimuli_, "ACTIVE_STIMULI");
        Bind(sampleWindows_, "SAMPLE_WINDOWS");
        Bind(protocolTimeOutput_, "PROTOCOL_TIME");
        Bind(trialTimeOutput_, "TRIAL_TIME");
        Bind(trialIndexOutput_, "TRIAL_INDEX");
        Bind(trialActiveOutput_, "TRIAL_ACTIVE");
        Bind(completedOutput_, "COMPLETED");
        Bind(untilActiveOutput_, "UNTIL_ACTIVE");
        Bind(untilRepetitionOutput_, "UNTIL_REPETITION");

        if(stimuli_.rank() != 2 || stimuli_.rows() != maxStimuli_.as_int() || stimuli_.cols() != stimulusColumns)
            throw exception("RingWorldProtocol: STIMULI shape was not resolved.", path_);
        try
        {
            loadProtocol();
        }
        catch(const std::exception & error)
        {
            throw exception("RingWorldProtocol: " + std::string(error.what()), path_);
        }
    }


    void Tick() override
    {
        stimuli_.reset();
        activeStimuli_.reset();
        sampleWindows_.reset();
        protocolTimeOutput_(0) = float(protocolTime_);
        trialTimeOutput_(0) = 0.0f;
        trialIndexOutput_(0) = completed_ ? -1.0f : float(trialIndex_);
        trialActiveOutput_(0) = 0.0f;
        completedOutput_(0) = completed_ ? 1.0f : 0.0f;
        untilActiveOutput_(0) = 0.0f;
        untilRepetitionOutput_(0) = 0.0f;

        if(completed_)
            return;

        Trial & trial = trials_[trialIndex_];
        if(trial.untilId >= 0)
        {
            untilActiveOutput_(0) = 1.0f;
            untilRepetitionOutput_(0) = float(trial.untilRepetition + 1);
        }
        const double localTime = protocolTime_ - trialStartTime_;
        const bool active = localTime < trial.duration;
        int activeRows = 0;

        for(const Stimulus & stimulus: globalContext_)
            writeStimulusRow(activeRows++, stimulus);

        if(active)
        {
            trialTimeOutput_(0) = float(localTime);
            trialActiveOutput_(0) = 1.0f;
            for(const Stimulus & stimulus: trial.context)
                writeStimulusRow(activeRows++, stimulus);
            for(const Presentation & presentation: trial.presentations)
                if(presentation.present && localTime >= presentation.onset &&
                   localTime < presentation.onset + presentation.duration)
                    writeStimulusRow(activeRows++, presentation.stimulus);
            for(size_t index = 0; index < trial.sampling.size(); ++index)
                if(localTime >= trial.sampling[index].onset &&
                   localTime < trial.sampling[index].onset + trial.sampling[index].duration)
                    sampleWindows_(int(index)) = 1.0f;
        }
        activeStimuli_(0) = float(activeRows);

        protocolTime_ += GetTickDuration();
        const double span = trial.duration + trial.interTrialInterval;
        if(protocolTime_ + 1e-9 >= trialStartTime_ + span)
        {
            const int completedUntilId = trial.untilId;
            const bool criterionSatisfied = criterionMet_.connected() && criterionMet_(0) > 0.5f &&
                trial.untilRepetition + 1 >= trial.untilMinimum;
            ++trialIndex_;
            trialStartTime_ += span;
            if(criterionSatisfied)
                while(trialIndex_ < int(trials_.size()) && trials_[trialIndex_].untilId == completedUntilId)
                {
                    ++trialIndex_;
                }
            if(trialIndex_ >= int(trials_.size()))
                completed_ = true;
        }
    }
};

INSTALL_CLASS(RingWorldProtocol)
