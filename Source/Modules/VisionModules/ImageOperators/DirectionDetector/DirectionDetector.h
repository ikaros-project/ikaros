//
//	DirectionDetector.h		This file is a part of the IKAROS project
//						A module to find changes in an image

#ifndef DirectionDetector_
#define DirectionDetector_

#include "IKAROS.h"

class DirectionDetector: public Module
{
public:

    DirectionDetector(Parameter * p) :  Module(p) {}
    virtual ~DirectionDetector() {}

    static Module * Create(Parameter * p) { return new DirectionDetector(p); }

    void 	Init();
    void 	Tick();

    int		size_x;
    int		size_y;
    int		no_of_directions;

    float **	input;
    float *		no_of_rows;
    
    float *		motion_vector;
	float *		motion_direction;
    float *		looming;
    float *		left_field_motion;
    float *		right_field_motion;
    
    float *		motion_vector_draw;

    float 		filtered_dx;
    float 		filtered_dy;
    float 		filtered_central_dx;
    float 		filtered_central_dy;
};


#endif
