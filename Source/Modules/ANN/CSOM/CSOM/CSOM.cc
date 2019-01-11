//
//	CSOM.cc   This file is a part of the IKAROS project
//				Self-Organizing Convolution Network
//
//    Copyright (C) 2008-2011 Christian Balkenius
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
//    2016-09-12: update to allow multiple up and downwards paths

/*

    INCREMENT:
        always 1 gives convolution

    WEIGHT_VISUALIZATION:
        size: rf_size * som_size
    
    OUTPUT:
        size: som_size * (input_size - rf_size + 1)
        packing: multiple SOMs / multiple convoluted maps
*/


#include "CSOM.h"
#include <sstream>
#include <string>

//#include <Accelerate/Accelerate.h>
//#include <dispatch/dispatch.h>

using namespace ikaros;



CSOM::CSOM(Parameter * p):
	Module(p)
{ 
    Bind(alpha, "alpha");
    Bind(alpha_min, "alpha_min");
    Bind(alpha_max, "alpha_max");
    Bind(alpha_decay, "alpha_decay");
        
	Bind(use_arbor, "use_arbor");
    Bind(serialize, "save_state");
    Bind(deserialize, "load_state");
    Bind(update_weights, "update_weights");

    Bind(upstreams, "upstreams");
    Bind(downstreams, "downstreams");
    Bind(serialize_only_weights, "save_weights_only");
    Bind(deserialize_only_weights, "load_weights_only");
    Bind(border_mult, "border_multiplier");
#ifdef USE_CUBLAS
	Bind(device_id, "device_id");
#endif
    //Bind(filename, "filename");
    serialization_filename = GetValue("filename"); //NULL;
    //printf("%s - serialization_filename: %s\n", GetName(), serialization_filename.c_str());

    rf_size_x = GetIntValue("rf_size_x");
    rf_size_y = GetIntValue("rf_size_y");
   
    rf_inc_x = GetIntValue("rf_inc_x");
    rf_inc_y = GetIntValue("rf_inc_y");
   
    som_size_x = GetIntValue("som_size_x");
    som_size_y = GetIntValue("som_size_y");

    block_size_x = GetIntValue("block_size_x");
    block_size_y = GetIntValue("block_size_y");
    if (block_size_x==-1) block_size_x = rf_size_x;
    if (block_size_x==-1) block_size_y = rf_size_y;
    span_size_x = GetIntValue("span_size_x");
    span_size_y = GetIntValue("span_size_y");
    learning_buffer_size = GetIntValue("learning_buffer_size");
    
    for (int i = 1; i <= upstreams; ++i)
    {
        std::string ix_str = upstreams>1?std::to_string(i):"";
        std::string str = "INPUT" + ix_str;
        AddInput(str.c_str());
        //str = "TOP_DOWN" + ix_str;
        //AddInput(str.c_str());
        str = "OUTPUT" + ix_str;
        AddOutput(str.c_str());
        //str = "RECONSTRUCTION" + ix_str;
        //AddOutput(str.c_str());
        str = "OUTPUT_RED" + ix_str;
        AddOutput(str.c_str());
        str = "OUTPUT_BLUE" + ix_str;
        AddOutput(str.c_str());
        str = "OUTPUT_GREEN" + ix_str;
        AddOutput(str.c_str());
        //PrintIONames();
        // delete ss;
    }
    for (int i = 1; i <= downstreams; ++i)
    {
        std::string ix_str = downstreams>1?std::to_string(i):"";
        //std::string str = "INPUT" + ix_str;
        //AddInput(str.c_str());
        std::string str = "TOP_DOWN" + ix_str;
        AddInput(str.c_str());
        //str = "OUTPUT" + ix_str;
        //AddOutput(str.c_str());
        str = "RECONSTRUCTION" + ix_str;
        AddOutput(str.c_str());
        //str = "OUTPUT_RED" + ix_str;
        //AddOutput(str.c_str());
        //str = "OUTPUT_BLUE" + ix_str;
        //AddOutput(str.c_str());
        //str = "OUTPUT_GREEN" + ix_str;
        //AddOutput(str.c_str());
        //PrintIONames();
        // delete ss;
    }
    AddOutput("WEIGHTS", true, rf_size_x*som_size_x, rf_size_y*som_size_y);

    
    AddOutput("ERROR");
    AddOutput("PROGRESS");

    output_type = GetIntValueFromList("output_type");
}



void
CSOM::SetSizes()
{
    // Assume all inputs same as #1
    std::string ix_str = upstreams>1?std::to_string(1):"";
    std::string str = "INPUT" + ix_str;
    int sx = GetInputSizeX(str.c_str());
    int sy = GetInputSizeY(str.c_str());
    if(sx == unknown_size || sy == unknown_size)
        return;
    int map_size_x = CalcMapSize(sx, rf_size_x, block_size_x, span_size_x, rf_inc_x);
    int map_size_y = CalcMapSize(sy, rf_size_y, block_size_y, span_size_y, rf_inc_y);
	printf("%s: input=(%i, %i), rf=%i, blk=%i, span=%i, inc=%i, map=(%i, %i)\n", GetName(), sx, sy, rf_size_x, block_size_x, span_size_x, rf_inc_x, map_size_x, map_size_y);
    
    // iterate over streams, set output sizes
    for (int i = 1; i <= upstreams; ++i)
    {
        std::string ix_str = upstreams>1?std::to_string(i):"";
        std::string str = "OUTPUT" + ix_str;
        SetOutputSize(str.c_str(), som_size_x*map_size_x, som_size_y*map_size_y);
        str = "OUTPUT_RED" + ix_str;
        SetOutputSize(str.c_str(), map_size_x, map_size_y);
        str = "OUTPUT_GREEN" + ix_str;
        SetOutputSize(str.c_str(), map_size_x, map_size_y);
        str = "OUTPUT_BLUE" + ix_str;
        SetOutputSize(str.c_str(), map_size_x, map_size_y);
        
    }
    for (int i = 1; i <= downstreams; ++i)
    {
        std::string ix_str = downstreams>1?std::to_string(i):"";
        std::string str  = "RECONSTRUCTION" + ix_str;
        SetOutputSize(str.c_str(), sx, sy);
    }

    SetOutputSize("ERROR", 1);
    SetOutputSize("PROGRESS", 1);
    
    // check that all inputs are same size
    for (int i = 2; i <= upstreams; ++i)
    {
        std::stringstream ss (std::stringstream::in | std::stringstream::out);
        ss << "INPUT" << i;

        int x = GetInputSizeX(ss.str().c_str());
        int y = GetInputSizeY(ss.str().c_str());
        if(x == unknown_size || y == unknown_size)
            return;
        if(x != sx || y != sy)
            Notify(msg_fatal_error, "Input streams have different sizes, must have same size.");
        sx = x;
        sy = y;
    }      
    
}



void
CSOM::Init()
{
    std::string ix_str = upstreams>1?std::to_string(1):"";
    std::string str = "INPUT" + ix_str;
    input_size_x = GetInputSizeX(str.c_str());
    input_size_y = GetInputSizeY(str.c_str());

    str = "OUTPUT" + ix_str;
    output_size_x = GetOutputSizeX(str.c_str());
    output_size_y = GetOutputSizeY(str.c_str());

    // "map" is activity map
    //int mpsz = (indims-(rfsz+(rfsz/blck-1)*span)) /(inc) +1;

    // take block size and span into account when calculating map size
    map_size_x = CalcMapSize(input_size_x, rf_size_x, block_size_x, span_size_x, rf_inc_x);
    map_size_y = CalcMapSize(input_size_y, rf_size_y, block_size_y, span_size_y, rf_inc_y);
    // TODO calc buffer map size with inputsize+2*span
    
    map_size_x_scr = CalcMapSize(input_size_x + 2 * border_mult*span_size_x , rf_size_x, block_size_x, span_size_x, rf_inc_x);
    map_size_y_scr = CalcMapSize(input_size_y + 2 * border_mult*span_size_y, rf_size_y, block_size_y, span_size_y, rf_inc_y);
    //activity_scratch = create_matrix(map_size_x_scr, map_size_y_scr);
    input_scratch = create_matrix(input_size_x + 2 * border_mult * span_size_x, input_size_y + 2 * border_mult * span_size_y);
    top_down_scratch = create_matrix(som_size_x*map_size_x_scr, som_size_y*map_size_y_scr);
    // create arrays of ** float:
    input = new float **[upstreams];
    top_down = new float**[downstreams];
    reconstruction = new float**[downstreams];
    // backward_gain = new float**[streams];
    output = new float**[upstreams];
    output_red = new float**[upstreams];
    output_green = new float**[upstreams];
    output_blue = new float**[upstreams];
    buffer_size_x = rf_size_x * rf_size_y;
    buffer_size_y = map_size_x * map_size_y;
    buffer = create_matrix( buffer_size_x, buffer_size_y, upstreams); // These should be allocated only once in Init()
    topdown_buffer = create_matrix(som_size_y*som_size_x, buffer_size_y);
    if (learning_buffer_size == -1) learning_buffer_size = buffer_size_y;
    learning_buffer = create_matrix(upstreams, buffer_size_x, learning_buffer_size);
    // iterate over streams
    for (int i = 0; i < upstreams; ++i)
    {
		// TODO add   learning_buffer = create_matrix(buffer_size_x, learning_buffer_size);
        std::string ix_str = upstreams>1?std::to_string(i+1):"";
        std::string str = "INPUT" + ix_str;
        input[i] = GetInputMatrix(str.c_str());
        str = "OUTPUT" + ix_str;
        output[i] = GetOutputMatrix(str.c_str());
        str = "OUTPUT_RED" + ix_str;
        output_red[i] = GetOutputMatrix(str.c_str());
        str = "OUTPUT_GREEN" + ix_str;
        output_green[i] = GetOutputMatrix(str.c_str());
        str = "OUTPUT_BLUE" + ix_str;
        output_blue[i] = GetOutputMatrix(str.c_str());

        activity.push_back(create_matrix(map_size_x, map_size_y, som_size_x, som_size_y));
    }
    for (int i = 0; i < downstreams; ++i)
    {
        std::string ix_str = downstreams>1?std::to_string(i+1):"";
        //std::string str = "INPUT" + ix_str;
        std::string str = "TOP_DOWN" + ix_str;
        top_down[i] = GetInputMatrix(str.c_str(), false);
        str = "RECONSTRUCTION" + ix_str;
        reconstruction[i] = GetOutputMatrix(str.c_str());
    }

    backward_gain   = create_matrix(input_size_x, input_size_y);
    weights     = GetOutputMatrix("WEIGHTS");
    // TODO try setting streams as last ?
    float r = (rf_size_x+0.5)/2;
    float c = (rf_size_x)/2;
    
    // Quadratic arbor function
    
    arbor = create_matrix(rf_size_x, rf_size_y);
	set_matrix(arbor, 1.0, rf_size_x, rf_size_y);
    if(use_arbor)
	{
		for(int y=0; y<rf_size_y; y++)
			for(int x=0; x<rf_size_x; x++)
				arbor[y][x] = max(0.0f, 1.0f-sqr(float(hypot(x-c, y-c)))/sqr(r));
	}
    
    // Init weights to random values
	w = create_matrix(rf_size_x, rf_size_y, som_size_x, som_size_y);
	random(***w, 0.f, 0.0001f, rf_size_x*rf_size_y*som_size_x*som_size_y);
/*
    w	= new float *** [som_size_y];
    for(int j=0; j<som_size_y; j++)
    {
        w[j] = new float ** [som_size_x];
        for(int i=0; i<som_size_x; i++)
            // note that inner weight matrices are same dims 
            // as receptive fields
            w[j][i] = multiply(random(create_matrix(rf_size_x, rf_size_y), 
                0.0, 
                0.0001, 
                rf_size_x, 
                rf_size_y), 
            arbor, rf_size_x, rf_size_y);
//            w[j][i] = normalize(multiply(random(create_matrix(rf_size_x, rf_size_y), 
//            	0.0, 0.2, rf_size_x, rf_size_y), arbor, rf_size_x, rf_size_y), rf_size_x, rf_size_y);
    }
*/	
    dw	= new float *** [som_size_y];
    for(int j=0; j<som_size_y; j++)
    {
       dw[j] = new float ** [som_size_x];
        for(int i=0; i<som_size_x; i++)
            dw[j][i] = create_matrix(rf_size_x, rf_size_y);
    }
	
    // Pre-calculate backward gains
    
    for(int mj=0; mj<map_size_y; mj++)
        for(int mi=0; mi<map_size_x; mi++)
            for(int sj=0; sj<som_size_y; sj++)
                for(int si=0; si<som_size_x; si++)
                    for(int rj=0; rj<rf_size_y; rj++)
                        for(int ri=0; ri<rf_size_x; ri++)
                            backward_gain[rf_inc_y*mj+rj][rf_inc_x*mi+ri] += 1.0 * 1.0 * arbor[rj][ri]; // top_down * gain * arbor
    
    for(int i=0; i<input_size_x; i++)
        for(int j=0; j<input_size_y; j++)
            backward_gain[j][i] = (backward_gain[j][i] != 0 ? 1 / backward_gain[j][i] : 0);

    if(deserialize)
        Deserialize(serialization_filename.c_str());    
    
    local_reconstruction = create_matrix(downstreams, input_size_x, input_size_y);
    error = GetOutputArray("ERROR");
    progress = GetOutputArray("PROGRESS");
#ifdef USE_CUBLAS

	// allocate memory and create handle
	printf("allocating device memory..\n");
	unsigned long buffer_sz = buffer_size_x*buffer_size_y*sizeof(float);
	unsigned long w_sz = som_size_x*som_size_y*buffer_size_x*sizeof(float);
	unsigned long act_sz =  som_size_x*som_size_y*map_size_x*map_size_y*sizeof(float);
	unsigned long total_sz = buffer_sz + w_sz + act_sz;
	cublasStatus_t status; 
	cudaError_t error; 
	error = cudaSetDevice(device_id);
	cuda_checkerror(error, "CUDA set device error");
	status = cublasCreate(&handle);
	cuda_checkerror(status, "CUDA create handle error");
	//error = cudaStreamCreate(&stream);
	cuda_checkerror(error, "CUDA create stream error");

	error = cudaMalloc((void**)&d_buffer, buffer_sz );
	cuda_checkerror(error, "CUDA malloc error: d_buffer");
	
	error = cudaMalloc((void**)&d_weight, w_sz);
	cuda_checkerror(error, "CUDA malloc error: d_weight");
	
	error = cudaMalloc((void**)&d_activity, act_sz);
	cuda_checkerror(error, "CUDA malloc error: d_activity");

	printf("allocated for module %s: \n d_buffer: %lu\n d_weight: %lu\n d_activity: %lu\n total: %lu bytes\n",
		GetName(), buffer_sz,
		w_sz,
		act_sz,
		total_sz);
	printf("allocating device memory done\n");
#endif
}



CSOM::~CSOM()
{
    if(serialize)
        Serialize(serialization_filename.c_str());

    
    for (int i = 0; i < upstreams; ++i)
    {   
        destroy_matrix(activity.at(i));
    }
    // delete[] input; etc
    delete[] input;
    delete[] top_down;
    delete[] reconstruction;
    delete[] output;
    delete[] output_red;
    delete[] output_green;
    delete[] output_blue;
    
    destroy_matrix(backward_gain);
    
    //delete learning_buffer;
    //delete buffer;
    destroy_matrix(buffer);
    destroy_matrix(w);
    destroy_matrix(dw);
    destroy_matrix(input_scratch);
    destroy_matrix(top_down_scratch);
#ifdef USE_CUBLAS
	cudaFree(d_buffer);
	cudaFree(d_weight);
	cudaFree(d_activity);
	cublasDestroy(handle);
#endif
}




void
CSOM::GenerateOutput()  // Rearrange activity for output output [Copy activity to output [use fast copy to submatrix later]]
{
    // row major so do x direction first
    // output = activity
    if(output_type == 0) { // combined
        // iterate over streams
        for (int s = 0; s < upstreams; ++s)
            // iterate over som lattice
            for(int j=0; j<som_size_y; j++)
                for(int i=0; i<som_size_x; i++)
                    // iterate over each "pixel" in map
                    for(int l=0; l<map_size_y; l++)
                        for(int k=0; k<map_size_x; k++)
                        {
                            int y = l*som_size_y+j;
                            int x = k*som_size_x+i;
                            // put ix = y*output_sz_y + x
                            // map the activity of all the maps to 2 dimensions
                            output[s][y][x] = activity.at(s)[j][i][l][k];
                            // printf("j=%i, i=%i, l=%i, k=%i; x=%i, y=%i\n", j, i, l, k, x, y);
                        }

    }
    else if(output_type == 1) // separate
        for (int s = 0; s < upstreams; ++s)
            for(int j=0; j<som_size_y; j++)
                for(int i=0; i<som_size_x; i++)
                    for(int l=0; l<map_size_y; l++)
                        for(int k=0; k<map_size_x; k++)
                            output[s][j*map_size_y+l][i*map_size_x+k] = activity.at(s)[j][i][l][k];
                            // 012 012 012
    
}           



//
// Visualization functions
//
void
CSOM::GenerateColorCodedOutput()
{
    float minimum = maxfloat;
    float maximum = 0;
    for (int s = 0; s < upstreams; ++s)
    {
        for(int mj=0; mj<map_size_y; mj++)
            for(int mi=0; mi<map_size_x; mi++)
            {
                int max_i = 0;
                int max_j = 0;
                float m = activity.at(s)[0][0][mj][mi];
                float t;
                for(int j=0; j<som_size_y; j++)
                    for(int i=0; i<som_size_x; i++)
                        if((t = activity.at(s)[j][i][mj][mi]) > m)
                        {
                            max_i = i;
                            max_j = j;
                            m = t;
                        }
                
                output_red[s][mj][mi]      =  m * ((float(max_j)/float(som_size_y-1)));
                output_green[s][mj][mi]    =  m * ((float(max_i)/float(som_size_x-1)));
                output_blue[s][mj][mi]     =  m * (1 - 0.5*(float(max_j)/float(som_size_y-1)) - 0.5*(float(max_i)/float(som_size_x-1)));
                
                if(m > maximum)
                    maximum = m;
                else if(m < minimum)
                    minimum = m;
            }
        
        if(maximum > 0)
        {
            multiply(output_red[s], 1/maximum, map_size_x, map_size_y);
            multiply(output_green[s], 1/maximum, map_size_x, map_size_y);
            multiply(output_blue[s], 1/maximum, map_size_x, map_size_y);
        }
    }
}



void
CSOM::GenerateWeightOutput()
{
    // map 1:1 to a 2 dim array
    for(int j=0; j<som_size_y; j++)
        for(int i=0; i<som_size_x; i++)
            for(int l=0; l<rf_size_y; l++)
                for(int k=0; k<rf_size_x; k++)
                {
                    float tmp = w[j][i][l][k];
                    weights[j*rf_size_y+l][i*rf_size_x+k] = tmp;
                }
  
    normalize_max(weights, rf_size_x*som_size_x, rf_size_y*som_size_y); // TODO: Check that this is ok
}



// CalculateForwardActivation() calculates the linear forward mapping
// ONLY WORKS FOR *COMBINED* OUTPUT
//

void
CSOM::CalculateForwardActivation() // 3.3%
{
    int inp_scr_x = input_size_x + 2*border_mult * span_size_x;
    int inp_scr_y = input_size_y + 2*border_mult * span_size_y;
    int offset_x = (map_size_x_scr - map_size_x) / 2;
    int offset_y = (map_size_y_scr - map_size_y) / 2;
    // iterate over streams
    for (int str = 0; str < upstreams; ++str)
    {
        {
            //reset_matrix(input_scratch, inp_scr_x, inp_scr_y);
            // setsubmatrix(inputscratch, input[str], x0,x1, y0,y1)

            //set_submatrix(*input_scratch, inp_scr_x, *(input[str]), input_size_y, input_size_x, border_mult * span_size_y, border_mult * span_size_x);
            // spanned_im2row(buffer[str], inputscratch, map_size_x, map_size_y, input_size_x, input_size_y, rf_size_x, rf_size_y, rf_inc_x, rf_inc_y, block_size_x, block_size_y, span_size_x, span_size_y);
			spanned_im2row(buffer[str], input[str], map_size_x, map_size_y, input_size_x, input_size_y, rf_size_x, rf_size_y, rf_inc_x, rf_inc_y, block_size_x, block_size_y, span_size_x, span_size_y);
			//float **buffer_t = create_matrix(buffer_size_y, buffer_size_x);
			//transpose(buffer_t, buffer[str], buffer_size_y, buffer_size_x);

#ifdef USE_CUBLAS
			  
			float *d_aa = d_buffer;
			float *d_bb = d_weight;
			float *d_rr = d_activity;

			// int size2_x = rf_size_x*rf_size_y;
			// int size2_y = som_size_x*som_size_y;
			// int size1_x = buffer_size_x; // because transposed
			// int size1_y = buffer_size_y;
			int sizex = som_size_x*som_size_y; //size2_y;
			int sizey = buffer_size_y; //size1_y;
			int n = rf_size_x*rf_size_y; //size2_x;
			
			// float **input1 = buffer[str];
			// float **input2 = w[0][0];
			// float **output = (activity.at(str))[0][0];
			float **aa = buffer[str]; //input1;
			float **bb = w[0][0]; //input2;
			float **r = (activity.at(str))[0][0]; //output;
			 
		  float alpha = 1.f;
		  float beta = 0.f;
		  // set values on device
		   cublasStatus_t status;
		  cudaError_t error;
		  error = cudaSetDevice(device_id);
		  cuda_checkerror(error, 
				(GetName() + std::string("cudaSetDevice: error")).c_str());
		  //cuda_checkerror(status, "CSOM::CalculateForwardActivation: set stream failed");
		  //status = cublasSetMatrix(sizey, n, sizeof(**aa), *aa, sizey, d_aa, sizey);
			//printf("%s, buffer x=%i, y=%i, sizey=%i, n=%i\n", GetName(), buffer_size_x, buffer_size_y,sizey, n);
		  status = cublasSetVector(sizey* n, sizeof(**aa), *aa, 1, d_aa, 1);
		  cuda_checkerror(status, 
				(GetName() + std::string(": multiply(r, a, b, sizex, sizey, n): CUDA setmatrix error: d_aa")).c_str());
		  //status = cublasSetMatrix(n, sizex, sizeof(**bb), *bb, n, d_bb, n);
			//printf("%s, weight(bb) x=%i, y=%i, sizex=%i, n=%i\n", GetName(), 
			//	rf_size_x*rf_size_y, som_size_x*som_size_y, sizex, n);
		  status = cublasSetVector(n * sizex, sizeof(**bb), *bb, 1, d_bb, 1);
		  cuda_checkerror(status, 
				(GetName() + std::string(": multiply(r, a, b, sizex, sizey, n): CUDA setmatrix error: d_bb")).c_str());

		  //status = cublasSetMatrix(sizey, sizex, sizeof(**r), *r, sizey, d_rr, sizey);
		  status = cublasSetVector(sizey * sizex, sizeof(**r), *r, 1, d_rr, 1);
		  cuda_checkerror(status, "multiply(r, a, b, sizex, sizey, n): CUDA setmatrix error: d_rr");
		  
		  //status = cublasSetStream(handle, stream);
		  //status = cublasSgemm((cublasHandle_t)handle, CUBLAS_OP_T, CUBLAS_OP_T, sizey, sizex, n, 
		  //  	  &alpha, d_aa, sizex, d_bb, n, &beta, d_rr, sizey);
			multiply_t(d_rr, d_aa, d_bb, sizex, sizey, n, handle);
		  cuda_checkerror(status, "multiply(r, a, b, sizex, sizey, n): CUDA Sgemm error");
		  // retrieve answer
		  //status = cublasGetMatrix(sizey, sizex, sizeof(**r), d_rr, sizey, *r, sizey);
		  status = cublasGetVector(sizey * sizex, sizeof(**r), d_rr, 1, *r, 1);
		  cuda_checkerror(status, "multiply(r, a, b, sizex, sizey, n): CUDA getmatrix error");
#else
            // TODO fix blurry, misaligned output of multiply_t
            //multiply_t(**(activity.at(str)), buffer[str], **w, som_size_y * som_size_x, buffer_size_y,
            //           rf_size_x * rf_size_y);
            
			 for (int sj = 0; sj < som_size_y; sj++)
                for (int si = 0; si < som_size_x; si++)
                {
                    // im2row(buffer, input, map_size_x, map_size_y, input_size_x, input_size_y, rf_size_x, rf_size_y, rf_inc_x, rf_inc_y);
                    //reset_matrix(activity_scratch, map_size_x_scr, map_size_y_scr);
                    multiply(*(activity.at(str)[sj][si]), buffer[str], *w[sj][si], buffer_size_x, buffer_size_y);
                    
                }
                /**/
#endif
			//destroy_matrix(buffer_t);
        }
        //UpdateLearningBuffer();
	}
}

void
CSOM::UpdateLearningBuffer()
{
    // TODO update also learning_activity
    int *ixbuf = new int[learning_buffer_size];
    random_int(ixbuf, 0, buffer_size_y, learning_buffer_size);
    for(int j=0; j<learning_buffer_size; ++j)
        for(int i=0; i< buffer_size_x; ++i)
            learning_buffer[j][i] = buffer[ixbuf[j]][i];
        //copy_array(learning_buffer[i], buffer[ixbuf[i]], buffer_size_x);
}

/*
void
CSOM::CalculateBackwardActivation() // 22.2% -> 15.8% -> 4.3% -> 1.4%
{
    if(!top_down) return;
    
    reset_matrix(reconstruction, input_size_x, input_size_y);

    for (int mj=0; mj<map_size_y; mj++)
        for (int mi=0; mi<map_size_x; mi++)
        {
            int mjsy = mj*som_size_y;
            int misx = mi*som_size_x;
            int rfimi = rf_inc_x*mi;
            int rfjmj = rf_inc_y*mj;
            
            for(int sj=0; sj<som_size_y; sj++)
                for(int si=0; si<som_size_x; si++)
                {
                    float ** ww = w[sj][si];
                    float td = top_down[mjsy+sj][misx+si];

                    for(int rj=0; rj<rf_size_y; rj++)
                         multiply(&reconstruction[rfjmj+rj][rfimi], ww[rj], td, rf_size_x);
                }
        }

    for(int i=0; i<input_size_x; i++)
        for(int j=0; j<input_size_y; j++)
            reconstruction[j][i] *= backward_gain[j][i];        // 7.6%
}
*/


void
CSOM::CalculateBackwardActivation() // 22.2% -> 15.8% -> 4.3% -> 1.4%
{
    if(!top_down) return;
    
    // reset_matrix(reconstruction, input_size_x, input_size_y);

    // for (int mj=0; mj<map_size_y; mj++)
    //     for (int mi=0; mi<map_size_x; mi++)
    //     {
    //         for(int sj=0; sj<som_size_y; sj++)
    //             for(int si=0; si<som_size_x; si++)
    //                 for(int ri=0; ri<rf_size_x; ri++)
    //                     for(int rj=0; rj<rf_size_y; rj++)
    //                         reconstruction[rf_inc_y*mj+rj][rf_inc_x*mi+ri] += top_down[mj*som_size_y+sj][mi*som_size_x+si] * w[sj][si][rj][ri];
    //     }

    // for(int i=0; i<input_size_x; i++)
    //     for(int j=0; j<input_size_y; j++)
    //         reconstruction[j][i] *= backward_gain[j][i];        // 7.6%
    
    // int offset_x = border_mult * span_size_x;
    // int offset_y = border_mult * span_size_y;
    // int inp_scr_x = input_size_x + 2 * offset_x;
    // int inp_scr_y = input_size_y + 2 * offset_y;
    // int topd_scr_x = som_size_x * map_size_x_scr; 
    // int topd_scr_y = som_size_y * map_size_y_scr;
    // int td_offset_x = som_size_x * (map_size_x_scr - map_size_x) / 2;
    // int td_offset_y = som_size_y * (map_size_x_scr - map_size_x) / 2;
                                                                                       // normalize_max(reconstruction, input_size_x, input_size_y); // TODO: Check that this is ok
    for(int i=0; i<downstreams; ++i)                                                                                   for (int i = 0; i < downstreams; ++i)
    {
        // loop regeneration with scratch
        //reset_matrix(input_scratch, inp_scr_x, inp_scr_y);
        //reset_matrix(top_down_scratch, topd_scr_x, topd_scr_y);
        //set_submatrix(*top_down_scratch, topd_scr_x, *top_down[i], output_size_y, output_size_x, td_offset_y, td_offset_x);
        //Regenerate(input_scratch, top_down_scratch, inp_scr_x, inp_scr_y, map_size_x_scr, map_size_y_scr);
        //get_submatrix(*reconstruction[i], input_size_y, input_size_x, *input_scratch, inp_scr_x, offset_y, offset_x);

        // matrix mult regeneration:
        Regenerate(reconstruction[i], top_down[i], 
            input_size_x, input_size_y, 
            map_size_x, map_size_y);
    }
}

// reconstruction[rf_inc_y*mj+rj][rf_inc_x*mi+ri] += top_down[mj*som_size_y+sj][mi*som_size_x+si] * w[sj][si][rj][ri];     // 90%



//
// Tick
//

void
CSOM::Tick()
{
    // Categorization Layer

    CalculateForwardActivation();
    if(update_weights) UpdateWeights();
/*
    if(alpha_decay < 0.124 && GetTick() > 10000) // SECOND LAYER :-)
    {
    for (int mj=0; mj<map_size_y; mj++)
        for (int mi=0; mi<map_size_x; mi++)
        {
            for(int sj=0; sj<som_size_y; sj++)
                for(int si=0; si<som_size_x; si++)
                {
                    activity[sj][si][mj][mi] *= ((GetTick() % som_size_x*som_size_y) == sj*som_size_x+si  ? 0 : 1);
                }
        }
    }
*/    
    CalculateBackwardActivation();
    
    // Outputs

    GenerateOutput();
    GenerateColorCodedOutput();
    GenerateWeightOutput();
    if(update_weights)
    {
        //CalcError();
        //CalcProgress();
    }

    // update learning rate
    alpha = alpha > alpha_min ? alpha*alpha_decay : alpha;
}

void
CSOM::Serialize(const char *fname)
{
    FILE *file = fopen(fname, "wb");
    if (file == NULL)
    {
        printf("CSOM::Serialize (%s): Could not open output file \"%s\" \n", GetName(), fname);
        return;
    }
    // write to file
    // write the 4d data
    if(!serialize_only_weights)
    	for (int i = 0; i < upstreams; ++i)
        	Serialize4d(file, activity.at(i), som_size_y, som_size_x, map_size_y, map_size_x);
    Serialize4d(file, w,  som_size_y, som_size_x, rf_size_y, rf_size_x);
    if(!serialize_only_weights)
    {
        Serialize4d(file, dw, som_size_y, som_size_x, rf_size_y, rf_size_x);

        // write the 2d data
        Serialize2d(file, arbor, rf_size_y, rf_size_x);
        Serialize2d(file, backward_gain, input_size_y, input_size_x);
    }
    fclose(file);

}

void 
CSOM::Deserialize(const char *fname)
{
    FILE *file = fopen(fname, "r");
    if (file == NULL)
    {
        printf("CSOM::Serialize (%s): Could not open input file \"%s\" \n", GetName(), fname);
        return;
    }
    // read from file - 2d
    if(!serialize_only_weights)
	    for (int i = 0; i < upstreams; ++i)
    	    Deserialize4d(file, activity.at(i), som_size_y, som_size_x, map_size_y, map_size_x);
    Deserialize4d(file, w,  som_size_y, som_size_x, rf_size_y, rf_size_x);
    if(!serialize_only_weights)
    {
        Deserialize4d(file, dw, som_size_y, som_size_x, rf_size_y, rf_size_x);

        // write the 2d data
        Deserialize2d(file, arbor, rf_size_y, rf_size_x);
        Deserialize2d(file, backward_gain, input_size_y, input_size_x);
    }
    fclose(file);
}

void
CSOM::CalcError()
{
    // calc top down reconstruction with current output
	 //for(int str=0; str<downstreams; ++str)
    for(int str=0; str<1; ++str) // 2017-04-24: only calc for first output - may be different num of ups and down streams
	 {
         // TODO update so works with new Regenerate
		 Regenerate(local_reconstruction[str], output[str], input_size_x, input_size_y, map_size_x, map_size_y);

		 float **diff = create_matrix(input_size_x, input_size_y);
		 subtract(diff, local_reconstruction[str], input[str], input_size_x, input_size_y);
		 prev_error = error[0];
		 // use frobenius norm
		 // error = sqrt(sum(square(reconstruction - input)))
		 // error[0] = norm1(subtract(local_reconstruction, input, input_size_x, input_size_y), 
		 //                 input_size_x,
		 //                 input_size_y);

		 // use mean square - move to math.h?
		 {
			  // error = sqrt(sum(square(reconstruction - input))/(x*y))
			  for (int y = 0; y < input_size_y; ++y)
					for (int x = 0; x < input_size_x; ++x)
						 error[0] += diff[y][x]*diff[y][x];     
			  error[0] /= input_size_x*input_size_y;
			  error[0] = sqrt(error[0]);
		 }
		 destroy_matrix(diff);
	}
    
}

void 
CSOM::CalcProgress()
{
    progress[0] = prev_error - error[0];
}

void
CSOM::Regenerate(float **out_reconstruction, float ** input, int size_x, int size_y, int map_x, int map_y)
{
    // TODO change to use matrix multiply
    // spanned_im2row(recons_buffer, input, ..)
    // double check params here - this can be called with scratch space
    // around
    /**/
    spanned_im2row(topdown_buffer, input, 
        map_x, map_y,
        output_size_x, output_size_y,
        som_size_x, som_size_y,
        som_size_x, som_size_y,
        som_size_x, som_size_y,
        0, 0);
    //printf("im2row done\n" );
    // multiply(tmp, recons_buffer, weights, ..)
    float **tmp = create_matrix(buffer_size_x, buffer_size_y);
    multiply(tmp, topdown_buffer, **w, buffer_size_x, buffer_size_y, 
        som_size_x*som_size_y);
    //printf("mult done\n" );
    // cumulative_take(out_reconstruction, tmp, indeces, ..) // spanned_row2im
    reset_matrix(out_reconstruction, size_x, size_y);
    spanned_row2im(out_reconstruction, tmp, 
        size_x, size_y,
        map_x, map_y,
        rf_size_x, rf_size_y,
        rf_inc_x, rf_inc_y,
        block_size_x, block_size_y,
        span_size_x, span_size_y);
    //printf("row2im done\n" );
    destroy_matrix(tmp);
    

    /*
     reset_matrix(out_reconstruction, size_x, size_y);
    for (int mj=0; mj<map_y; mj++)
        for (int mi=0; mi<map_x; mi++)
        {
            for(int sj=0; sj<som_size_y; sj++)
                for(int si=0; si<som_size_x; si++)
                    for(int ri=0; ri<rf_size_x; ri++)
                    {
                        int dv_x = ri/block_size_x;
                        int offset_x = dv_x*span_size_x;
                        for(int rj=0; rj<rf_size_y; rj++)
                        {
                            int dv_y = rj/block_size_y;
                            int offset_y = dv_y*span_size_y;
                            out_reconstruction[rf_inc_y*mj+rj+offset_y][rf_inc_x*mi+ri+offset_x] += 
                                input[mj*som_size_y+sj][mi*som_size_x+si] * w[sj][si][rj][ri];
                        }
                    }
        }*/
    // TODO fix for borders, span if necessary
    // for(int i=0; i<size_x; i++)
    //     for(int j=0; j<size_y; j++)
    //         out_reconstruction[j][i] *= backward_gain[j][i];        // 7.6%
    
    normalize_max(out_reconstruction, size_x, size_y); // TODO: Check that this is ok

}

int 
CSOM::CalcMapSize(
    int inpsz,
    int rfsz,
    int blksz,
    int spansz,
    int inc)
{
    return (inpsz-(rfsz+(rfsz/blksz-1)*spansz)) /(inc) + 1;
}

static InitClass init("CSOM", &CSOM::Create, "Source/Modules/ANN/CSOM/CSOM/");
