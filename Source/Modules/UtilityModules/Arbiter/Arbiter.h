//
//		Arbiter.h		This file is a part of the IKAROS project
//
//    Copyright (C) 2006-2016 Christian Balkenius
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

// input: 2 x (vector weight)
// output: (vector with max weight (output) and the max veight (value))
// allows cascading

#ifndef ARBITER
#define ARBITER

#include "IKAROS.h"

class Arbiter: public Module
{
public:
    static Module * Create(Parameter * p) { return new Arbiter(p); }

    char **     input_name;
    char **     value_name;

    float **    input;
    float **    value_in;

    float *     output;
    float *     value_out;

    float *     amplitudes;
    float *     arbitration_state;
    float *     smoothed;
    float *     normalized;
    
    int         no_of_inputs;
    int         size;
    
    int         switch_time;
    int         switch_counter;
    int         from_channel;
    int         current_channel;

    int         metric;
    int         arbitration_method;
    float       softmax_exponent;
    float       hysteresis_threshold;

    Arbiter(Parameter * p);
    virtual	~Arbiter();

    void        SetSizes();

    void        CalculateAmplitues();
    void        Arbitrate();
    void        Smooth();

    void		Init();
    void		Tick();
};

#endif
