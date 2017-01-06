//
//	CSOM.h    This file is a part of the IKAROS project
//            Base class for convolution SOM of different types
//
//    Copyright (C) 2007-2011 Christian Balkenius
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

#include "IKAROS.h"

#ifndef _CSOM
#define _CSOM

using namespace ikaros;

class CSOM: public Module 
{
public:

    static Module * Create(Parameter * p) {return new CSOM(p);}

    CSOM(Parameter * p);
    virtual ~CSOM();
    
    virtual void        SetSizes();
    virtual void        Init();
    
    virtual void 		CalculateForwardActivation();
    virtual void 		CalculateBackwardActivation();
    virtual void 		UpdateWeights() {}
    
    void                GenerateOutput();
    void                GenerateColorCodedOutput();
    void                GenerateWeightOutput();

    virtual void 		Tick();

    float **	input;
    float **	output;             // merged output map

    float **	top_down;           // top-down input
    float **    reconstruction;     // reconstructed input based on output
    
    
    float **	weights;            // merged weight output

    float **	output_red;         // color coded output for visualization
    float **	output_green;       // color coded output for visualization
    float **	output_blue;        // color coded output for visualization


    float ****  activity;           // 4D activity map
    float ****  w;                  // 4D filter map
    float ****  dw;                 // 4D filter map change
    float **    arbor;              // arbor function
    float **    backward_gain;      // multiplier for each "pixel" of reconstructed input

    float **    buffer;             // convolution buffer
    
    int         rf_size_x;
    int         rf_size_y;

    int         rf_inc_x;
    int         rf_inc_y;

    int         som_size_x;
    int         som_size_y;
    
    int         map_size_x;
    int         map_size_y;
    
    int         buffer_size_x;
    int         buffer_size_y;

    int         output_size_x;      // size of the merged output map
    int         output_size_y;

    int         input_size_x;       // size of the input matrix
    int         input_size_y;

    int         output_type;    
    
    float		alpha;              // RF learning constant
    float		alpha_min;
    float		alpha_max;
    float		alpha_decay;
	
	bool		use_arbor;
};

#endif
