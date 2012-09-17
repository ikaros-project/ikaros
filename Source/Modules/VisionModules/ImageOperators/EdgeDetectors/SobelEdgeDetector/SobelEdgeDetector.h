//
//	SobelEdgeDetector.h         This file is a part of the IKAROS project
//                                                        A module to find edges in an image

#ifndef SOBELEDGEDETECTOR
#define SOBELEDGEDETECTOR

#include "IKAROS.h"

class SobelEdgeDetector: public Module
{
public:

    SobelEdgeDetector(Parameter * p);
    virtual ~SobelEdgeDetector();

    static Module * Create(Parameter * p);

    void	SetSizes();
    void    Init();

    void        CalculateSqrt();
    void        CalculateAbs();
    void        CalculateDx();
    void        CalculateDy();
    void        CalculateAbsDx();
    void        CalculateAbsDy();

    void        Tick();

    int			inputsize_x;
    int			inputsize_y;

    int			outputsize_x;
    int			outputsize_y;

    float **	input;
    float **	output;

    float **	dx_temp;
    float **	dy_temp;

    int			type;

    float **	dx_kernel;
    float **	dy_kernel;
};


#endif
