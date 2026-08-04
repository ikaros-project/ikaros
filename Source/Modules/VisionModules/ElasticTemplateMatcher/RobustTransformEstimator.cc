#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include "ProjectiveGeometry.h"

using namespace ikaros;

class RobustTransformEstimator : public Module
{
    matrix keypoints_;
    matrix templateKeypoints_;
    matrix templateRanges_;
    matrix templateCorners_;
    matrix correspondences_;
    matrix homography_;
    matrix inliers_;
    matrix result_;
    parameter reprojectionThreshold_;
    parameter minInliers_;
    parameter minInlierRatio_, minMeanMatchScore_, minSourceCoverage_, maxMeanError_;
    parameter imageWidth_, imageHeight_, minAreaFraction_, maxAreaFraction_;
    parameter ransacIterations_;
    parameter maxMatches_;
    parameter maxTemplates_;
    matrix source_;
    matrix target_;
    matrix sampleSource_;
    matrix sampleTarget_;
    matrix candidate_;
    matrix bestTransform_;
    matrix refinedSource_;
    matrix refinedTarget_;
    ProjectiveGeometry geometry_;

public:
    RobustTransformEstimator() : geometry_(512) {}

    void Init() override
    {
        Bind(keypoints_, "KEYPOINTS");
        Bind(templateKeypoints_, "TEMPLATE_KEYPOINTS");
        Bind(templateRanges_, "TEMPLATE_RANGES");
        Bind(templateCorners_, "TEMPLATE_CORNERS");
        Bind(correspondences_, "CORRESPONDENCES");
        Bind(homography_, "HOMOGRAPHY");
        Bind(inliers_, "INLIERS");
        Bind(result_, "RESULT");
        Bind(reprojectionThreshold_, "reprojection_threshold");
        Bind(minInliers_, "min_inliers");
        Bind(minInlierRatio_, "min_inlier_ratio");
        Bind(minMeanMatchScore_, "min_mean_match_score");
        Bind(minSourceCoverage_, "min_source_coverage");
        Bind(maxMeanError_, "max_mean_error");
        Bind(imageWidth_, "image_width");
        Bind(imageHeight_, "image_height");
        Bind(minAreaFraction_, "min_area_fraction");
        Bind(maxAreaFraction_, "max_area_fraction");
        Bind(ransacIterations_, "ransac_iterations");
        Bind(maxMatches_, "max_matches");
        Bind(maxTemplates_, "max_templates");
        if(static_cast<int>(maxMatches_) > 512)
            throw std::runtime_error("RobustTransformEstimator max_matches cannot exceed 512");
        source_.realloc(512, 2);
        target_.realloc(512, 2);
        sampleSource_.realloc(4, 2);
        sampleTarget_.realloc(4, 2);
        candidate_.realloc(3, 3);
        bestTransform_.realloc(3, 3);
        refinedSource_.realloc(512, 2);
        refinedTarget_.realloc(512, 2);
    }


    void Tick() override
    {
        homography_.reset();
        homography_(0, 0) = homography_(1, 1) = homography_(2, 2) = 1.0f;
        inliers_.clear();
        result_.clear();
        int globalBestCount = 0;
        float globalBestError = std::numeric_limits<float>::infinity();
        int globalBestTemplate = -1;
        int globalCorrespondences = 0;

        const int templateLimit = std::min(templateRanges_.rows(),
                                           static_cast<int>(maxTemplates_));
        for(int templateIndex = 0; templateIndex < templateLimit; ++templateIndex)
        {
            source_.resize(512, 2);
            target_.resize(512, 2);
            const int bankStart = static_cast<int>(templateRanges_(templateIndex, 0));
            const int bankLength = static_cast<int>(templateRanges_(templateIndex, 1));
            int count = 0;
            for(int row = 0; row < correspondences_.rows() &&
                             count < static_cast<int>(maxMatches_); ++row)
            {
                if(static_cast<int>(correspondences_(row, 0)) != templateIndex)
                    continue;
                const int local = static_cast<int>(correspondences_(row, 1));
                const int current = static_cast<int>(correspondences_(row, 2));
                if(local < 0 || local >= bankLength || current < 0 || current >= keypoints_.rows())
                    continue;
                source_(count, 0) = templateKeypoints_(bankStart + local, 0);
                source_(count, 1) = templateKeypoints_(bankStart + local, 1);
                target_(count, 0) = keypoints_(current, 0);
                target_(count, 1) = keypoints_(current, 1);
                ++count;
            }
            if(count < 4)
                continue;
            source_.resize(count, 2);
            target_.resize(count, 2);
            int bestCount = 0;
            float bestError = std::numeric_limits<float>::infinity();
            uint32_t state = 0x9e3779b9u ^ static_cast<uint32_t>(templateIndex);
            for(int iteration = 0; iteration < static_cast<int>(ransacIterations_); ++iteration)
            {
                int selected[4];
                bool unique = true;
                for(int i = 0; i < 4; ++i)
                {
                    state = 1664525u * state + 1013904223u;
                    selected[i] = static_cast<int>(state % static_cast<uint32_t>(count));
                    for(int j = 0; j < i; ++j)
                        if(selected[i] == selected[j])
                            unique = false;
                }
                if(!unique)
                    continue;
                for(int i = 0; i < 4; ++i)
                {
                    sampleSource_(i, 0) = source_(selected[i], 0);
                    sampleSource_(i, 1) = source_(selected[i], 1);
                    sampleTarget_(i, 0) = target_(selected[i], 0);
                    sampleTarget_(i, 1) = target_(selected[i], 1);
                }
                if(!geometry_.Homography(sampleSource_, sampleTarget_, candidate_))
                    continue;
                int inlierCount = 0;
                float errorSum = 0.0f;
                for(int i = 0; i < count; ++i)
                {
                    float x, y;
                    if(!ProjectiveGeometry::Project(candidate_, source_(i, 0), source_(i, 1), x, y))
                        continue;
                    const float error = std::hypot(x - target_(i, 0), y - target_(i, 1));
                    if(error <= static_cast<float>(reprojectionThreshold_))
                    {
                        ++inlierCount;
                        errorSum += error;
                    }
                }
                const float meanError = inlierCount ? errorSum / inlierCount
                                                     : std::numeric_limits<float>::infinity();
                if(inlierCount > bestCount || (inlierCount == bestCount && meanError < bestError))
                {
                    bestCount = inlierCount;
                    bestError = meanError;
                    bestTransform_.copy(candidate_);
                }
            }
            if(bestCount < static_cast<int>(minInliers_))
                continue;
            refinedSource_.resize(512, 2);
            refinedTarget_.resize(512, 2);
            int refinedCount = 0;
            for(int i = 0; i < count; ++i)
            {
                float x, y;
                if(!ProjectiveGeometry::Project(bestTransform_, source_(i, 0), source_(i, 1), x, y) ||
                   std::hypot(x - target_(i, 0), y - target_(i, 1)) >
                       static_cast<float>(reprojectionThreshold_))
                    continue;
                refinedSource_(refinedCount, 0) = source_(i, 0);
                refinedSource_(refinedCount, 1) = source_(i, 1);
                refinedTarget_(refinedCount, 0) = target_(i, 0);
                refinedTarget_(refinedCount, 1) = target_(i, 1);
                ++refinedCount;
            }
            refinedSource_.resize(refinedCount, 2);
            refinedTarget_.resize(refinedCount, 2);
            if(!geometry_.Homography(refinedSource_, refinedTarget_, candidate_))
                continue;
            float finalError = 0.0f;
            for(int i = 0; i < refinedCount; ++i)
            {
                float x, y;
                ProjectiveGeometry::Project(candidate_, refinedSource_(i, 0), refinedSource_(i, 1), x, y);
                finalError += std::hypot(x - refinedTarget_(i, 0), y - refinedTarget_(i, 1));
            }
            finalError /= refinedCount;
            float inlierScore = 0.0f;
            for(int row = 0; row < correspondences_.rows(); ++row)
            {
                if(static_cast<int>(correspondences_(row, 0)) != templateIndex)
                    continue;
                const int local = static_cast<int>(correspondences_(row, 1));
                const int current = static_cast<int>(correspondences_(row, 2));
                if(local < 0 || local >= bankLength || current < 0 || current >= keypoints_.rows())
                    continue;
                float x, y;
                if(ProjectiveGeometry::Project(candidate_,
                       templateKeypoints_(bankStart + local, 0),
                       templateKeypoints_(bankStart + local, 1), x, y) &&
                   std::hypot(x - keypoints_(current, 0), y - keypoints_(current, 1)) <=
                       static_cast<float>(reprojectionThreshold_))
                    inlierScore += correspondences_(row, 3);
            }

            float templateMinX = std::numeric_limits<float>::infinity();
            float templateMinY = std::numeric_limits<float>::infinity();
            float templateMaxX = -std::numeric_limits<float>::infinity();
            float templateMaxY = -std::numeric_limits<float>::infinity();
            for(int i = 0; i < bankLength; ++i)
            {
                templateMinX = std::min(templateMinX, templateKeypoints_(bankStart + i, 0));
                templateMinY = std::min(templateMinY, templateKeypoints_(bankStart + i, 1));
                templateMaxX = std::max(templateMaxX, templateKeypoints_(bankStart + i, 0));
                templateMaxY = std::max(templateMaxY, templateKeypoints_(bankStart + i, 1));
            }
            float inlierMinX = std::numeric_limits<float>::infinity();
            float inlierMinY = std::numeric_limits<float>::infinity();
            float inlierMaxX = -std::numeric_limits<float>::infinity();
            float inlierMaxY = -std::numeric_limits<float>::infinity();
            for(int i = 0; i < refinedCount; ++i)
            {
                inlierMinX = std::min(inlierMinX, refinedSource_(i, 0));
                inlierMinY = std::min(inlierMinY, refinedSource_(i, 1));
                inlierMaxX = std::max(inlierMaxX, refinedSource_(i, 0));
                inlierMaxY = std::max(inlierMaxY, refinedSource_(i, 1));
            }
            const float templateArea = (templateMaxX - templateMinX) *
                                       (templateMaxY - templateMinY);
            const float inlierArea = (inlierMaxX - inlierMinX) * (inlierMaxY - inlierMinY);
            const float sourceCoverage = templateArea > 1.0e-6f ? inlierArea / templateArea : 0.0f;

            bool validQuad = templateCorners_.rank() == 2 && templateCorners_.cols() == 8 &&
                             templateIndex < templateCorners_.rows();
            float quadX[4];
            float quadY[4];
            float twiceArea = 0.0f;
            float crossSign = 0.0f;
            for(int i = 0; validQuad && i < 4; ++i)
            {
                validQuad = ProjectiveGeometry::Project(candidate_,
                    templateCorners_(templateIndex, 2 * i),
                    templateCorners_(templateIndex, 2 * i + 1), quadX[i], quadY[i]);
                if(validQuad && (quadX[i] < -0.5f * static_cast<float>(imageWidth_) ||
                                 quadX[i] > 1.5f * static_cast<float>(imageWidth_) ||
                                 quadY[i] < -0.5f * static_cast<float>(imageHeight_) ||
                                 quadY[i] > 1.5f * static_cast<float>(imageHeight_)))
                    validQuad = false;
            }
            for(int i = 0; validQuad && i < 4; ++i)
            {
                const int next = (i + 1) % 4;
                const int next2 = (i + 2) % 4;
                twiceArea += quadX[i] * quadY[next] - quadY[i] * quadX[next];
                const float cross = (quadX[next] - quadX[i]) * (quadY[next2] - quadY[next]) -
                                    (quadY[next] - quadY[i]) * (quadX[next2] - quadX[next]);
                if(std::abs(cross) < 1.0e-5f || (i > 0 && cross * crossSign <= 0.0f))
                    validQuad = false;
                if(i == 0)
                    crossSign = cross;
            }
            const float areaFraction = 0.5f * std::abs(twiceArea) /
                (static_cast<float>(imageWidth_) * static_cast<float>(imageHeight_));
            validQuad = validQuad && areaFraction >= static_cast<float>(minAreaFraction_) &&
                        areaFraction <= static_cast<float>(maxAreaFraction_);

            if(static_cast<float>(refinedCount) / count < static_cast<float>(minInlierRatio_) ||
               inlierScore / refinedCount < static_cast<float>(minMeanMatchScore_) ||
               sourceCoverage < static_cast<float>(minSourceCoverage_) ||
               finalError > static_cast<float>(maxMeanError_) || !validQuad)
                continue;
            if(refinedCount > globalBestCount ||
               (refinedCount == globalBestCount && finalError < globalBestError))
            {
                globalBestCount = refinedCount;
                globalBestError = finalError;
                globalBestTemplate = templateIndex;
                globalCorrespondences = count;
                homography_.copy(candidate_);
            }
        }
        if(globalBestTemplate < 0)
            return;
        result_.resize(1, 4);
        result_(0, 0) = static_cast<float>(globalBestTemplate);
        result_(0, 1) = static_cast<float>(globalBestCount);
        result_(0, 2) = globalBestError;
        result_(0, 3) = static_cast<float>(globalCorrespondences);
        for(int row = 0; row < correspondences_.rows(); ++row)
            if(static_cast<int>(correspondences_(row, 0)) == globalBestTemplate &&
               inliers_.rows() < globalBestCount)
            {
                const int local = static_cast<int>(correspondences_(row, 1));
                const int current = static_cast<int>(correspondences_(row, 2));
                const int bankStart = static_cast<int>(templateRanges_(globalBestTemplate, 0));
                const int bankLength = static_cast<int>(templateRanges_(globalBestTemplate, 1));
                if(local < 0 || local >= bankLength || current < 0 || current >= keypoints_.rows())
                    continue;
                float x, y;
                if(ProjectiveGeometry::Project(homography_,
                       templateKeypoints_(bankStart + local, 0),
                       templateKeypoints_(bankStart + local, 1), x, y) &&
                   std::hypot(x - keypoints_(current, 0), y - keypoints_(current, 1)) <=
                       static_cast<float>(reprojectionThreshold_))
                {
                    const int targetRow = inliers_.rows();
                    inliers_.resize(targetRow + 1, 4);
                    for(int column = 0; column < 4; ++column)
                        inliers_(targetRow, column) = correspondences_(row, column);
                }
            }
    }
};

INSTALL_CLASS(RobustTransformEstimator)
