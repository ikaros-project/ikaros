#include <algorithm>
#include <stdexcept>
#include <vector>

#include "ikaros.h"

using namespace ikaros;

class FeatureRegionFilter : public Module
{
    matrix inputKeypoints_;
    matrix inputDescriptors_;
    matrix restrict_;
    matrix outputKeypoints_;
    matrix outputDescriptors_;
    parameter imageWidth_;
    parameter imageHeight_;
    parameter region_;
    parameter maxFeatures_;
    parameter descriptorSize_;

public:
    void Init() override
    {
        Bind(inputKeypoints_, "KEYPOINTS");
        Bind(inputDescriptors_, "DESCRIPTORS");
        Bind(restrict_, "RESTRICT");
        Bind(outputKeypoints_, "FILTERED_KEYPOINTS");
        Bind(outputDescriptors_, "FILTERED_DESCRIPTORS");
        Bind(imageWidth_, "image_width");
        Bind(imageHeight_, "image_height");
        Bind(region_, "region");
        Bind(maxFeatures_, "max_features");
        Bind(descriptorSize_, "descriptor_size");

        if(outputKeypoints_.capacity() !=
               std::vector<int>({static_cast<int>(maxFeatures_), 2}) ||
           outputDescriptors_.capacity() !=
               std::vector<int>({static_cast<int>(maxFeatures_),
                                 static_cast<int>(descriptorSize_)}))
            throw std::runtime_error("FeatureRegionFilter output capacities were not set up correctly");
    }


    void Tick() override
    {
        outputKeypoints_.clear();
        outputDescriptors_.clear();
        if(inputKeypoints_.rows() == 0)
            return;
        if(inputKeypoints_.rank() != 2 || inputKeypoints_.cols() != 2 ||
           inputDescriptors_.rank() != 2 ||
           inputDescriptors_.cols() != static_cast<int>(descriptorSize_) ||
           inputDescriptors_.rows() != inputKeypoints_.rows() ||
           inputKeypoints_.rows() > static_cast<int>(maxFeatures_))
        {
            Warning("FeatureRegionFilter received incompatible feature matrices");
            return;
        }

        const bool restrictRegion = restrict_.size() > 0 && restrict_(0) > 0.0f;
        const float side = static_cast<float>(region_) *
                           std::min(static_cast<float>(imageWidth_),
                                    static_cast<float>(imageHeight_));
        const float left = 0.5f * (static_cast<float>(imageWidth_) - side);
        const float top = 0.5f * (static_cast<float>(imageHeight_) - side);
        const float right = left + side;
        const float bottom = top + side;

        int count = 0;
        for(int row = 0; row < inputKeypoints_.rows(); ++row)
            if(!restrictRegion ||
               (inputKeypoints_(row, 0) >= left && inputKeypoints_(row, 0) <= right &&
                inputKeypoints_(row, 1) >= top && inputKeypoints_(row, 1) <= bottom))
                ++count;

        outputKeypoints_.resize(count, 2);
        outputDescriptors_.resize(count, static_cast<int>(descriptorSize_));
        int target = 0;
        for(int source = 0; source < inputKeypoints_.rows(); ++source)
        {
            if(restrictRegion &&
               (inputKeypoints_(source, 0) < left || inputKeypoints_(source, 0) > right ||
                inputKeypoints_(source, 1) < top || inputKeypoints_(source, 1) > bottom))
                continue;
            outputKeypoints_(target, 0) = inputKeypoints_(source, 0);
            outputKeypoints_(target, 1) = inputKeypoints_(source, 1);
            std::copy_n(inputDescriptors_.data() + static_cast<int>(descriptorSize_) * source,
                        static_cast<int>(descriptorSize_),
                        outputDescriptors_.data() + static_cast<int>(descriptorSize_) * target);
            ++target;
        }
    }
};

INSTALL_CLASS(FeatureRegionFilter)
