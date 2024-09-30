// math.h - scalar math operations for iteration (c) Christian Balkenius 2024

#ifndef MATHS
#define MATHS

#include <cmath>

namespace ikaros
{
	const double pi = 3.14159265358979323846;  // Awaiting C++20

	double sgn(double x);

	double min(double x, double y);
	double max(double x, double y);
	double clip(double x, double low, double high);

	double angle_to_angle(double angle, int from_angle_unit, int to_angle_unit);
   	double short_angle(double a1, double a2); // in radians
};



#endif

