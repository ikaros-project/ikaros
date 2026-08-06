#include "ikaros.h"

#include <algorithm>
#include <cmath>

using namespace ikaros;

namespace
{
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
    matrix retina_;
    matrix auditoryMap_;
    matrix distalRetina_;

    void
    mapStimuliToRetina(const matrix & stimuli, matrix & retina, float gaze)
    {
        const float halfField = 0.5f * fieldOfView_.as_float();
        const int retinaWidth = retinaWidth_.as_int();

        for(int row = 0; row < stimuli.rows(); ++row)
        {
            const float offset = angular_difference(stimuli(row, 0), gaze);
            if(std::abs(offset) > halfField)
                continue;

            const float retinalPosition = (offset + halfField) / (2.0f * halfField);
            const int column = retinaWidth == 1 ? 0 :
                std::clamp(int(std::round(retinalPosition * float(retinaWidth - 1))),
                           0, retinaWidth - 1);

            for(int component = 0; component < stimuli.cols() - 1; ++component)
                retina(component, column) = std::max(retina(component, column),
                                                     stimuli(row, component + 1));
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
        Bind(retina_, "RETINA");
        Bind(auditoryMap_, "AUDITORY_MAP");
        Bind(distalRetina_, "DISTAL_RETINA");

        if(stimuli_.rank() != 2 || stimuli_.cols() < 2)
            throw exception("RingWorld: STIMULI must be a matrix with an angle column and at least one component column.", path_);
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

        const int componentCount = stimuli_.cols() - 1;
        if(fovea_.rank() != 1 || fovea_.size() != componentCount)
            throw exception("RingWorld: FOVEA shape was not resolved from STIMULI.", path_);
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
        retina_.reset();
        auditoryMap_.reset();
        distalRetina_.reset();

        const float gaze = gaze_.connected() ? gaze_(0) : 0.0f;
        const float fixationMargin = fixationMargin_.as_float();
        int foveaRow = -1;
        float smallestDistance = fixationMargin;

        for(int row = 0; row < stimuli_.rows(); ++row)
        {
            const float offset = angular_difference(stimuli_(row, 0), gaze);
            const float distance = std::abs(offset);

            if(distance <= fixationMargin &&
               (foveaRow < 0 || distance < smallestDistance))
            {
                foveaRow = row;
                smallestDistance = distance;
            }
        }

        mapStimuliToRetina(stimuli_, retina_, gaze);
        if(auditoryStimuli_.connected())
            mapStimuliToRetina(auditoryStimuli_, auditoryMap_, gaze);
        if(distalStimuli_.connected())
            mapStimuliToRetina(distalStimuli_, distalRetina_, gaze);

        if(foveaRow >= 0)
            for(int component = 0; component < stimuli_.cols() - 1; ++component)
                fovea_(component) = stimuli_(foveaRow, component + 1);
    }
};

INSTALL_CLASS(RingWorld)
