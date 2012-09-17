//
//	PrewittEdgeDetector.h	This file is a part of the IKAROS project
//						A module to find edges in an image

#ifndef PREWITTEDGEDETECTOR
#define PREWITTEDGEDETECTOR

#include "IKAROS.h"

class PrewittEdgeDetector: public Module
{
public:

    PrewittEdgeDetector(Parameter * p);
    virtual ~PrewittEdgeDetector();

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
