//
//	GaussianEdgeDetector.h		This file is a part of the IKAROS project
//							A module to find edges in an image

#ifndef GAUSSIANEDGEDETECTOR
#define GAUSSIANEDGEDETECTOR

#include "IKAROS.h"

class GaussianEdgeDetector: public Module
{
public:

    GaussianEdgeDetector(Parameter * p);
    virtual ~GaussianEdgeDetector();

    static Module * Create(Parameter * p);

    void	SetSizes();
    void Init();
    void Tick();

    int		inputsize_x;
    int		inputsize_y;

    int		outputsize_x;
    int		outputsize_y;

    float		scale;
    int		filtersize;
    int		filterradius;

    float **	input;

    float	**	dGx;			// Derivate gaussian filters
    float	**	dGy;
    float	**	dx;
    float	**	dy;
    float	**	orientation;
    float	**	output;
    float	**	maxima;
    float	**	nodes;
};


#endif
