//
//	ChangeDetector.h		This file is a part of the IKAROS project
//						A module to find changes in an image

#ifndef CHANGEDETECTOR
#define CHANGEDETECTOR

#include "IKAROS.h"

class ChangeDetector: public Module
{
public:

    ChangeDetector(Parameter * p);
    virtual ~ChangeDetector();

    static Module * Create(Parameter * p);

    void	SetSizes();
    void Init();
    void Tick();

    int		inputsize_x;
    int		inputsize_y;

    int		outputsize_x;
    int		outputsize_y;

    int		border;	// part of the image to ignore

    float **	last_input;

    float **	input;
    float	**	output;
};


#endif
