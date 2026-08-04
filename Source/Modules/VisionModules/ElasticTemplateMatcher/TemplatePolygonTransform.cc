#include <algorithm>
#include <cmath>

#include "ProjectiveGeometry.h"

using namespace ikaros;

class TemplatePolygonTransform : public Module
{
    matrix homography_;
    matrix result_;
    matrix templateCorners_;
    matrix keypoints_;
    matrix inliers_;
    matrix trackedPoints_;
    matrix path_;
    matrix location_;
    matrix matchedFeatures_;
    parameter imageWidth_;
    parameter imageHeight_;
    parameter minAreaFraction_;
    parameter maxAreaFraction_;
    parameter maxFeatures_;

public:
    void Init() override
    {
        Bind(homography_, "HOMOGRAPHY");
        Bind(result_, "RESULT");
        Bind(templateCorners_, "TEMPLATE_CORNERS");
        Bind(keypoints_, "KEYPOINTS");
        Bind(inliers_, "INLIERS");
        Bind(trackedPoints_, "TRACKED_POINTS");
        Bind(path_, "PATH");
        Bind(location_, "LOCATION");
        Bind(matchedFeatures_, "MATCHED_FEATURES");
        Bind(imageWidth_, "image_width");
        Bind(imageHeight_, "image_height");
        Bind(minAreaFraction_, "min_area_fraction");
        Bind(maxAreaFraction_, "max_area_fraction");
        Bind(maxFeatures_, "max_features");
    }


    void Tick() override
    {
        path_.clear();
        location_.clear();
        matchedFeatures_.clear();
        if(result_.rows() != 1 || result_.cols() < 1 || templateCorners_.cols() != 8)
            return;
        const int templateIndex = static_cast<int>(result_(0, 0));
        if(templateIndex < 0 || templateIndex >= templateCorners_.rows())
            return;
        float x[4];
        float y[4];
        for(int i = 0; i < 4; ++i)
            if(!ProjectiveGeometry::Project(homography_,
                    templateCorners_(templateIndex, 2 * i),
                    templateCorners_(templateIndex, 2 * i + 1), x[i], y[i]))
                return;
        float twiceArea = 0.0f;
        float crossSign = 0.0f;
        for(int i = 0; i < 4; ++i)
        {
            const int next = (i + 1) % 4;
            const int next2 = (i + 2) % 4;
            twiceArea += x[i] * y[next] - y[i] * x[next];
            const float cross = (x[next] - x[i]) * (y[next2] - y[next]) -
                                (y[next] - y[i]) * (x[next2] - x[next]);
            if(std::abs(cross) < 1.0e-5f)
                return;
            if(i == 0)
                crossSign = cross;
            else if(cross * crossSign <= 0.0f)
                return;
            if(x[i] < -0.5f * static_cast<float>(imageWidth_) ||
               x[i] > 1.5f * static_cast<float>(imageWidth_) ||
               y[i] < -0.5f * static_cast<float>(imageHeight_) ||
               y[i] > 1.5f * static_cast<float>(imageHeight_))
                return;
        }
        const float areaFraction = 0.5f * std::abs(twiceArea) /
            (static_cast<float>(imageWidth_) * static_cast<float>(imageHeight_));
        if(areaFraction < static_cast<float>(minAreaFraction_) ||
           areaFraction > static_cast<float>(maxAreaFraction_))
            return;
        path_.resize(5, 2);
        float centerX = 0.0f;
        float centerY = 0.0f;
        for(int i = 0; i < 5; ++i)
        {
            const int corner = i % 4;
            path_(i, 0) = 2.0f * x[corner] /
                          std::max(1.0f, static_cast<float>(imageWidth_ - 1)) - 1.0f;
            path_(i, 1) = 2.0f * y[corner] /
                          std::max(1.0f, static_cast<float>(imageHeight_ - 1)) - 1.0f;
            if(i < 4)
            {
                centerX += path_(i, 0);
                centerY += path_(i, 1);
            }
        }
        location_.resize(1, 2);
        location_(0, 0) = 0.25f * centerX;
        location_(0, 1) = 0.25f * centerY;
        const bool useTracked = trackedPoints_.rank() == 2 && trackedPoints_.cols() == 2 &&
                                trackedPoints_.rows() > 0;
        const int featureLimit = std::min(useTracked ? trackedPoints_.rows() : inliers_.rows(),
                                          static_cast<int>(maxFeatures_));
        matchedFeatures_.resize(featureLimit, 4);
        const float boxWidth = 12.0f / static_cast<float>(imageWidth_);
        const float boxHeight = 12.0f / static_cast<float>(imageHeight_);
        for(int i = 0; i < featureLimit; ++i)
        {
            const int current = useTracked ? -1 : static_cast<int>(inliers_(i, 2));
            if(!useTracked && (current < 0 || current >= keypoints_.rows()))
                continue;
            const float pointX = useTracked ? trackedPoints_(i, 0) : keypoints_(current, 0);
            const float pointY = useTracked ? trackedPoints_(i, 1) : keypoints_(current, 1);
            matchedFeatures_(i, 0) = 2.0f * pointX /
                                     static_cast<float>(imageWidth_ - 1) - 1.0f - boxWidth / 2;
            matchedFeatures_(i, 1) = 2.0f * pointY /
                                     static_cast<float>(imageHeight_ - 1) - 1.0f - boxHeight / 2;
            matchedFeatures_(i, 2) = boxWidth;
            matchedFeatures_(i, 3) = boxHeight;
        }
    }
};

INSTALL_CLASS(TemplatePolygonTransform)
