//
//	Erode.h			This file is a part of the IKAROS project
//					A module to erode an image

#ifndef ERODE
#define ERODE

#include "IKAROS.h"

class Erode: public Module
{
public:

    Erode(Parameter * p);
    virtual ~Erode();

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
