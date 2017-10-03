//
//    Autoassociator.h		This file is a part of the IKAROS project
//
//
//    Copyright (C) 2016 Christian Balkenius
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

#ifndef Autoassociator_
#define Autoassociator_

#include "IKAROS.h"
// #include "LearningModule.h"

class Autoassociator : public Module
{
public:
	static Module * Create(Parameter * p) { return new Autoassociator(p); }
	
				Autoassociator(Parameter * p) : Module(p) {};
    virtual		~Autoassociator() {};

	void		    Learn();
	void		    Init();
	void		    Tick();
    
    float           learning_rate_const;
    float           activation_gain_const;
    float           depression_rate;
    float           noise_level;

    float *         input;
    float *         output;
    float *         net;
    float *         reset;

    float *         t_input;
    float *         t_output;
    
    float *         delayed_t_input;
    float *         top_down_input;

    float *         top_down_error;
    float *         error_output;

    float *         learning_gain;
    float *         activation_gain;
    
    float *         aux_input;
    float *         aux_output;
    float *         aux_t_input;
    float *         aux_t_output;
    
    float **        w;    // size(input) x size(output)
    float **        u;    // size(aux_input) x size(output)
    
    float **        w_depression;
    float **        u_depression;


    int             input_size;
    int             aux_input_size;
    int             output_size;

    float *		    energy;
    
 };

#endif

