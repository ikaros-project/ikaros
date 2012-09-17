//
//	HysteresisThresholding.h		This file is a part of the IKAROS project
//							A module to threshold edges in an image

#ifndef HYSTERESISTHRESHOLDING
#define HYSTERESISTHRESHOLDING

#include "IKAROS.h"

class HysteresisThresholding: public Module
{
public:

    HysteresisThresholding(Parameter * p);
    virtual ~HysteresisThresholding();

    static Module * Create(Parameter * p);

    void	SetSizes();
    void	Init();
    void	Tick();

    int		inputsize_x;
    int		inputsize_y;

    int		outputsize_x;
    int		outputsize_y;

    float	**	input;
    float	**	output;

    int		iterations;
    int		range;
    float	T1;
    float	T2;
};


#endif
