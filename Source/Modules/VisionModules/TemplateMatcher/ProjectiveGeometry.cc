#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "ProjectiveGeometry.h"

namespace ikaros
{
    namespace
    {
        bool
        normalizePoints(const matrix & input, matrix & output, matrix & normalization)
        {
            const int count = input.rows();
            float centerX = 0.0f;
            float centerY = 0.0f;
            for(int i = 0; i < count; ++i)
            {
                centerX += input(i, 0);
                centerY += input(i, 1);
            }
            centerX /= count;
            centerY /= count;
            float meanDistance = 0.0f;
            for(int i = 0; i < count; ++i)
                meanDistance += std::hypot(input(i, 0) - centerX,
                                           input(i, 1) - centerY);
            meanDistance /= count;
            if(meanDistance < 1.0e-6f)
                return false;
            const float scale = std::sqrt(2.0f) / meanDistance;
            output.resize(count, 2);
            for(int i = 0; i < count; ++i)
            {
                output(i, 0) = scale * (input(i, 0) - centerX);
                output(i, 1) = scale * (input(i, 1) - centerY);
            }
            normalization.reset();
            normalization(0, 0) = scale;
            normalization(1, 1) = scale;
            normalization(0, 2) = -scale * centerX;
            normalization(1, 2) = -scale * centerY;
            normalization(2, 2) = 1.0f;
            return true;
        }
    }


    ProjectiveGeometry::ProjectiveGeometry(int maxPoints)
        : maxPoints_(maxPoints),
          normalizedSource_(maxPoints, 2),
          normalizedTarget_(maxPoints, 2),
          design_(2 * maxPoints, 9),
          designTranspose_(9, 2 * maxPoints),
          normal_(9, 9), u_(9, 9), singular_(9, 9), vt_(9, 9),
          normalizedTransform_(3, 3), sourceNormalization_(3, 3),
          targetNormalization_(3, 3), targetNormalizationInverse_(3, 3),
          temporary_(3, 3)
    {
        if(maxPoints < 4)
            throw std::invalid_argument("ProjectiveGeometry requires at least four points");
    }


    bool
    ProjectiveGeometry::Homography(const matrix & source, const matrix & target,
                                   matrix & result)
    {
        if(source.rank() != 2 || target.rank() != 2 || source.cols() != 2 ||
           target.cols() != 2 || source.rows() != target.rows() ||
           source.rows() < 4 || source.rows() > maxPoints_)
            return false;
        if(!normalizePoints(source, normalizedSource_, sourceNormalization_) ||
           !normalizePoints(target, normalizedTarget_, targetNormalization_))
            return false;

        const int count = source.rows();
        design_.resize(2 * count, 9);
        for(int i = 0; i < count; ++i)
        {
            const float x = normalizedSource_(i, 0);
            const float y = normalizedSource_(i, 1);
            const float u = normalizedTarget_(i, 0);
            const float v = normalizedTarget_(i, 1);
            design_(2 * i, 0) = -x;
            design_(2 * i, 1) = -y;
            design_(2 * i, 2) = -1;
            design_(2 * i, 3) = 0;
            design_(2 * i, 4) = 0;
            design_(2 * i, 5) = 0;
            design_(2 * i, 6) = u * x;
            design_(2 * i, 7) = u * y;
            design_(2 * i, 8) = u;
            design_(2 * i + 1, 0) = 0;
            design_(2 * i + 1, 1) = 0;
            design_(2 * i + 1, 2) = 0;
            design_(2 * i + 1, 3) = -x;
            design_(2 * i + 1, 4) = -y;
            design_(2 * i + 1, 5) = -1;
            design_(2 * i + 1, 6) = v * x;
            design_(2 * i + 1, 7) = v * y;
            design_(2 * i + 1, 8) = v;
        }
        designTranspose_.resize(9, 2 * count);
        design_.transpose(designTranspose_);
        normal_.matmul(designTranspose_, design_);
        normal_.singular_value_decomposition(normal_, u_, singular_, vt_);
        for(int i = 0; i < 9; ++i)
            normalizedTransform_(i / 3, i % 3) = vt_(8, i);
        targetNormalizationInverse_.inv(targetNormalization_);
        temporary_.matmul(targetNormalizationInverse_, normalizedTransform_);
        result.matmul(temporary_, sourceNormalization_);
        const float divisor = result(2, 2);
        if(!std::isfinite(divisor) || std::abs(divisor) < 1.0e-8f)
            return false;
        result.scale(1.0f / divisor);
        for(int i = 0; i < result.size(); ++i)
            if(!std::isfinite(result.data()[i]))
                return false;
        return true;
    }


    bool
    ProjectiveGeometry::Similarity(const matrix & source, const matrix & target,
                                   matrix & result) const
    {
        if(source.rows() != target.rows() || source.rows() < 2 ||
           source.cols() != 2 || target.cols() != 2)
            return false;
        float sx = 0, sy = 0, tx = 0, ty = 0;
        for(int i = 0; i < source.rows(); ++i)
        {
            sx += source(i, 0); sy += source(i, 1);
            tx += target(i, 0); ty += target(i, 1);
        }
        sx /= source.rows(); sy /= source.rows();
        tx /= source.rows(); ty /= source.rows();
        float denominator = 0, a = 0, b = 0;
        for(int i = 0; i < source.rows(); ++i)
        {
            const float x = source(i, 0) - sx;
            const float y = source(i, 1) - sy;
            const float u = target(i, 0) - tx;
            const float v = target(i, 1) - ty;
            denominator += x * x + y * y;
            a += x * u + y * v;
            b += x * v - y * u;
        }
        if(denominator < 1.0e-8f)
            return false;
        a /= denominator;
        b /= denominator;
        result.reset();
        result(0, 0) = a; result(0, 1) = -b;
        result(1, 0) = b; result(1, 1) = a;
        result(0, 2) = tx - a * sx + b * sy;
        result(1, 2) = ty - b * sx - a * sy;
        result(2, 2) = 1.0f;
        return true;
    }


    bool
    ProjectiveGeometry::Project(const matrix & transform, float x, float y,
                                float & targetX, float & targetY)
    {
        const float w = transform(2, 0) * x + transform(2, 1) * y + transform(2, 2);
        if(!std::isfinite(w) || std::abs(w) < 1.0e-8f)
            return false;
        targetX = (transform(0, 0) * x + transform(0, 1) * y + transform(0, 2)) / w;
        targetY = (transform(1, 0) * x + transform(1, 1) * y + transform(1, 2)) / w;
        return std::isfinite(targetX) && std::isfinite(targetY);
    }
}
