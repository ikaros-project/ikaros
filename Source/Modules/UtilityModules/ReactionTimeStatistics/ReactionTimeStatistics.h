//
//		ReactionTimeStatistics.h		This file is a part of the IKAROS project
//
//
//    Copyright (C) 2020 Christian Balkenius
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

#ifndef ReactionTimeStatistics_
#define ReactionTimeStatistics_

#include "IKAROS.h"

class ReactionTimeStatistics: public Module
{
public:
	static Module * Create(Parameter * p) { return new ReactionTimeStatistics(p); }
	
    int         bins;
    float       max_rt;

    int         size;
    int         start_size;
    int         rt_size_x;
    int         rt_size_y;

    int         reaction_time;
    int         startbalance;
   
    float *		start;
    float *		stop;
    float **    rt_histogram;
    float *     rt_outlier;
    float *     rt_mean;
    float *     rt_sum;
    float *     choice_count;
    float *     choice_probability;
    float *     startbalance_out;

				ReactionTimeStatistics(Parameter * p) : Module(p) {};
    virtual		~ReactionTimeStatistics() {};

    void		Init();
    void		Tick();

    void        Command(std::string s, float x, float y, std::string value);
};

#endif
