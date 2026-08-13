// math.cc - scalar math operations for iteration (c) Christian Balkenius 2024

#include "maths.h"

#include <limits>
#include <numbers>
#include <random>
#include <stdexcept>

namespace ikaros
{

	double sgn(double x) noexcept
	{
		if(std::isnan(x))
			return x;
		if(x>0)
			return 1;
		else if(x<0)
			return -1;
		else	return 0;
	}

	double clip(double x, double low, double high)
	{
		if(std::isnan(low) || std::isnan(high) || low > high)
			throw std::invalid_argument("clip() requires ordered, non-NaN bounds.");
		if (x < low)
			return low;
		else if (x > high)
			return high;
		else
			return x;
	}
    

    double
    angle_to_angle(double angle, angle_unit from_angle_unit,
                   angle_unit to_angle_unit)
    {
        auto radians_per_unit = [](angle_unit unit)
        {
            switch(unit)
            {
                case angle_unit::degrees:
                    return std::numbers::pi_v<double> / 180.0;
                case angle_unit::radians:
                    return 1.0;
                case angle_unit::turns:
                    return 2.0 * std::numbers::pi_v<double>;
                default:
                    throw std::invalid_argument("Unknown angle unit.");
            }
        };

        const double source_scale = radians_per_unit(from_angle_unit);
        const double target_scale = radians_per_unit(to_angle_unit);
        if(from_angle_unit == to_angle_unit)
            return angle;
        return angle * (source_scale / target_scale);
    }


    double
    short_angle(double a1, double a2) noexcept
    {
        constexpr double full_turn =
            2.0 * std::numbers::pi_v<double>;
        if(!std::isfinite(a1) || !std::isfinite(a2))
            return std::numeric_limits<double>::quiet_NaN();

        const double difference = a2 - a1;
        if(!std::isfinite(difference))
            return std::remainder(
                std::remainder(a2, full_turn) -
                    std::remainder(a1, full_turn),
                full_turn);

        constexpr double accurate_arithmetic_limit =
            full_turn / std::numeric_limits<double>::epsilon();
        if(std::fabs(difference) >= accurate_arithmetic_limit)
            return std::remainder(difference, full_turn);

        return difference -
               full_turn * std::floor(
                   (difference + std::numbers::pi_v<double>) /
                   full_turn);
    }



	float sample_normal_distribution(float mean, float stddev)
	{
		thread_local std::mt19937 gen(std::random_device{}());
		thread_local std::normal_distribution<float> distribution;
		return sample_normal_distribution(
			gen, distribution, mean, stddev);
	}


    // Ex-Gaussian PDF (exponnorm parameterization)
    // K = tau / sigma, mu = mean of Gaussian, sigma = std of Gaussian
    // Optional A scales the output (default = 1.0 for a proper PDF).
    double
    exgaussian(double x, double K, double mu, double sigma, double A)
    {
        if(!std::isfinite(x) || !std::isfinite(mu))
            throw std::invalid_argument("Ex-Gaussian x and mu must be finite.");
        if(!std::isfinite(sigma) || sigma <= 0.0 ||
           !std::isfinite(K) || K <= 0.0)
            throw std::invalid_argument("Ex-Gaussian sigma and K must be finite and positive.");
        if(!std::isfinite(A))
            throw std::invalid_argument("Ex-Gaussian amplitude must be finite.");
        if(A == 0.0)
            return A;

        constexpr double sqrt_two = 1.41421356237309504880;
        constexpr double half_log_two_pi = 0.91893853320467274178;
        const double z = (x - mu) / sigma;
        const double left_tail_scale = std::fma(-K, z, 1.0);
        double log_density = 0.0;

        // When t = z - 1/K is far into the left tail, evaluating
        // exp(...) and Phi(t) separately produces infinity times zero.
        // The normal-CDF asymptotic series cancels those terms algebraically.
        if(left_tail_scale >= 10.0 * K)
        {
            const double ratio = K / left_tail_scale;
            const double inverse_t_squared = ratio * ratio;
            double correction = 1.0;
            double term = 1.0;
            double previous_term = std::numeric_limits<double>::infinity();
            for(int order = 1; order <= 32; ++order)
            {
                term *= -(2.0 * order - 1.0) * inverse_t_squared;
                const double term_size = std::fabs(term);
                if(term_size >= previous_term)
                    break;
                correction += term;
                if(term_size <= std::numeric_limits<double>::epsilon() *
                                std::fabs(correction))
                    break;
                previous_term = term_size;
            }

            log_density =
                -0.5 * z * z -
                half_log_two_pi -
                std::log(sigma) -
                std::log(left_tail_scale) +
                std::log(correction);
        }
        else
        {
            const double inverse_K = 1.0 / K;
            const double normal_argument = z - inverse_K;
            const double normal_cdf =
                0.5 * std::erfc(-normal_argument / sqrt_two);
            const double exponential_term =
                -0.5 * inverse_K * inverse_K -
                normal_argument * inverse_K;
            log_density =
                -std::log(K) -
                std::log(sigma) +
                exponential_term +
                std::log(normal_cdf);
        }

        return std::copysign(
            std::exp(std::log(std::fabs(A)) + log_density),
            A);
    }
};
