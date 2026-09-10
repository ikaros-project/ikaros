#include <algorithm>
#include <stdexcept>
#include <vector>

#include "ikaros.h"

using namespace ikaros;

class TemplateFeatureBank : public Module
{
    matrix keypoints_;
    matrix descriptors_;
    matrix scores_;
    matrix learn_;
    matrix clear_;
    matrix templateKeypoints_;
    matrix templateDescriptors_;
    matrix templateScores_;
    matrix templateRanges_;
    matrix templateCorners_;
    parameter imageWidth_;
    parameter imageHeight_;
    parameter learningRegion_;
    parameter minFeatures_;
    parameter maxTemplates_;
    parameter maxStoredFeatures_;
    bool previousLearn_ = false;
    bool previousClear_ = false;
    bool pendingLearn_ = false;

public:
    void Init() override
    {
        Bind(keypoints_, "KEYPOINTS");
        Bind(descriptors_, "DESCRIPTORS");
        Bind(scores_, "SCORES");
        Bind(learn_, "LEARN");
        Bind(clear_, "CLEAR");
        Bind(templateKeypoints_, "TEMPLATE_KEYPOINTS");
        Bind(templateDescriptors_, "TEMPLATE_DESCRIPTORS");
        Bind(templateScores_, "TEMPLATE_SCORES");
        Bind(templateRanges_, "TEMPLATE_RANGES");
        Bind(templateCorners_, "TEMPLATE_CORNERS");
        Bind(imageWidth_, "image_width");
        Bind(imageHeight_, "image_height");
        Bind(learningRegion_, "learning_region");
        Bind(minFeatures_, "min_features");
        Bind(maxTemplates_, "max_templates");
        Bind(maxStoredFeatures_, "max_stored_features");

        if(templateKeypoints_.capacity() !=
               std::vector<int>({static_cast<int>(maxStoredFeatures_), 2}) ||
           templateDescriptors_.capacity() !=
               std::vector<int>({static_cast<int>(maxStoredFeatures_), 128}) ||
           templateScores_.capacity() !=
               std::vector<int>({static_cast<int>(maxStoredFeatures_), 1}) ||
           templateRanges_.capacity() !=
               std::vector<int>({static_cast<int>(maxTemplates_), 2}) ||
           templateCorners_.capacity() !=
               std::vector<int>({static_cast<int>(maxTemplates_), 8}))
            throw std::runtime_error(
                "TemplateFeatureBank dynamic output capacities were not set up correctly");
    }


    void Tick() override
    {
        const bool clearActive = clear_.size() > 0 && clear_(0) > 0.0f;
        const bool learnActive = learn_.size() > 0 && learn_(0) > 0.0f;
        const bool clearTriggered = clearActive && !previousClear_;
        const bool learnTriggered = learnActive && !previousLearn_;
        previousClear_ = clearActive;
        previousLearn_ = learnActive;

        if(clearTriggered)
        {
            pendingLearn_ = false;
            templateKeypoints_.clear();
            templateDescriptors_.clear();
            templateScores_.clear();
            templateRanges_.clear();
            templateCorners_.clear();
            return;
        }
        if(learnTriggered)
            pendingLearn_ = true;
        if(!pendingLearn_)
            return;

        if(keypoints_.rows() == 0)
            return;
        if(keypoints_.rank() != 2 || keypoints_.cols() != 2 ||
           descriptors_.rank() != 2 || descriptors_.cols() != 128 ||
           scores_.rank() != 2 || scores_.cols() != 1 ||
           descriptors_.rows() != keypoints_.rows() ||
           scores_.rows() != keypoints_.rows())
        {
            Warning("TemplateFeatureBank received incompatible feature matrices");
            pendingLearn_ = false;
            return;
        }
        if(templateRanges_.rows() >= static_cast<int>(maxTemplates_))
        {
            Warning("TemplateFeatureBank has reached max_templates");
            pendingLearn_ = false;
            return;
        }

        const float side = static_cast<float>(learningRegion_) *
                           std::min(static_cast<float>(imageWidth_),
                                    static_cast<float>(imageHeight_));
        const float left = 0.5f * (static_cast<float>(imageWidth_) - side);
        const float top = 0.5f * (static_cast<float>(imageHeight_) - side);
        const float right = left + side;
        const float bottom = top + side;

        int selected = 0;
        for(int row = 0; row < keypoints_.rows(); ++row)
            if(keypoints_(row, 0) >= left && keypoints_(row, 0) <= right &&
               keypoints_(row, 1) >= top && keypoints_(row, 1) <= bottom)
                ++selected;
        if(selected < static_cast<int>(minFeatures_))
        {
            Warning("TemplateFeatureBank found too few features in the learning region");
            pendingLearn_ = false;
            return;
        }
        if(templateKeypoints_.rows() + selected > static_cast<int>(maxStoredFeatures_))
        {
            Warning("TemplateFeatureBank has insufficient feature capacity for this template");
            pendingLearn_ = false;
            return;
        }

        const int start = templateKeypoints_.rows();
        templateKeypoints_.resize(start + selected, 2);
        templateDescriptors_.resize(start + selected, 128);
        templateScores_.resize(start + selected, 1);
        int target = start;
        for(int source = 0; source < keypoints_.rows(); ++source)
        {
            if(keypoints_(source, 0) < left || keypoints_(source, 0) > right ||
               keypoints_(source, 1) < top || keypoints_(source, 1) > bottom)
                continue;
            templateKeypoints_(target, 0) = keypoints_(source, 0);
            templateKeypoints_(target, 1) = keypoints_(source, 1);
            std::copy_n(descriptors_.data() + 128 * source, 128,
                        templateDescriptors_.data() + 128 * target);
            templateScores_(target, 0) = scores_(source, 0);
            ++target;
        }

        const int templateIndex = templateRanges_.rows();
        templateRanges_.resize(templateIndex + 1, 2);
        templateRanges_(templateIndex, 0) = static_cast<float>(start);
        templateRanges_(templateIndex, 1) = static_cast<float>(selected);
        templateCorners_.resize(templateIndex + 1, 8);
        templateCorners_(templateIndex, 0) = left;
        templateCorners_(templateIndex, 1) = top;
        templateCorners_(templateIndex, 2) = right;
        templateCorners_(templateIndex, 3) = top;
        templateCorners_(templateIndex, 4) = right;
        templateCorners_(templateIndex, 5) = bottom;
        templateCorners_(templateIndex, 6) = left;
        templateCorners_(templateIndex, 7) = bottom;
        pendingLearn_ = false;
    }
};

INSTALL_CLASS(TemplateFeatureBank)
