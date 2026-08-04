#include <algorithm>
#include <stdexcept>
#include <vector>

#include "ikaros.h"

using namespace ikaros;

class FeaturePointBoxes : public Module
{
    matrix points_;
    matrix boxes_;
    parameter imageWidth_;
    parameter imageHeight_;
    parameter boxSize_;
    parameter maxPoints_;

public:
    void Init() override
    {
        Bind(points_, "POINTS");
        Bind(boxes_, "BOXES");
        Bind(imageWidth_, "image_width");
        Bind(imageHeight_, "image_height");
        Bind(boxSize_, "box_size");
        Bind(maxPoints_, "max_points");

        if(boxes_.capacity() != std::vector<int>({static_cast<int>(maxPoints_), 4}))
            throw std::runtime_error("FeaturePointBoxes output capacity was not set up correctly");
    }


    void Tick() override
    {
        boxes_.clear();
        if(points_.rows() == 0)
            return;
        if(points_.rank() != 2 || points_.cols() != 2)
        {
            Warning("FeaturePointBoxes requires point rows with x,y columns");
            return;
        }

        const int count = std::min(points_.rows(), static_cast<int>(maxPoints_));
        const float normalizedWidth = static_cast<float>(boxSize_) /
                                      static_cast<float>(imageWidth_);
        const float normalizedHeight = static_cast<float>(boxSize_) /
                                       static_cast<float>(imageHeight_);
        boxes_.resize(count, 4);
        for(int row = 0; row < count; ++row)
        {
            boxes_(row, 0) = 2.0f * points_(row, 0) /
                             static_cast<float>(imageWidth_ - 1) - 1.0f -
                             0.5f * normalizedWidth;
            boxes_(row, 1) = 2.0f * points_(row, 1) /
                             static_cast<float>(imageHeight_ - 1) - 1.0f -
                             0.5f * normalizedHeight;
            boxes_(row, 2) = normalizedWidth;
            boxes_(row, 3) = normalizedHeight;
        }
    }
};

INSTALL_CLASS(FeaturePointBoxes)
