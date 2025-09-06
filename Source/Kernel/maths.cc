// math.cc - scalar math operations for iteration (c) Christian Balkenius 2024

#include "maths.h"

#include <random>

namespace ikaros
{

	double sgn(double x)
	{
		if(x>0)
			return 1;
		else if(x<0)
			return -1;
		else	return 0;
	}

	double min(double x, double y)
	{
		if(x<y)
			return x;
		else
			return y;
	}

	double max(double x, double y)
	{
		if(x>y)
			return x;
		else
			return y;
	}


	double clip(double x, double low, double high)
	{
		if (x < low)
			return low;
		else if (x > high)
			return high;
		else
			return x;
	}
    

    double angle_to_angle(double angle, int from_angle_unit, int to_angle_unit)
    {
        switch(from_angle_unit)
        {
            default:
            case 0: angle = angle; break;
            case 1: angle = (angle/(2.0*pi))*360; break;
            case 2: angle = angle*360; break;
        }
        switch(to_angle_unit)
        {
            default:
            case 0: angle = angle;break;
            case 1: angle = angle/360*(2.0*pi);break;
            case 2: angle = angle/360;break;
        }
        return angle;
    }


    double
    short_angle(double a1, double a2)
    {
        return atan2(sin(a2-a1), cos(a2-a1));
    }



	float sample_normal_distribution(float mean, float stddev)
	{
		thread_local std::mt19937 gen(std::random_device{}());
		std::normal_distribution<float> dist(mean, stddev);
		return dist(gen);
	}


	// Ex-Gaussian PDF (exponnorm parameterization)
	// K = tau / sigma, mu = mean of Gaussian, sigma = std of Gaussian
	// Optional A scales the output (default = 1.0 for a proper PDF).
	double
	exgaussian(double x, double K, double mu, double sigma, double A) 
	{
		if (sigma <= 0.0 || K <= 0.0)
			throw std::invalid_argument("sigma and K must be > 0");

		const double z = (x - mu) / sigma;
		const double invK = 1.0 / K;

		// Standard normal CDF Φ(z) = 0.5 * erfc(-z / sqrt(2))
		auto Phi = [](double val) 
		{
			return 0.5 * std::erfc(-val / std::sqrt(2.0));
		};

		// f(x) = (1/(Kσ)) * exp( 1/(2K²) - (x-μ)/(Kσ) ) * Φ( (x-μ)/σ - 1/K )
		const double expo = std::exp(0.5 * invK * invK - z * invK);
		const double phi_arg = z - invK;
		return A * (expo / (K * sigma)) * Phi(phi_arg);
	}
};

