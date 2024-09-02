// math.h - scalar math operations for iteration (c) Christian Balkenius 2024

#ifndef MATHS
#define MATHS

#include <cmath>

namespace ikaros
{

	double sgn(double x);

	double min(double x, double y);
	double max(double x, double y);
	double clip(double x, double low, double high);
};

#endif

