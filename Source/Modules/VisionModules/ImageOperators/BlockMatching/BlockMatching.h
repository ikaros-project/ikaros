//
//	BlockMatching.h		This file is a part of the IKAROS project
//						A module to find changes in an image

#ifndef BlockMatching_
#define BlockMatching_

#include "IKAROS.h"

class BlockMatching: public Module
{
public:

    BlockMatching(Parameter * p) :  Module(p) {}
    virtual ~BlockMatching() {}

    static Module * Create(Parameter * p) { return new BlockMatching(p); }

    void 	Init();
	void	MatchBlock(int i);
    void 	Tick();

    int		size_x;
    int		size_y;
	int		max_points;
    
	int		block_radius;
    int		search_window_radius;
    int		search_method;
    int		metric;

    float **	input;
    float **	input_last;

	float **	points;
    float *		no_of_points;
    
    float **	flow;
    float *		flow_size;
    
    float **	debug;
};


#endif
