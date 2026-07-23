#include <algorithm>
#include <cmath>
#include <iostream>

#include "ikaros.h"

using namespace ikaros;


class LaunchpadAnimationTestModule: public Module
{
public:
    void
    Init() override
    {
        Bind(color_, "COLOR");
        if(color_.rank() != 2 || color_.rows() != 64 || color_.cols() != 3)
            throw exception(
                "LaunchpadAnimationTestModule received the wrong COLOR shape.");
        previous_ = matrix(color_.shape());
        previous_.reset();
    }


    void
    Tick() override
    {
        bool colorful = false;
        float maximum = 0;
        for(int pad = 0; pad < color_.rows(); ++pad)
        {
            const float red = color_(pad, 0);
            const float green = color_(pad, 1);
            const float blue = color_(pad, 2);
            if(!std::isfinite(red) || !std::isfinite(green) ||
               !std::isfinite(blue) || red < 0 || red > 1 ||
               green < 0 || green > 1 || blue < 0 || blue > 1)
                throw exception(
                    "LaunchpadAnimation produced an invalid RGB component.");
            maximum = std::max({maximum, red, green, blue});
            colorful = colorful || std::abs(red - green) > 0.05f ||
                       std::abs(green - blue) > 0.05f;
        }

        if(frameCount_ > 0 && !(color_ == previous_))
            changed_ = true;
        richColor_ = richColor_ || (maximum > 0.2f && colorful);
        previous_.copy(color_);
        ++frameCount_;

        if(frameCount_ == 4)
        {
            if(!changed_ || !richColor_)
                throw exception(
                    "LaunchpadAnimation did not produce a changing color pattern.");
            std::cout << "LAUNCHPAD ANIMATION TEST OK" << std::endl;
        }
    }

private:
    matrix color_;
    matrix previous_;
    int frameCount_ = 0;
    bool changed_ = false;
    bool richColor_ = false;
};

INSTALL_CLASS(LaunchpadAnimationTestModule)
