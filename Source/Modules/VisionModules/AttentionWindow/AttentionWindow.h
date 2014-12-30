//
//	AttentionWindow.h		This file is a part of the IKAROS project
//                          Module that extracts a focus of attention from an object detector
//
//    Copyright (C) 2014  Christian Balkenius
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

#ifndef AttentionWindow_
#define AttentionWindow_

#include "IKAROS.h"

class AttentionWindow: public Module
{
public:

    AttentionWindow(Parameter * p) : Module(p) {};
    virtual ~AttentionWindow() {};

    static Module * Create(Parameter * p);

    void Init();
    void Tick();

    int		input_size_x;
    int		input_size_y;

    int		output_size_x;
    int		output_size_y;

    int		window_size_x;
    int		window_size_y;
    int		window_radius;

    float **	input;
    float **	output;

    float *     top_down_position;
    float *     top_down_bounds;
    
    float **    bottom_up_position;
    float **    bottom_up_bounds;
    float *     bottom_up_count;

    bool    mask;
};


#endif

