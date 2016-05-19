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

    int         max_edges;
    int         grid;
    float       threshold;
    bool        normalize;

    int			size_x;
    int			size_y;

    float **	input;
    float **	dx;
    float **	dy;
    float **	output;

    float **	edge_list;
    float **	edge_elements;
    float *     edge_list_size;
};


#endif

