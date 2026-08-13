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


class RandomSequenceTestModule : public Module
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
            throw exception("RandomSequenceTestModule: sequence shapes differ");

        bool contains_nonzero_value = false;
        for(int index = 0; index < first.size(); ++index)
        {
            if(first(index) != second(index))
                throw exception("RandomSequenceTestModule: seeded module sequences differ");
            contains_nonzero_value = contains_nonzero_value || first(index) != 0.0f;
        }
        if(!contains_nonzero_value)
            throw exception("RandomSequenceTestModule: sequence was not populated");

        verified = true;
        std::cout << "RANDOM SEQUENCE TEST OK" << std::endl;
    }
};


class RotationUnitTestModule : public Module
{
    matrix degreeMatrix;
    matrix radianMatrix;
    matrix turnMatrix;
    matrix tauMatrix;
    matrix degreeAngles;
    matrix radianAngles;
    matrix turnAngles;
    matrix tauAngles;
    bool verified = false;

    void RequireSameMatrix(
        const matrix & actual, const matrix & expected,
        const std::string & message)
    {
        if(actual.shape() != expected.shape())
            throw exception(
                "RotationUnitTestModule: " + message + " shape differs");
        for(int index = 0; index < actual.size(); ++index)
            require_close(
                actual.data()[index], expected.data()[index],
                1.0e-5, message);
    }

    void RequireAngles(
        const matrix & actual, double expectedX,
        const std::string & message)
    {
        if(actual.size() != 3)
            throw exception(
                "RotationUnitTestModule: " + message + " shape differs");
        require_close(actual.data()[0], expectedX, 1.0e-5, message + " x");
        require_close(actual.data()[1], 0.0, 1.0e-5, message + " y");
        require_close(actual.data()[2], 0.0, 1.0e-5, message + " z");
    }

    void Init() override
    {
        Bind(degreeMatrix, "DEGREE_MATRIX");
        Bind(radianMatrix, "RADIAN_MATRIX");
        Bind(turnMatrix, "TURN_MATRIX");
        Bind(tauMatrix, "TAU_MATRIX");
        Bind(degreeAngles, "DEGREE_ANGLES");
        Bind(radianAngles, "RADIAN_ANGLES");
        Bind(turnAngles, "TURN_ANGLES");
        Bind(tauAngles, "TAU_ANGLES");
    }

    void Tick() override
    {
        if(verified || GetTick() < 3)
            return;

        RequireSameMatrix(radianMatrix, degreeMatrix, "radian input");
        RequireSameMatrix(turnMatrix, degreeMatrix, "turn input");
        RequireSameMatrix(tauMatrix, degreeMatrix, "tau input");

        require_close(degreeMatrix(0, 0), 1.0, 1.0e-5,
                      "degree rotation xx");
        require_close(degreeMatrix(1, 2), -1.0, 1.0e-5,
                      "degree rotation yz");
        require_close(degreeMatrix(2, 1), 1.0, 1.0e-5,
                      "degree rotation zy");

        RequireAngles(degreeAngles, 90.0, "degree output");
        RequireAngles(
            radianAngles, 0.5 * std::numbers::pi_v<double>,
            "radian output");
        RequireAngles(turnAngles, 0.25, "turn output");
        RequireAngles(tauAngles, 0.25, "tau output");

        verified = true;
        std::cout << "ROTATION UNIT TEST OK" << std::endl;
    }
};


class UniformBoundsRuntimeTestModule : public Module
{
    matrix noise;
    matrix randomizer;
    parameter stage_;
    bool observedEqualBounds = false;
    bool observedInvalidBounds = false;
    bool completed = false;

    void ValidateValues(const matrix & values, const std::string & name)
    {
        for(int index = 0; index < values.size(); ++index)
        {
            const float value = values.data()[index];
            if(!std::isfinite(value) || value < -4.0f || value > 3.0f)
                throw exception(
                    "UniformBoundsRuntimeTestModule: " + name +
                    " emitted an invalid value");
        }
    }

    bool AllEqual(const matrix & values, float expected)
    {
        for(int index = 0; index < values.size(); ++index)
            if(values.data()[index] != expected)
                return false;
        return true;
    }

    void Init() override
    {
        Bind(noise, "NOISE");
        Bind(randomizer, "RANDOMIZER");
        Bind(stage_, "stage");
    }

    void Tick() override
    {
        if(completed || GetTick() < 2)
            return;

        ValidateValues(noise, "Noise");
        ValidateValues(randomizer, "Randomizer");

        const bool noiseEqual = AllEqual(noise, 1.25f);
        const bool randomizerEqual = AllEqual(randomizer, 1.25f);
        const int stage = stage_.as_int();
        if(stage == 1)
        {
            if(!noiseEqual || !randomizerEqual)
                throw exception(
                    "UniformBoundsRuntimeTestModule: equal bounds did not "
                    "produce the bound value");
            if(!observedEqualBounds)
            {
                observedEqualBounds = true;
                std::cout << "UNIFORM BOUNDS EQUAL OK" << std::endl;
            }
        }
        else if(stage == 2)
        {
            if(!noiseEqual || !randomizerEqual)
                throw exception(
                    "UniformBoundsRuntimeTestModule: invalid bounds did not "
                    "retain the last valid range");
            if(!observedInvalidBounds)
            {
                observedInvalidBounds = true;
                std::cout << "UNIFORM BOUNDS FALLBACK OK" << std::endl;
            }
        }
        else if(stage == 3)
        {
            if(noiseEqual || randomizerEqual)
                throw exception(
                    "UniformBoundsRuntimeTestModule: valid bounds did not "
                    "recover after an invalid update");
            completed = true;
            std::cout << "UNIFORM BOUNDS RECOVERY OK" << std::endl;
        }
    }
};


class SoftmaxInputTestModule : public Module
{
    matrix original;
    matrix softmax;
    bool verified = false;

    void Init() override
    {
        Bind(original, "ORIGINAL");
        Bind(softmax, "SOFTMAX");
    }

    void Tick() override
    {
        if(verified || GetTick() < 2)
            return;
        if(original.size() != 4 || softmax.size() != 4)
            throw exception("SoftmaxInputTestModule: unexpected matrix size");

        constexpr float logits[] = {-1.0f, 0.0f, 1.0f, 2.0f};
        double exponentialSum = 0.0;
        for(float logit : logits)
            exponentialSum += std::exp(static_cast<double>(logit));

        double probabilitySum = 0.0;
        for(int index = 0; index < 4; ++index)
        {
            require_close(
                original(index), logits[index], 0.0,
                "Softmax changed its input");
            require_close(
                softmax(index),
                std::exp(static_cast<double>(logits[index])) /
                    exponentialSum,
                1.0e-6, "Softmax probability");
            probabilitySum += softmax(index);
        }
        require_close(
            probabilitySum, 1.0, 1.0e-6,
            "Softmax probabilities did not sum to one");

        verified = true;
        std::cout << "SOFTMAX INPUT TEST OK" << std::endl;
    }
};


class NormalizeZeroTestModule : public Module
{
    matrix range;
    matrix euclidean;
    matrix cityBlock;
    matrix maximum;
    bool verified = false;

    void RequireZero(const matrix & values, const std::string & name)
    {
        if(values.size() != 3)
            throw exception(
                "NormalizeZeroTestModule: unexpected " + name + " size");
        for(int index = 0; index < values.size(); ++index)
            if(values(index) != 0.0f)
                throw exception(
                    "NormalizeZeroTestModule: " + name +
                    " did not produce zeros");
    }

    void Init() override
    {
        Bind(range, "RANGE");
        Bind(euclidean, "EUCLIDEAN");
        Bind(cityBlock, "CITY_BLOCK");
        Bind(maximum, "MAXIMUM");
    }

    void Tick() override
    {
        if(verified || GetTick() < 2)
            return;

        RequireZero(range, "range");
        RequireZero(euclidean, "euclidean");
        RequireZero(cityBlock, "city-block");
        RequireZero(maximum, "maximum");

        verified = true;
        std::cout << "NORMALIZE ZERO TEST OK" << std::endl;
    }
};


class NormalizeFiniteTestModule : public Module
{
    matrix range;
    matrix euclidean;
    matrix cityBlock;
    bool verified = false;

    void Init() override
    {
        Bind(range, "RANGE");
        Bind(euclidean, "EUCLIDEAN");
        Bind(cityBlock, "CITY_BLOCK");
    }

    void Tick() override
    {
        if(verified || GetTick() < 2)
            return;
        if(range.size() != 2 || euclidean.size() != 2 ||
           cityBlock.size() != 2)
            throw exception(
                "NormalizeFiniteTestModule: unexpected matrix size");

        require_close(range(0), 0.0, 0.0, "finite range minimum");
        require_close(range(1), 1.0, 0.0, "finite range maximum");

        const double inverseSquareRootOfTwo =
            1.0 / std::sqrt(2.0);
        require_close(
            euclidean(0), inverseSquareRootOfTwo, 1.0e-6,
            "finite Euclidean first value");
        require_close(
            euclidean(1), inverseSquareRootOfTwo, 1.0e-6,
            "finite Euclidean second value");
        require_close(
            cityBlock(0), 0.5, 1.0e-6,
            "finite city-block first value");
        require_close(
            cityBlock(1), 0.5, 1.0e-6,
            "finite city-block second value");

        verified = true;
        std::cout << "NORMALIZE FINITE TEST OK" << std::endl;
    }
};


class RegressionCapacityTestModule : public Module
{
    matrix scatterX;
    matrix sampleCount;
    parameter stage_;
    bool completed = false;

    void Init() override
    {
        Bind(scatterX, "SCATTER_X");
        Bind(sampleCount, "SAMPLE_COUNT");
        Bind(stage_, "stage");
    }

    void Tick() override
    {
        if(completed || stage_.as_int() == 0)
            return;
        if(scatterX.size() != 3)
            throw exception(
                "RegressionCapacityTestModule: scatter shape changed");
        if(sampleCount.size() != 1 || sampleCount(0) != 3.0f)
            throw exception(
                "RegressionCapacityTestModule: sample capacity changed");

        if(stage_.as_int() == 3)
        {
            completed = true;
            std::cout << "REGRESSION CAPACITY TEST OK" << std::endl;
        }
    }
};


class RegressionModelComparisonTestModule : public Module
{
    matrix x;
    matrix y;
    matrix modelComparison;
    bool verified = false;

    void Init() override
    {
        Bind(x, "X");
        Bind(y, "Y");
        Bind(modelComparison, "MODEL_COMPARISON");
    }

    void Tick() override
    {
        const double sample = static_cast<double>(GetTick());
        const double xValue = sample * 1.0e-8;
        const double noise0 =
            GetTick() % 2 == 0 ? -0.001 : 0.001;
        const double noise1 =
            GetTick() % 3 == 0 ? -0.0015 : 0.00075;
        x(0) = static_cast<float>(xValue);
        y(0) = static_cast<float>(1.0 + 1.0e6 * xValue + noise0);
        y(1) = static_cast<float>(2.0 + 2.0e6 * xValue + noise1);

        if(verified || GetTick() < 20)
            return;
        if(modelComparison.rank() != 2 ||
           modelComparison.rows() != 7 ||
           modelComparison.cols() != 2)
            throw exception(
                "RegressionModelComparisonTestModule: unexpected output "
                "shape");

        for(int index = 0; index < modelComparison.size(); ++index)
            if(!std::isfinite(modelComparison.data()[index]))
                throw exception(
                    "RegressionModelComparisonTestModule: non-finite "
                    "model comparison");
        for(int column = 0; column < 2; ++column)
        {
            if(modelComparison(0, column) < 0.0f ||
               modelComparison(0, column) > 1.0f ||
               modelComparison(4, column) < 0.0f ||
               modelComparison(4, column) > 1.0f)
                throw exception(
                    "RegressionModelComparisonTestModule: probability or "
                    "effect size is out of range");
        }
        require_close(
            modelComparison(2, 0), 1.0, 0.0,
            "intercept-comparison numerator degrees of freedom");
        require_close(
            modelComparison(2, 1), 1.0, 0.0,
            "slope-comparison numerator degrees of freedom");
        require_close(
            modelComparison(5, 0), 2.0, 0.0,
            "intercept-comparison group count");
        require_close(
            modelComparison(5, 1), 2.0, 0.0,
            "slope-comparison group count");
        require_close(
            modelComparison(6, 0), modelComparison(6, 1), 0.0,
            "model-comparison sample counts");
        require_close(
            modelComparison(3, 0), modelComparison(6, 0) - 3.0, 0.0,
            "intercept-comparison denominator degrees of freedom");
        require_close(
            modelComparison(3, 1), modelComparison(6, 1) - 4.0, 0.0,
            "slope-comparison denominator degrees of freedom");

        verified = true;
        std::cout << "REGRESSION MODEL TEST OK" << std::endl;
    }
};


class RegressionMaskTestModule : public Module
{
    matrix x;
    matrix y;
    matrix sample;
    matrix sampleCount;
    bool verified = false;

    void Init() override
    {
        Bind(x, "X");
        Bind(y, "Y");
        Bind(sample, "SAMPLE");
        Bind(sampleCount, "SAMPLE_COUNT");
    }

    void Tick() override
    {
        x(0) = static_cast<float>(GetTick());
        y(0) = static_cast<float>(2 * GetTick());

        if(GetTick() < 5)
            sample(0) = std::numeric_limits<float>::quiet_NaN();
        else if(GetTick() < 8)
            sample(0) = 1.0f;
        else if(GetTick() % 2 == 0)
            sample(0) = std::numeric_limits<float>::infinity();
        else
            sample(0) = -std::numeric_limits<float>::infinity();

        if(verified || GetTick() < 14)
            return;
        if(sampleCount.size() != 1 || sampleCount(0) != 3.0f)
            throw exception(
                "RegressionMaskTestModule: non-finite mask sampled data");

        verified = true;
        std::cout << "REGRESSION MASK TEST OK" << std::endl;
    }
};


class SoftmaxNonfiniteSourceModule : public Module
{
    matrix output;

    void Init() override
    {
        Bind(output, "OUTPUT");
    }

    void Tick() override
    {
        if(GetTick() < 3)
        {
            output(0) = std::numeric_limits<float>::infinity();
            output(1) = 1.0f;
            output(2) = std::numeric_limits<float>::infinity();
            output(3) = -std::numeric_limits<float>::infinity();
        }
        else if(GetTick() < 6)
            output.set(-std::numeric_limits<float>::infinity());
        else if(GetTick() < 9)
        {
            output(0) = std::numeric_limits<float>::quiet_NaN();
            output(1) = 0.0f;
            output(2) = 1.0f;
            output(3) = 2.0f;
        }
        else
        {
            output(0) = 0.0f;
            output(1) = 1.0f;
            output(2) = 2.0f;
            output(3) = 3.0f;
        }
    }
};


class SoftmaxNonfiniteVerifierModule : public Module
{
    matrix input;
    bool verified = false;

    void Init() override
    {
        Bind(input, "INPUT");
    }

    void Tick() override
    {
        if(verified)
            return;
        if(input.size() != 4)
            throw exception(
                "SoftmaxNonfiniteVerifierModule: unexpected input size");

        if(GetTick() < 3)
        {
            require_close(input(0), 0.5, 0.0,
                          "positive-infinity probability");
            require_close(input(1), 0.0, 0.0,
                          "finite probability beside positive infinity");
            require_close(input(2), 0.5, 0.0,
                          "second positive-infinity probability");
            require_close(input(3), 0.0, 0.0,
                          "negative-infinity probability");
        }
        else if(GetTick() < 6)
        {
            for(int index = 0; index < input.size(); ++index)
                require_close(input(index), 0.25, 0.0,
                              "all-negative-infinity probability");
        }
        else if(GetTick() < 9)
        {
            for(int index = 0; index < input.size(); ++index)
                if(!std::isnan(input(index)))
                    throw exception(
                        "SoftmaxNonfiniteVerifierModule: NaN input did not "
                        "produce NaN output");
        }
        else
        {
            double sum = 0.0;
            for(int index = 0; index < input.size(); ++index)
            {
                if(!std::isfinite(input(index)))
                    throw exception(
                        "SoftmaxNonfiniteVerifierModule: Softmax did not "
                        "recover after NaN input");
                sum += input(index);
            }
            require_close(
                sum, 1.0, 1.0e-6,
                "recovered Softmax probabilities");
            verified = true;
            std::cout << "SOFTMAX NONFINITE TEST OK" << std::endl;
        }
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
INSTALL_CLASS(RandomSequenceTestModule)
INSTALL_CLASS(RotationUnitTestModule)
INSTALL_CLASS(UniformBoundsRuntimeTestModule)
INSTALL_CLASS(SoftmaxInputTestModule)
INSTALL_CLASS(NormalizeZeroTestModule)
INSTALL_CLASS(NormalizeFiniteTestModule)
INSTALL_CLASS(RegressionCapacityTestModule)
INSTALL_CLASS(RegressionModelComparisonTestModule)
INSTALL_CLASS(RegressionMaskTestModule)
INSTALL_CLASS(SoftmaxNonfiniteSourceModule)
INSTALL_CLASS(SoftmaxNonfiniteVerifierModule)
INSTALL_CLASS(MathsBenchmarkModule)
