// math.cc - scalar math operations for iteration (c) Christian Balkenius 2024

#include "maths.h"

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


};

