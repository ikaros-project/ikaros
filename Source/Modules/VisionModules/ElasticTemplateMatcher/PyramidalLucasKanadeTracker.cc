#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

#include "ProjectiveGeometry.h"

using namespace ikaros;

namespace
{
    float
    sample(const matrix & image, float x, float y)
    {
        const int x0 = static_cast<int>(std::floor(x));
        const int y0 = static_cast<int>(std::floor(y));
        if(x0 < 0 || y0 < 0 || x0 + 1 >= image.cols() || y0 + 1 >= image.rows())
            return std::numeric_limits<float>::quiet_NaN();
        const float ax = x - x0;
        const float ay = y - y0;
        return (1 - ay) * ((1 - ax) * image(y0, x0) + ax * image(y0, x0 + 1)) +
               ay * ((1 - ax) * image(y0 + 1, x0) + ax * image(y0 + 1, x0 + 1));
    }


    void
    downsample(const matrix & source, matrix & target)
    {
        for(int y = 0; y < target.rows(); ++y)
            for(int x = 0; x < target.cols(); ++x)
                target(y, x) = 0.25f * (source(2 * y, 2 * x) +
                                        source(2 * y, 2 * x + 1) +
                                        source(2 * y + 1, 2 * x) +
                                        source(2 * y + 1, 2 * x + 1));
    }


    bool
    trackPoint(const std::array<matrix, 4> & previous,
               const std::array<matrix, 4> & current,
               int levels, int radius, int iterations,
               float sourceX, float sourceY, float & targetX, float & targetY,
               float & residual)
    {
        float estimateX = sourceX;
        float estimateY = sourceY;
        residual = 0.0f;
        for(int level = levels - 1; level >= 0; --level)
        {
            const float scale = static_cast<float>(1 << level);
            const float px = sourceX / scale;
            const float py = sourceY / scale;
            float qx = estimateX / scale;
            float qy = estimateY / scale;
            for(int iteration = 0; iteration < iterations; ++iteration)
            {
                float gxx = 0, gxy = 0, gyy = 0, bx = 0, by = 0, error = 0;
                int samples = 0;
                for(int wy = -radius; wy <= radius; ++wy)
                    for(int wx = -radius; wx <= radius; ++wx)
                    {
                        const float reference = sample(previous[level], px + wx, py + wy);
                        const float observed = sample(current[level], qx + wx, qy + wy);
                        const float gx = 0.5f * (sample(previous[level], px + wx + 1, py + wy) -
                                                 sample(previous[level], px + wx - 1, py + wy));
                        const float gy = 0.5f * (sample(previous[level], px + wx, py + wy + 1) -
                                                 sample(previous[level], px + wx, py + wy - 1));
                        if(!std::isfinite(reference) || !std::isfinite(observed) ||
                           !std::isfinite(gx) || !std::isfinite(gy))
                            continue;
                        const float difference = reference - observed;
                        gxx += gx * gx; gxy += gx * gy; gyy += gy * gy;
                        bx += gx * difference; by += gy * difference;
                        error += std::abs(difference);
                        ++samples;
                    }
                const float determinant = gxx * gyy - gxy * gxy;
                if(samples < 9 || determinant < 1.0e-8f)
                    return false;
                const float dx = (gyy * bx - gxy * by) / determinant;
                const float dy = (gxx * by - gxy * bx) / determinant;
                qx += dx;
                qy += dy;
                residual = error / samples;
                if(dx * dx + dy * dy < 1.0e-4f)
                    break;
            }
            estimateX = qx * scale;
            estimateY = qy * scale;
        }
        targetX = estimateX;
        targetY = estimateY;
        return std::isfinite(targetX) && std::isfinite(targetY);
    }
}

class PyramidalLucasKanadeTracker : public Module
{
    matrix image_, keypoints_, templateKeypoints_, templateRanges_;
    matrix seedHomography_, seedResult_, seedInliers_;
    matrix transform_, referenceOutput_, currentOutput_, result_;
    parameter imageWidth_, imageHeight_, levels_, windowRadius_, iterations_;
    parameter maxResidual_, forwardBackwardThreshold_, minPoints_, maxPoints_;
    std::array<matrix, 4> previousPyramid_;
    std::array<matrix, 4> currentPyramid_;
    matrix referencePoints_, previousPoints_, nextPoints_, acceptedReference_, acceptedCurrent_;
    ProjectiveGeometry geometry_;
    bool hasPreviousImage_ = false;
    bool active_ = false;
    int templateIndex_ = -1;

    void buildPyramid(const matrix & source, std::array<matrix, 4> & pyramid)
    {
        pyramid[0].copy(source);
        for(int level = 1; level < static_cast<int>(levels_); ++level)
            downsample(pyramid[level - 1], pyramid[level]);
    }

    void seed()
    {
        if(seedResult_.rows() != 1 || seedInliers_.rows() < static_cast<int>(minPoints_))
            return;
        templateIndex_ = static_cast<int>(seedResult_(0, 0));
        if(templateIndex_ < 0 || templateIndex_ >= templateRanges_.rows())
            return;
        const int start = static_cast<int>(templateRanges_(templateIndex_, 0));
        const int length = static_cast<int>(templateRanges_(templateIndex_, 1));
        const int limit = std::min(seedInliers_.rows(), static_cast<int>(maxPoints_));
        referencePoints_.resize(limit, 2);
        previousPoints_.resize(limit, 2);
        int count = 0;
        for(int i = 0; i < limit; ++i)
        {
            const int local = static_cast<int>(seedInliers_(i, 1));
            const int current = static_cast<int>(seedInliers_(i, 2));
            if(local < 0 || local >= length || current < 0 || current >= keypoints_.rows())
                continue;
            referencePoints_(count, 0) = templateKeypoints_(start + local, 0);
            referencePoints_(count, 1) = templateKeypoints_(start + local, 1);
            previousPoints_(count, 0) = keypoints_(current, 0);
            previousPoints_(count, 1) = keypoints_(current, 1);
            ++count;
        }
        referencePoints_.resize(count, 2);
        previousPoints_.resize(count, 2);
        if(count < static_cast<int>(minPoints_))
            return;
        transform_.copy(seedHomography_);
        active_ = true;
    }

public:
    PyramidalLucasKanadeTracker() : geometry_(512) {}

    void Init() override
    {
        Bind(image_, "IMAGE"); Bind(keypoints_, "KEYPOINTS");
        Bind(templateKeypoints_, "TEMPLATE_KEYPOINTS"); Bind(templateRanges_, "TEMPLATE_RANGES");
        Bind(seedHomography_, "SEED_HOMOGRAPHY"); Bind(seedResult_, "SEED_RESULT");
        Bind(seedInliers_, "SEED_INLIERS"); Bind(transform_, "TRANSFORM");
        Bind(referenceOutput_, "REFERENCE_POINTS"); Bind(currentOutput_, "CURRENT_POINTS");
        Bind(result_, "RESULT"); Bind(imageWidth_, "image_width"); Bind(imageHeight_, "image_height");
        Bind(levels_, "levels"); Bind(windowRadius_, "window_radius"); Bind(iterations_, "iterations");
        Bind(maxResidual_, "max_residual"); Bind(forwardBackwardThreshold_, "forward_backward_threshold");
        Bind(minPoints_, "min_points"); Bind(maxPoints_, "max_points");
        if(image_.shape() != std::vector<int>({static_cast<int>(imageHeight_), static_cast<int>(imageWidth_)}))
            throw std::runtime_error("PyramidalLucasKanadeTracker IMAGE has unexpected shape");
        for(int level = 0; level < static_cast<int>(levels_); ++level)
        {
            previousPyramid_[level].realloc(static_cast<int>(imageHeight_) >> level,
                                             static_cast<int>(imageWidth_) >> level);
            currentPyramid_[level].realloc(static_cast<int>(imageHeight_) >> level,
                                            static_cast<int>(imageWidth_) >> level);
        }
        const int capacity = static_cast<int>(maxPoints_);
        referencePoints_.realloc(capacity, 2); previousPoints_.realloc(capacity, 2);
        nextPoints_.realloc(capacity, 2); acceptedReference_.realloc(capacity, 2);
        acceptedCurrent_.realloc(capacity, 2);
    }

    void Tick() override
    {
        referenceOutput_.clear(); currentOutput_.clear(); result_.clear();
        buildPyramid(image_, currentPyramid_);
        if(!hasPreviousImage_)
        {
            previousPyramid_.swap(currentPyramid_);
            hasPreviousImage_ = true;
            seed();
            return;
        }
        if(!active_)
        {
            seed();
            previousPyramid_.swap(currentPyramid_);
            return;
        }
        acceptedReference_.resize(static_cast<int>(maxPoints_), 2);
        acceptedCurrent_.resize(static_cast<int>(maxPoints_), 2);
        int accepted = 0;
        float totalForwardBackward = 0.0f;
        for(int i = 0; i < previousPoints_.rows(); ++i)
        {
            float x, y, residual;
            if(!trackPoint(previousPyramid_, currentPyramid_, static_cast<int>(levels_),
                           static_cast<int>(windowRadius_), static_cast<int>(iterations_),
                           previousPoints_(i, 0), previousPoints_(i, 1), x, y, residual) ||
               residual > static_cast<float>(maxResidual_))
                continue;
            float backX, backY, backResidual;
            if(!trackPoint(currentPyramid_, previousPyramid_, static_cast<int>(levels_),
                           static_cast<int>(windowRadius_), static_cast<int>(iterations_),
                           x, y, backX, backY, backResidual))
                continue;
            const float fb = std::hypot(backX - previousPoints_(i, 0),
                                        backY - previousPoints_(i, 1));
            if(fb > static_cast<float>(forwardBackwardThreshold_))
                continue;
            acceptedReference_(accepted, 0) = referencePoints_(i, 0);
            acceptedReference_(accepted, 1) = referencePoints_(i, 1);
            acceptedCurrent_(accepted, 0) = x;
            acceptedCurrent_(accepted, 1) = y;
            totalForwardBackward += fb;
            ++accepted;
        }
        acceptedReference_.resize(accepted, 2); acceptedCurrent_.resize(accepted, 2);
        if(accepted < static_cast<int>(minPoints_) ||
           !geometry_.Similarity(acceptedReference_, acceptedCurrent_, transform_))
        {
            active_ = false;
            previousPyramid_.swap(currentPyramid_);
            return;
        }
        referencePoints_.resize(accepted, 2); previousPoints_.resize(accepted, 2);
        referencePoints_.copy(acceptedReference_); previousPoints_.copy(acceptedCurrent_);
        referenceOutput_.resize(accepted, 2); currentOutput_.resize(accepted, 2);
        referenceOutput_.copy(referencePoints_); currentOutput_.copy(previousPoints_);
        result_.resize(1, 4);
        result_(0, 0) = static_cast<float>(templateIndex_);
        result_(0, 1) = static_cast<float>(accepted);
        result_(0, 2) = totalForwardBackward / accepted;
        result_(0, 3) = 1.0f;
        previousPyramid_.swap(currentPyramid_);
    }
};

INSTALL_CLASS(PyramidalLucasKanadeTracker)
