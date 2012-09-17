//
//    SaliencyMap.h		This file is a part of the IKAROS project
//					A generic attention map
//
//    Copyright (C) 2003-2006  Christian Balkenius
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#ifndef SALIENCYMAP
#define SALIENCYMAP

#include "IKAROS.h"

class DelayLine;

class SaliencyMap: public Module
{
public:
    SaliencyMap(Parameter * p);
    virtual ~SaliencyMap();

    static Module * Create(Parameter * p);

    void SetSizes();
    void Init();

    void Tick();

    int			no_of_inputs;	// number of inputs set in xml file

    int			input_size_x;    // input size
    int			input_size_y;

    int			output_size_x;	// input size
    int			output_size_y;

    float *		spatial_bias;

    float ***		input;
    float **		input_sum;

    float *		gain;
    float *		initial_gain;

    float *		beta;

    DelayLine **	input_store;

    float **		salience;
    float *		focus;            // 2 x 1
    float *		estimation;
    DelayLine *	estimation_store;

    float *		reinforcement;

    int			integration_radius;		// smoothing
    int			type;					// max = 0, boltzmann = 1, 2 = exponential
    int			reinforcement_delay;

    float			temperature;
    float			alpha;
};


#endif
