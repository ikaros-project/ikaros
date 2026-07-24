#include <limits>
#include <stdexcept>
#include <string>

#include "ikaros.h"

using namespace ikaros;

namespace
{
    template<typename Function>
    void
    require_invalid_argument(Function function, const std::string & message)
    {
        try
        {
            function();
        }
        catch(const std::invalid_argument &)
        {
            return;
        }
        throw exception("MathsTestModule: " + message);
    }
}


class MathsTestModule : public Module
{
    void Init() override
    {
        if(sample_normal_distribution(3.25f, 0.0f) != 3.25f)
            throw exception("MathsTestModule: zero deviation did not return the mean");
        if(sample_normal_distribution(-2.0f, -0.0f) != -2.0f)
            throw exception("MathsTestModule: negative zero deviation did not return the mean");

        const float nan = std::numeric_limits<float>::quiet_NaN();
        const float infinity = std::numeric_limits<float>::infinity();
        require_invalid_argument(
            [] { (void)sample_normal_distribution(0.0f, -1.0f); },
            "negative Gaussian deviation was accepted");
        require_invalid_argument(
            [nan] { (void)sample_normal_distribution(0.0f, nan); },
            "NaN Gaussian deviation was accepted");
        require_invalid_argument(
            [infinity] { (void)sample_normal_distribution(0.0f, infinity); },
            "infinite Gaussian deviation was accepted");
        require_invalid_argument(
            [nan] { (void)sample_normal_distribution(nan, 1.0f); },
            "NaN Gaussian mean was accepted");
        require_invalid_argument(
            [infinity] { (void)sample_normal_distribution(infinity, 1.0f); },
            "infinite Gaussian mean was accepted");

        std::cout << "MATHS TEST OK" << std::endl;
    }
};

INSTALL_CLASS(MathsTestModule)
