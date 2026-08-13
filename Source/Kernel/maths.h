// math.h - scalar math operations for iteration (c) Christian Balkenius 2024

#pragma once

#include <cmath>
#include <random>

namespace ikaros
{
    enum class angle_unit
    {
        degrees,
        radians,
        turns,
    };

    [[nodiscard]] double sgn(double x) noexcept;

    [[nodiscard]] double clip(double x, double low, double high);

    [[nodiscard]] double angle_to_angle(
        double angle, angle_unit from_angle_unit, angle_unit to_angle_unit);
    [[nodiscard]] double short_angle(double a1, double a2) noexcept;

    template<std::uniform_random_bit_generator RandomGenerator>
    // Keep the distribution paired with this generator. Reset it before using
    // it with a different independent generator.
    [[nodiscard]] float sample_normal_distribution(
        RandomGenerator & generator,
        std::normal_distribution<float> & distribution,
        float mean, float stddev)
    {
        if(!std::isfinite(mean))
            throw std::invalid_argument("Normal-distribution mean must be finite.");
        if(!std::isfinite(stddev) || stddev < 0.0f)
            throw std::invalid_argument("Normal-distribution standard deviation must be finite and non-negative.");
        if(stddev == 0.0f)
            return mean;

        return distribution(
            generator,
            std::normal_distribution<float>::param_type(mean, stddev));
    }

    template<std::uniform_random_bit_generator RandomGenerator>
    [[nodiscard]] float sample_normal_distribution(
        RandomGenerator & generator, float mean, float stddev)
    {
        std::normal_distribution<float> distribution;
        return sample_normal_distribution(
            generator, distribution, mean, stddev);
    }

    [[nodiscard]] float sample_normal_distribution(float mean, float stddev);

    [[nodiscard]] double exgaussian(
        double x, double K, double mu, double sigma, double A = 1.0);
};
