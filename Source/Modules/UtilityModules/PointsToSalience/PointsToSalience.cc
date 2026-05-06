#include "ikaros.h"

#include <algorithm>
#include <cmath>

using namespace ikaros;

class PointsToSalience : public Module
{
    matrix points_;
    matrix widths_;
    matrix output_;
    parameter standard_deviation_;
    parameter size_x_;
    parameter size_y_;

    int
    point_count() const
    {
        if(points_.empty())
            return 0;
        if(points_.rank() == 2)
            return points_.rows();
        return points_.size() >= 2 ? 1 : 0;
    }

    bool
    point(int index, float & x, float & y) const
    {
        if(points_.rank() == 2)
        {
            if(points_.cols() < 2 || index >= points_.rows())
                return false;
            x = points_(index, 0);
            y = points_(index, 1);
            return true;
        }

        if(index != 0 || points_.size() < 2)
            return false;
        x = points_.data()[0];
        y = points_.data()[1];
        return true;
    }

    float
    width(int index) const
    {
        if(!widths_.connected() || widths_.empty())
            return std::max(0.0f, static_cast<float>(standard_deviation_));

        if(widths_.rank() == 2)
        {
            if(index < widths_.rows() && widths_.cols() > 0)
                return std::max(0.0f, widths_(index, 0));
            return std::max(0.0f, static_cast<float>(standard_deviation_));
        }

        if(index < widths_.size())
            return std::max(0.0f, widths_.data()[index]);

        return std::max(0.0f, static_cast<float>(standard_deviation_));
    }

public:
    void Init() override
    {
        Bind(points_, "POINTS");
        Bind(widths_, "WIDTHS");
        Bind(output_, "OUTPUT");
        Bind(standard_deviation_, "standard_deviation");
        Bind(size_x_, "size_x");
        Bind(size_y_, "size_y");
    }

    void Tick() override
    {
        output_.reset();

        int width_pixels = output_.size_x();
        int height_pixels = output_.size_y();
        if(width_pixels <= 0 || height_pixels <= 0)
            return;

        int n = point_count();
        for(int i = 0; i < n; ++i)
        {
            float px = 0.0f;
            float py = 0.0f;
            if(!point(i, px, py))
                continue;

            float sigma = width(i);
            if(sigma <= 0.0f)
                continue;

            float inv_two_sigma2 = 1.0f / (2.0f * sigma * sigma);
            for(int y = 0; y < height_pixels; ++y)
            {
                float yy = height_pixels > 1 ? 2.0f * static_cast<float>(y) / static_cast<float>(height_pixels - 1) - 1.0f : 0.0f;
                float dy = yy - py;
                for(int x = 0; x < width_pixels; ++x)
                {
                    float xx = width_pixels > 1 ? 2.0f * static_cast<float>(x) / static_cast<float>(width_pixels - 1) - 1.0f : 0.0f;
                    float dx = xx - px;
                    output_(y, x) += std::exp(-(dx * dx + dy * dy) * inv_two_sigma2);
                }
            }
        }
    }
};

INSTALL_CLASS(PointsToSalience)
