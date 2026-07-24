#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <limits>
#include <numbers>
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

        if(sgn(-3.0) != -1.0 || sgn(0.0) != 0.0 || sgn(3.0) != 1.0)
            throw exception("MathsTestModule: sign function returned an incorrect value");
        if(!std::isnan(sgn(std::numeric_limits<double>::quiet_NaN())))
            throw exception("MathsTestModule: sign function did not propagate NaN");

        require_close(clip(-2.0, -1.0, 1.0), -1.0, 0.0,
                      "clip lower bound");
        require_close(clip(2.0, -1.0, 1.0), 1.0, 0.0,
                      "clip upper bound");
        require_close(
            clip(2.0, -std::numeric_limits<double>::infinity(),
                 std::numeric_limits<double>::infinity()),
            2.0, 0.0, "clip infinite bounds");
        if(!std::isnan(clip(std::numeric_limits<double>::quiet_NaN(),
                            -1.0, 1.0)))
            throw exception("MathsTestModule: clip did not propagate a NaN value");
        require_invalid_argument(
            [] { (void)clip(0.0, 1.0, -1.0); },
            "reversed clip bounds were accepted");
        require_invalid_argument(
            [] {
                (void)clip(
                    0.0, std::numeric_limits<double>::quiet_NaN(), 1.0);
            },
            "NaN lower clip bound was accepted");
        require_invalid_argument(
            [] {
                (void)clip(
                    0.0, -1.0, std::numeric_limits<double>::quiet_NaN());
            },
            "NaN upper clip bound was accepted");

        std::mt19937 first_generator(12345);
        std::mt19937 second_generator(12345);
        for(int sample = 0; sample < 32; ++sample)
            if(sample_normal_distribution(first_generator, 1.0f, 2.0f) !=
               sample_normal_distribution(second_generator, 1.0f, 2.0f))
                throw exception("MathsTestModule: seeded Gaussian sequences differ");

        std::normal_distribution<float> first_distribution;
        std::normal_distribution<float> second_distribution;
        for(int sample = 0; sample < 32; ++sample)
            if(sample_normal_distribution(
                   first_generator, first_distribution, 1.0f, 2.0f) !=
               sample_normal_distribution(
                   second_generator, second_distribution, 1.0f, 2.0f))
                throw exception(
                    "MathsTestModule: cached seeded Gaussian sequences differ");

        std::mt19937 zero_deviation_generator(54321);
        std::mt19937 untouched_generator(54321);
        (void)sample_normal_distribution(
            zero_deviation_generator, 4.0f, 0.0f);
        if(zero_deviation_generator() != untouched_generator())
            throw exception("MathsTestModule: zero deviation advanced its generator");

        const double largest = std::numeric_limits<double>::max();
        if(angle_to_angle(largest, angle_unit::degrees,
                         angle_unit::degrees) != largest ||
           angle_to_angle(largest, angle_unit::radians,
                         angle_unit::radians) != largest ||
           angle_to_angle(largest, angle_unit::turns,
                         angle_unit::turns) != largest)
            throw exception("MathsTestModule: identity angle conversion changed its input");
        require_close(
            angle_to_angle(
                180.0, angle_unit::degrees, angle_unit::radians),
            std::numbers::pi_v<double>, 1.0e-15, "degrees to radians");
        require_close(angle_to_angle(
                          std::numbers::pi_v<double>,
                          angle_unit::radians, angle_unit::degrees),
                      180.0, 1.0e-15, "radians to degrees");
        require_close(
            angle_to_angle(
                1.0, angle_unit::turns, angle_unit::radians),
            2.0 * std::numbers::pi_v<double>, 1.0e-15,
            "turns to radians");
        require_close(angle_to_angle(
                          360.0, angle_unit::degrees,
                          angle_unit::turns),
                      1.0, 1.0e-15, "degrees to turns");
        require_invalid_argument(
            [] {
                (void)angle_to_angle(
                    1.0, static_cast<angle_unit>(99),
                    angle_unit::radians);
            },
            "invalid source angle unit was accepted");
        require_invalid_argument(
            [] {
                (void)angle_to_angle(
                    1.0, angle_unit::degrees,
                    static_cast<angle_unit>(99));
            },
            "invalid target angle unit was accepted");

        const double math_pi = std::numbers::pi_v<double>;
        require_close(short_angle(0.0, 0.5 * math_pi),
                      0.5 * math_pi, 1.0e-15,
                      "positive short angle");
        require_close(short_angle(0.0, 1.5 * math_pi),
                      -0.5 * math_pi, 1.0e-15,
                      "wrapped short angle");
        require_close(short_angle(1.5 * math_pi, 0.0),
                      0.5 * math_pi, 1.0e-15,
                      "reverse wrapped short angle");
        if(std::fabs(std::fabs(short_angle(0.0, 3.0 * math_pi)) -
                     math_pi) > 1.0e-15)
            throw exception(
                "MathsTestModule: antipodal short angle had wrong magnitude");
        const double finite_extreme =
            short_angle(-largest, largest);
        if(!std::isfinite(finite_extreme) ||
           std::fabs(finite_extreme) > math_pi)
            throw exception(
                "MathsTestModule: finite extreme angles did not wrap safely");
        if(!std::isnan(short_angle(
               0.0, std::numeric_limits<double>::infinity())))
            throw exception(
                "MathsTestModule: non-finite short angle did not produce NaN");

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


class GaussianSequenceTestModule : public Module
{
    matrix first;
    matrix second;
    bool verified = false;

    void Init() override
    {
        Bind(first, "FIRST");
        Bind(second, "SECOND");
    }

    void Tick() override
    {
        if(verified || GetTick() < 3)
            return;
        if(first.shape() != second.shape())
            throw exception("GaussianSequenceTestModule: sequence shapes differ");

        bool contains_nonzero_value = false;
        for(int index = 0; index < first.size(); ++index)
        {
            if(first(index) != second(index))
                throw exception("GaussianSequenceTestModule: seeded module sequences differ");
            contains_nonzero_value = contains_nonzero_value || first(index) != 0.0f;
        }
        if(!contains_nonzero_value)
            throw exception("GaussianSequenceTestModule: sequence was not populated");

        verified = true;
        std::cout << "GAUSSIAN SEQUENCE TEST OK" << std::endl;
    }
};


class MathsBenchmarkModule : public Module
{
    void Init() override
    {
        constexpr int samples = 20000000;
        double checksum = 0.0;

        const auto start = std::chrono::steady_clock::now();
        for(int sample = 0; sample < samples; ++sample)
            checksum += sample_normal_distribution(1.0f, 2.0f);
        const double elapsed = std::chrono::duration<double, std::nano>(
            std::chrono::steady_clock::now() - start).count();

        double angle = 0.0;
        const auto angle_start = std::chrono::steady_clock::now();
        for(int sample = 0; sample < samples; ++sample)
        {
            angle += 0.001;
            checksum += short_angle(angle, -0.5 * angle);
        }
        const double angle_elapsed = std::chrono::duration<double, std::nano>(
            std::chrono::steady_clock::now() - angle_start).count();

        std::cout << std::fixed << std::setprecision(3)
                  << "MATHS BENCHMARK"
                  << " gaussian_ns=" << elapsed / samples
                  << " short_angle_ns=" << angle_elapsed / samples
                  << " checksum=" << checksum << std::endl;
    }
};


INSTALL_CLASS(MathsTestModule)
INSTALL_CLASS(GaussianSequenceTestModule)
INSTALL_CLASS(MathsBenchmarkModule)
