#include "ikaros.h"

#include <algorithm>
#include <cmath>

using namespace ikaros;

namespace
{
    constexpr int stimulusAngleColumn = 0;
    constexpr int rewardColumn = 1;
    constexpr int punishmentColumn = 2;
    constexpr int intensityColumn = 3;
    constexpr int firstComponentColumn = 4;


    float angular_difference(float angle, float reference)
    {
        float difference = std::fmod(angle - reference + 180.0f, 360.0f);
        if(difference < 0.0f)
            difference += 360.0f;
        return difference - 180.0f;
    }
}


class RingWorld: public Module
{
    parameter fieldOfView_;
    parameter fixationMargin_;
    parameter retinaWidth_;

    matrix stimuli_;
    matrix auditoryStimuli_;
    matrix distalStimuli_;
    matrix gaze_;
    matrix tracking_;
    matrix fovea_;
    matrix reward_;
    matrix punishment_;
    matrix retina_;
    matrix auditoryMap_;
    matrix distalRetina_;

    void
    mapStimuliToRetina(const matrix & stimuli, matrix & retina, float gaze,
                       int angleColumn, int componentColumn, int scaleColumn = -1)
    {
        const float halfField = 0.5f * fieldOfView_.as_float();
        const int retinaWidth = retinaWidth_.as_int();

        for(int row = 0; row < stimuli.rows(); ++row)
        {
            const float offset = angular_difference(stimuli(row, angleColumn), gaze);
            if(std::abs(offset) > halfField)
                continue;

            const float retinalPosition = (offset + halfField) / (2.0f * halfField);
            const int column = retinaWidth == 1 ? 0 :
                std::clamp(int(std::round(retinalPosition * float(retinaWidth - 1))),
                           0, retinaWidth - 1);

            const float scale = scaleColumn < 0 ? 1.0f : stimuli(row, scaleColumn);
            for(int component = 0; component < stimuli.cols() - componentColumn; ++component)
            {
                retina(component, column) = std::max(retina(component, column),
                                                     scale * stimuli(row, componentColumn + component));
            }
        }
    }

    void Init() override
    {
        Bind(fieldOfView_, "field_of_view");
        Bind(fixationMargin_, "fixation_margin");
        Bind(retinaWidth_, "retina_width");

        Bind(stimuli_, "STIMULI");
        Bind(auditoryStimuli_, "AUDITORY_STIMULI");
        Bind(distalStimuli_, "DISTAL_STIMULI");
        Bind(gaze_, "GAZE");
        Bind(tracking_, "TRACKING");
        Bind(fovea_, "FOVEA");
        Bind(reward_, "REWARD");
        Bind(punishment_, "PUNISHMENT");
        Bind(retina_, "RETINA");
        Bind(auditoryMap_, "AUDITORY_MAP");
        Bind(distalRetina_, "DISTAL_RETINA");

        if(stimuli_.rank() != 2 || stimuli_.cols() < 5)
            throw exception("RingWorld: STIMULI must contain angle, reward, punishment, intensity, and at least one stimulus component.", path_);
        if(gaze_.connected() && gaze_.size() != 1)
            throw exception("RingWorld: GAZE must contain exactly one value.", path_);
        if(tracking_.connected() && tracking_.size() != 1)
            throw exception("RingWorld: TRACKING must contain exactly one value.", path_);
        if(auditoryStimuli_.connected() &&
           (auditoryStimuli_.rank() != 2 || auditoryStimuli_.cols() != 2))
            throw exception("RingWorld: AUDITORY_STIMULI must have angle and intensity columns.", path_);
        if(distalStimuli_.connected() &&
           (distalStimuli_.rank() != 2 || distalStimuli_.cols() != stimuli_.cols()))
            throw exception("RingWorld: DISTAL_STIMULI must have the same columns as STIMULI.", path_);
        if(fieldOfView_.as_float() <= 0.0f || fieldOfView_.as_float() > 360.0f)
            throw exception("RingWorld: field_of_view must be greater than 0 and at most 360 degrees.", path_);
        if(fixationMargin_.as_float() < 0.0f || fixationMargin_.as_float() > 180.0f)
            throw exception("RingWorld: fixation_margin must be between 0 and 180 degrees.", path_);
        if(retinaWidth_.as_int() < 1)
            throw exception("RingWorld: retina_width must be at least 1.", path_);

        const int componentCount = stimuli_.cols() - firstComponentColumn;
        if(fovea_.rank() != 1 || fovea_.size() != componentCount)
            throw exception("RingWorld: FOVEA shape was not resolved from STIMULI.", path_);
        if(reward_.size() != 1 || punishment_.size() != 1)
            throw exception("RingWorld: REWARD and PUNISHMENT must each contain one value.", path_);
        if(retina_.rank() != 2 || retina_.rows() != componentCount ||
           retina_.cols() != retinaWidth_.as_int())
            throw exception("RingWorld: RETINA shape was not resolved from STIMULI and retina_width.", path_);
        if(auditoryMap_.rank() != 2 || auditoryMap_.rows() != 1 ||
           auditoryMap_.cols() != retinaWidth_.as_int())
            throw exception("RingWorld: AUDITORY_MAP shape was not resolved from retina_width.", path_);
        if(distalRetina_.rank() != 2 || distalRetina_.rows() != componentCount ||
           distalRetina_.cols() != retinaWidth_.as_int())
            throw exception("RingWorld: DISTAL_RETINA shape was not resolved from STIMULI and retina_width.", path_);
    }


    void Tick() override
    {
        fovea_.reset();
        reward_.reset();
        punishment_.reset();
        retina_.reset();
        auditoryMap_.reset();
        distalRetina_.reset();

        const float gaze = gaze_.connected() ? gaze_(0) : 0.0f;
        const float fixationMargin = fixationMargin_.as_float();
        int foveaRow = -1;
        float smallestDistance = fixationMargin;

        for(int row = 0; row < stimuli_.rows(); ++row)
        {
            const float offset = angular_difference(stimuli_(row, stimulusAngleColumn), gaze);
            const float distance = std::abs(offset);

            if(distance <= fixationMargin &&
               (foveaRow < 0 || distance < smallestDistance))
            {
                foveaRow = row;
                smallestDistance = distance;
            }
        }

        mapStimuliToRetina(stimuli_, retina_, gaze, stimulusAngleColumn,
                           firstComponentColumn, intensityColumn);
        if(auditoryStimuli_.connected())
            mapStimuliToRetina(auditoryStimuli_, auditoryMap_, gaze, 0, 1);
        if(distalStimuli_.connected())
            mapStimuliToRetina(distalStimuli_, distalRetina_, gaze, stimulusAngleColumn,
                               firstComponentColumn, intensityColumn);

        if(foveaRow >= 0)
        {
            const float intensity = stimuli_(foveaRow, intensityColumn);
            reward_(0) = stimuli_(foveaRow, rewardColumn);
            punishment_(0) = stimuli_(foveaRow, punishmentColumn);
            for(int component = 0; component < stimuli_.cols() - firstComponentColumn; ++component)
                fovea_(component) = intensity * stimuli_(foveaRow, firstComponentColumn + component);
        }
    }
};

INSTALL_CLASS(RingWorld)
