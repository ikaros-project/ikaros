//
//	CannyEdgeDetector.h		This file is a part of the IKAROS project
//							A module to find edges in an image

#ifndef CANNYEDGEDETECTOR
#define CANNYEDGEDETECTOR

#include "IKAROS.h"

class CannyEdgeDetector: public Module
{
public:

    CannyEdgeDetector(Parameter * p);
    virtual ~CannyEdgeDetector();

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
    float	**	edges;
    float	**	maxima;
    float	**	output;

    float		T0;
    float		T1;
    float		T2;
};

#endif
