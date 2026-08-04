#include <algorithm>

#include "ikaros.h"

using namespace ikaros;

class TemplateTrackingController : public Module
{
    matrix detectionTransform_, detectionResult_, trackingTransform_, trackingResult_;
    matrix matchEnable_, transform_, result_, status_;
    parameter detectionInterval_, trackingFbThreshold_;
    int ticksSinceDetection_ = 0;

public:
    void Init() override
    {
        Bind(detectionTransform_, "DETECTION_TRANSFORM");
        Bind(detectionResult_, "DETECTION_RESULT");
        Bind(trackingTransform_, "TRACKING_TRANSFORM");
        Bind(trackingResult_, "TRACKING_RESULT");
        Bind(matchEnable_, "MATCH_ENABLE"); Bind(transform_, "TRANSFORM");
        Bind(result_, "RESULT"); Bind(status_, "STATUS");
        Bind(detectionInterval_, "detection_interval");
        Bind(trackingFbThreshold_, "tracking_fb_threshold");
    }

    void Tick() override
    {
        result_.clear(); status_.clear();
        const bool tracking = trackingResult_.rows() >= 1 && trackingResult_.cols() >= 4 &&
                              trackingResult_(0, 1) > 0.0f && trackingResult_(0, 3) > 0.0f;
        const bool detection = detectionResult_.rows() >= 1 && detectionResult_.cols() >= 4 &&
                               detectionResult_(0, 1) > 0.0f && detectionResult_(0, 3) > 0.0f;
        if(detection)
            ticksSinceDetection_ = 0;
        else
            ++ticksSinceDetection_;
        matchEnable_(0) = (!tracking || ticksSinceDetection_ >= static_cast<int>(detectionInterval_))
                              ? 1.0f : 0.0f;

        if(tracking)
        {
            transform_.copy(trackingTransform_);
            result_.resize(1, 4);
            for(int column = 0; column < 4; ++column)
                result_(0, column) = trackingResult_(0, column);
            const float confidence = std::clamp(
                1.0f - trackingResult_(0, 2) /
                           static_cast<float>(trackingFbThreshold_), 0.0f, 1.0f);
            status_.resize(1, 6);
            status_(0, 0) = trackingResult_(0, 0);
            status_(0, 1) = 1.0f;
            status_(0, 2) = 1.0f;
            status_(0, 3) = trackingResult_(0, 1);
            status_(0, 4) = trackingResult_(0, 1);
            status_(0, 5) = confidence;
            return;
        }
        if(detection)
        {
            transform_.copy(detectionTransform_);
            result_.resize(1, 4);
            result_(0, 0) = detectionResult_(0, 0);
            result_(0, 1) = detectionResult_(0, 1);
            result_(0, 2) = detectionResult_(0, 2);
            result_(0, 3) = 0.0f;
            status_.resize(1, 6);
            status_(0, 0) = detectionResult_(0, 0);
            status_(0, 1) = 1.0f;
            status_(0, 2) = 0.0f;
            status_(0, 3) = detectionResult_(0, 3);
            status_(0, 4) = detectionResult_(0, 1);
            status_(0, 5) = detectionResult_(0, 3) > 0.0f
                                 ? detectionResult_(0, 1) / detectionResult_(0, 3) : 0.0f;
            return;
        }
        transform_.reset();
        transform_(0, 0) = transform_(1, 1) = transform_(2, 2) = 1.0f;
    }
};

INSTALL_CLASS(TemplateTrackingController)
