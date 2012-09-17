//
//	RobertsEdgeDetector.h		This file is a part of the IKAROS project
//							A module to find edges in an image

#ifndef ROBERTEDGEDETECTOR
#define ROBERTEDGEDETECTOR

#include "IKAROS.h"

class RobertsEdgeDetector: public Module
{
public:

    RobertsEdgeDetector(Parameter * p);
    virtual ~RobertsEdgeDetector();

    static Module * Create(Parameter * p);

    void	SetSizes();
    void Init();
    void Tick();

    int		inputsize_x;
    int		inputsize_y;

    int		outputsize_x;
    int		outputsize_y;

    float **	input;
    float	**	output;
};


#endif
