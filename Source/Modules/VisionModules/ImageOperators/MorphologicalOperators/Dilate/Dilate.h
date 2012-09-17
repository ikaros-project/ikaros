//
//	Dilate.h			This file is a part of the IKAROS project
//					A module to find edges in an image

#ifndef DILATE
#define DILATE

#include "IKAROS.h"

class Dilate: public Module
{
public:

    Dilate(Parameter * p);
    virtual ~Dilate();

    static Module * Create(Parameter * p);

    void	SetSizes();
    void	Init();
    void	Tick();

    int		inputsize_x;
    int		inputsize_y;

    int		outputsize_x;
    int		outputsize_y;

    float **	input;
    float **	output;
};


#endif
