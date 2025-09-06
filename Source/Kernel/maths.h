// math.h - scalar math operations for iteration (c) Christian Balkenius 2024

#pragma once

#include <cmath>

namespace ikaros
{
	enum angle_unit {degrees, radians, tau};

	const double pi = 3.14159265358979323846;  // Awaiting C++20
	
	double sgn(double x);

	double min(double x, double y);
	double max(double x, double y);
	double clip(double x, double low, double high);

	double angle_to_angle(double angle, int from_angle_unit, int to_angle_unit);
   	double short_angle(double a1, double a2); // in radians

	float sample_normal_distribution(float mean, float stddev);

	double exgaussian(double x, double K, double mu, double sigma, double A = 1.0); // Ex-Gaussian PDF (exponnorm parameterization) / K = tau / sigma, mu = mean of Gaussian, sigma = std of Gaussian
};


