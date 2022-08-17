//
//		ValueAccumulator.h		This file is a part of the IKAROS project
//
//
//    Copyright (C) 2015 Christian Balkenius
///
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

#ifndef ValueAccumulator_
#define ValueAccumulator_

#include "IKAROS.h"

class ValueAccumulator: public Module
{
public:
	static Module * Create(Parameter * p) { return new ValueAccumulator(p); }
	
    float       alpha;
    float       beta;
    float       gamma;
    float       delta;
    float       lambda;
    float       mean;
    float       sigma;
    int         input_size;
    int         size;
    int         rt_size_x;
    int         rt_size_y;

    int         reaction_time;
   
    float *		input;
    float *		index;
    float *     state;
    float *     output;
    float *     choice;
    float **    rt_histogram;
    float *     rt_mean;
    float *     rt_sum;
    float *     choice_count;
    float *     choice_probability;
    float *     choice_ix;

				ValueAccumulator(Parameter * p) : Module(p) {};
    virtual		~ValueAccumulator() {};

    void		Init();
    void		Tick();

    void        Command(std::string s, float x, float y, std::string value);
};

#endif
