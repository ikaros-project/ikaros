// math.h - scalar math operations for iteration (c) Christian Balkenius 2024

#pragma once

#include <cmath>
#include <random>
#include <stdexcept>

namespace ikaros
{
	enum angle_unit {degrees, radians, tau};

	inline constexpr double pi = 3.14159265358979323846;
	
	double sgn(double x);

	double clip(double x, double low, double high);

	double angle_to_angle(double angle, angle_unit from_angle_unit, angle_unit to_angle_unit);
   	double short_angle(double a1, double a2); // in radians

    template<typename RandomGenerator>
    float sample_normal_distribution(
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

    template<typename RandomGenerator>
    float sample_normal_distribution(RandomGenerator & generator,
                                     float mean, float stddev)
    {
        std::normal_distribution<float> distribution;
        return sample_normal_distribution(
            generator, distribution, mean, stddev);
    }

	float sample_normal_distribution(float mean, float stddev);

	double exgaussian(double x, double K, double mu, double sigma, double A = 1.0); // Ex-Gaussian PDF (exponnorm parameterization) / K = tau / sigma, mu = mean of Gaussian, sigma = std of Gaussian
};
