//
//	  LinearAssociator.h		This file is a part of the IKAROS project
//								The module learns a linear map
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

#ifndef LINEARASSOCIATOR
#define LINEARASSOCIATOR

#include "IKAROS.h"

class LinearAssociator: public Module
{
public:
	float		alpha;		// Learning rate
	float		beta;			// Momentum

	float *     input;			
	float *     t_input;			
	int         input_size;

	float *     output;
	float *     t_output;
	int         output_size;
	
	int         memory_max;
	int         memory_used;
	int         memory_training;
	float **	memory_t_input;
	float **	memory_t_output;

	float *     learning_rate;
	
	float *     delta;		// Prediction error
	float *     momentum;	// Momentum vector
	float *     error;
	float *     confidence;

	float **	m;			// The linear mapping

	LinearAssociator(Parameter * p) : Module(p) {}
	virtual ~LinearAssociator();

	static	Module* Create(Parameter * p) { return new LinearAssociator(p); }

	void		Init();
	
	void		Train(float * input, float * output);
	void		Tick();
};

#endif

