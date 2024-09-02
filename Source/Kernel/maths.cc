// math.cc - scalar math operations for iteration (c) Christian Balkenius 2024

#include "math.h"

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
    

};

