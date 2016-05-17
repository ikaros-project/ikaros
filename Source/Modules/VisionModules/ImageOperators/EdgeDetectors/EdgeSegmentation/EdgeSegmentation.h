//
//	EdgeSegmentation.h         This file is a part of the IKAROS project
//

#ifndef EdgeSegmentation_
#define EdgeSegmentation_

#include "IKAROS.h"

class EdgeSegmentation: public Module
{
public:

    EdgeSegmentation(Parameter * p): Module(p) {}
    virtual ~EdgeSegmentation() {}

    static Module * Create(Parameter * p) { return new EdgeSegmentation(p); }

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

