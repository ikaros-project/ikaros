//
//	CSOM_L.h    This file is a part of the IKAROS project
//				A straight forward but inefficient convolution SOM implementation
//
//    Copyright (C) 2007-2010 Christian Balkenius
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

//
//  Concolution-SOM with repeated lateral connections
//

#include "IKAROS.h"

#ifndef _CSOM_L
#define _CSOM_L

using namespace ikaros;

class CSOM_L: public Module 
{
public:

    static Module * Create(Parameter * p) {return new CSOM_L(p);}

    CSOM_L(Parameter * p);
    virtual ~CSOM_L();
    
    void        SetSizes();
    void        Init();
    
    void 		CalculateActivation();
    void 		CalculateLearningActivity();
    void        UpdateConstants();
    void 		UpdateWeights();
    
    void 		GenerateOutput();
    void 		CalculateSalience();
    void        GenerateColorCodedOutput();
    void 		GenerateWeightOutput();

    void 		Tick();

    float **	input;
    float **	output;             // merged output map
    float **	weights;            // merged weight output

    float **	output_red;         // color coded output for visualization
    float **	output_green;       // color coded output for visualization
    float **	output_blue;        // color coded output for visualization
    
    float *     error;
    float *     progress;

    float ****  activity;           // 4D activity map
    float ****  learning_activity;  // 4D activity map
    float ****  w;                  // 4D filter map
    float **    arbor;              // arbor function

    float ****  activity_a;         // 4D associative activity map
    float ****  w_a;                // 4D associative weights (repeated structure)
    
	float **	salience;			// Summed activity maps

    // statistics
    
    float **    stat_distribution;  // number of activation of each code/category
    float **    stat_histogram;     // number of activation of each code/category
    
    int         rf_size_x;
    int         rf_size_y;

    int         rf_inc_x;
    int         rf_inc_y;

    int         som_size_x;
    int         som_size_y;
    
    int         map_size_x;
    int         map_size_y;
    
    int         output_size_x;      // size of the merged output map
    int         output_size_y;

    int         input_size_x;       // size of the input matrix
    int         input_size_y;

    int         assoc_radius_x;     // number of "units" that the associations reaches in each direction
    int         assoc_radius_y;
 
    int         assoc_size_x;       // size of the association matrix (2*radius+1)
    int         assoc_size_y;

    int         output_type;    
    int         topology;
    float       radius;
    
    float		alpha;              // RF learning constant
    float		alpha_min;
    float		alpha_max;
    float		alpha_decay;
                
    float		sigma;              // RF sigma
    float		sigma_min;
    float		sigma_max;
    float		sigma_decay;
	
	bool		use_arbor;
	
	const char * read_file;
	const char * write_file;
};

#endif
