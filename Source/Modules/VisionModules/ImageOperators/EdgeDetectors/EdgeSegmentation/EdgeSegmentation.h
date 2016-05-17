//
//	EdgeSegmentation.h         This file is a part of the IKAROS project
//                             A module to select edges in an image

#ifndef EdgeSegmentation_
#define EdgeSegmentation_

#include "IKAROS.h"

class EdgeSegmentation: public Module
{
public:

    EdgeSegmentation(Parameter * p);
    virtual ~EdgeSegmentation();

    static Module * Create(Parameter * p);

    void        Init();
    void        Tick();

    int			inputsize_x;
    int			inputsize_y;

    int			outputsize_x;
    int			outputsize_y;

    float **	input;
    float **	output;

    float **	edge_list;
    float *     edge_list_size;
};


#endif

