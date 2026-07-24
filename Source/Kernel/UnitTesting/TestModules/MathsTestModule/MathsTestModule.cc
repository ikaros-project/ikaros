#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

#include "ikaros.h"

using namespace ikaros;

namespace
{
    void
    require_close(double actual, double expected, double tolerance,
                  const std::string & message)
    {
        if(!std::isfinite(actual) ||
           std::fabs(actual - expected) > tolerance * std::max(1.0, std::fabs(expected)))
            throw exception("MathsTestModule: " + message +
                            " (expected " + std::to_string(expected) +
                            ", got " + std::to_string(actual) + ")");
    }


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

        require_close(exgaussian(0.0, 1.0, 0.0, 1.0),
                      0.2615782918651234, 1.0e-14,
                      "central ex-Gaussian density");
        require_close(exgaussian(0.0, 0.01, 0.0, 1.0),
                      0.3989023981356811, 1.0e-14,
                      "small-K ex-Gaussian density");
        require_close(exgaussian(0.0, 1.0, 0.0, 1.0, -2.0),
                      -0.5231565837302468, 1.0e-14,
                      "signed ex-Gaussian amplitude");
        if(exgaussian(-1000.0, 1.0, 0.0, 1.0) != 0.0 ||
           exgaussian(1000.0, 1.0, 0.0, 1.0) != 0.0)
            throw exception("MathsTestModule: ex-Gaussian tails did not underflow safely");

        const double double_nan = std::numeric_limits<double>::quiet_NaN();
        const double double_infinity = std::numeric_limits<double>::infinity();
        require_invalid_argument(
            [double_nan] { (void)exgaussian(0.0, double_nan, 0.0, 1.0); },
            "NaN ex-Gaussian K was accepted");
        require_invalid_argument(
            [double_infinity] { (void)exgaussian(0.0, 1.0, 0.0, double_infinity); },
            "infinite ex-Gaussian sigma was accepted");
        require_invalid_argument(
            [double_nan] { (void)exgaussian(double_nan, 1.0, 0.0, 1.0); },
            "NaN ex-Gaussian x was accepted");
        require_invalid_argument(
            [double_infinity] { (void)exgaussian(0.0, 1.0, double_infinity, 1.0); },
            "infinite ex-Gaussian mu was accepted");
        require_invalid_argument(
            [double_infinity] {
                (void)exgaussian(0.0, 1.0, 0.0, 1.0, double_infinity);
            },
            "infinite ex-Gaussian amplitude was accepted");

        std::cout << "MATHS TEST OK" << std::endl;
    }
};

INSTALL_CLASS(MathsTestModule)
