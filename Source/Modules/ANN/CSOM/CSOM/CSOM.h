//
//	CSOM.h    This file is a part of the IKAROS project
//            Base class for convolution SOM of different types
//
//    Copyright (C) 2007-2011 Christian Balkenius
//
//    This program is free software; you can redistribute it and/or modify
//    the Free Software Foundation; either version 2 of the License, or
//    it under the terms of the GNU General Public License as published by
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
#include <string>
#include <vector>
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
    virtual void        UpdateLearningBuffer();
    virtual void 		CalculateBackwardActivation();
    virtual void 		UpdateWeights() {}
    
    void                GenerateOutput();
    void                GenerateColorCodedOutput();
    void                GenerateWeightOutput();

    virtual void 		Tick();
    virtual void        Serialize(const char *fname);
    virtual void        Deserialize(const char *fname);

    virtual void        CalcError();
    virtual void        CalcProgress();

    virtual void Regenerate(float **regeneration, float **input, int size_x, int size_y, int map_x, int map_y);

    int                 CalcMapSize(int inpsz, int rfsz, int blksz, int spansz, int inc);
    
    float ***	input;
    float ***	output;             // merged output map

    float ***	top_down;           // list of 2D top-down input
    float ***    reconstruction;     // list of 2D reconstructed input based on output
    float ***    local_reconstruction;// reconstruct local output to calc error
    
    float **	weights;            // merged weight output - common for all io

    float ***	output_red;         // list of color coded output for visualization
    float ***	output_green;       // list of color coded output for visualization
    float ***	output_blue;        // list of color coded output for visualization

    //float **    activity_scratch;   // scratch activity with span
    float **    input_scratch;      // scratch input with borders
    float **    top_down_scratch;   // scratch top down with borders
    std::vector<float ****> activity;           // 5D activity map - list of 2d lattice of 2d maps
    float ****  w;                  // 4D filter map - actual weights of map
    float ****  dw;                 // 4D filter map change
    float **    arbor;              // arbor function
    float **    backward_gain;      // multiplier for each "pixel" of reconstructed input

    float ***    buffer;             // convolution buffer
    float ***    learning_buffer;    // subset of convolution buffer
    float **     topdown_buffer;    // for reconstruction

    float *     error;
    float       prev_error;
    float *     progress;
    
    int         rf_size_x;
    int         rf_size_y;

    int         rf_inc_x;
    int         rf_inc_y;

    int         upstreams;
    int         downstreams;

    int         border_mult;
    int         map_size_x_scr;
    int         map_size_y_scr;

    int         som_size_x;
    int         som_size_y;
    
    int         map_size_x;
    int         map_size_y;

    int         buffer_size_x;
    int         buffer_size_y;

    int         learning_buffer_size;

    int         block_size_x;       // size of sub-block of rf
    int         block_size_y;

    int         span_size_x;       // spacing between sub-blocks of rf 
    int         span_size_y;
    
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

    bool        serialize;
    bool        deserialize;
    bool        serialize_only_weights;
    bool        deserialize_only_weights;
	 bool			update_weights;
    std::string serialization_filename; //TODO
#ifdef USE_CUBLAS
	 float *d_buffer;
	 float *d_weight;
	 float *d_activity;
	 cublasHandle_t handle;
	 cudaStream_t stream;
	 int device_id;
#endif

};

#endif
