//
//	GaborFilter.h			This file is a part of the IKAROS project
//

#ifndef GABORFILTER
#define GABORFILTER

#include "IKAROS.h"

class GaborFilter: public Module
{
public:

    GaborFilter(Parameter * p);
    virtual ~GaborFilter();

    static Module * Create(Parameter * p);

    void	SetSizes();
    void Init();
    void Tick();

    int		inputsize_x;
    int		inputsize_y;

    int		outputsize_x;
    int		outputsize_y;

    float		lambda;	// wavelength
    float		theta;	// orientation
    float		phi;		// phase offset
    float		gamma;	// aspect ratio
    float		sigma;	// width

    float		scale;
    int		filtersize;
    int		filterradius;

    float **	input;
    float **	filter;			// Filter kernel
    float **	gaussian;
    float **	grating;
    float **	output;
};

#endif
