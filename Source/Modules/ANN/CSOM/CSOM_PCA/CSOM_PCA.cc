//
//	CSOM_PCA.cc   This file is a part of the IKAROS project
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

/*

    INCREMENT:
        always 1 gives convolution

    WEIGHT_VISUALIZATION:
        size: rf_size * som_size
    
    OUTPUT:
        size: som_size * (input_size - rf_size + 1)
        packing: multiple SOMs / multiple convoluted maps
*/



#include "CSOM_PCA.h"

//#include <Accelerate/Accelerate.h>
//#include <dispatch/dispatch.h>

using namespace ikaros;

static int ctr = 0;

CSOM_PCA::CSOM_PCA(Parameter * p):
	CSOM(p)
{ 
}



CSOM_PCA::~CSOM_PCA()
{
    destroy_matrix(inh_prev);
    destroy_matrix(inh_buffer);
    destroy_matrix(tmp_outer);
    destroy_matrix(delta_buf);
    destroy_matrix(delta_buf_t);
    destroy_array(tmp_act);
    destroy_matrix(mapped_weights);
    destroy_matrix(mapped_act);
    destroy_matrix(mapped_dw);
    
    
}

void
CSOM_PCA::Init()
{
    CSOM::Init();
    Bind(update_algo, "update_algo");
    numkernels = som_size_y*som_size_x;
    kernelsize = rf_size_x*rf_size_y;
    inp_rows = map_size_y*map_size_x;


    

    // block size default to size of receptive field
    block_size_x = block_size_x < 0 ? rf_size_x : block_size_x;
    block_size_y = block_size_y < 0 ? rf_size_y : block_size_y;

    // create necessary matrices
    inh_prev = create_matrix(kernelsize, inp_rows);
    inh_prev = create_matrix(kernelsize, inp_rows);
    inh_buffer = create_matrix(kernelsize, inp_rows, numkernels);
    tmp_outer = create_matrix(kernelsize, inp_rows);
    delta_buf = create_matrix(kernelsize, inp_rows);
    delta_buf_t = create_matrix(inp_rows, kernelsize);
    tmp_act = create_array(inp_rows);
    mapped_dw = create_matrix(kernelsize, numkernels);
    mapped_weights = create_matrix(kernelsize, numkernels);
    mapped_act = create_matrix(inp_rows, numkernels);    

    //mapped_inp = create_matrix(kernelsize, inp_rows);
    
}
 
//
// weights are updated with PCA
//
void
CSOM_PCA::UpdateWeights()
{
    switch(update_algo)
    {
        case 1:
            UpdateWeights_mmlt_blnc();
            break;
        case 0:
        default:
            UpdateWeights_4d();
    }
    //if(ctr++ < 1)
    //else
    //UpdateWeights_mmlt();
    //UpdateWeights_mmlt_blnc_b();
}

void
CSOM_PCA::UpdateWeights_mmlt()
{

    // map activity and weights
    for (int str = 0; str < upstreams; ++str)
    {
        MapFrom4d(mapped_weights, w, som_size_y, som_size_y, rf_size_x, rf_size_y);
        MapFrom4d(mapped_act, activity[str], som_size_x, som_size_y, map_size_x, map_size_y);
        //print_matrix("mapped_weights", mapped_weights, kernelsize, numkernels, 6);
        //print_matrix("mapped_act", mapped_act, inp_rows, numkernels, 6);
        
        float **mapped_inp = learning_buffer[str]; // result of im2row in forward activation
        reset_matrix(mapped_dw, kernelsize, numkernels);

        
        outer(inh_prev, mapped_act[0], mapped_weights[0], kernelsize, inp_rows);
        
        //for (int k = 0; k < numkernels; ++k)
        //    printf("mapped_act ptr initial %i: %p\n", k, (void*)mapped_act[k]);
        for (int k = 0; k < numkernels; ++k)
        {
            //printf("mapped_act ptr %i: %p\n", k, (void*)mapped_act[k]);
            reset_matrix(tmp_outer, kernelsize, inp_rows);
            outer(tmp_outer, mapped_act[k], mapped_weights[k], 
                    kernelsize, inp_rows);
            
            add(inh_buffer[k], 
                tmp_outer,
                inh_prev,
                kernelsize, inp_rows);
            //print_matrix("inh_buffer", inh_buffer[k], kernelsize, inp_rows,6);
            copy_matrix(inh_prev, inh_buffer[k], kernelsize, inp_rows);
            subtract(delta_buf, mapped_inp, inh_buffer[k], kernelsize, inp_rows);
            //print_matrix("delta_buf", delta_buf, kernelsize, inp_rows, 6);
            transpose(delta_buf_t, delta_buf, inp_rows, kernelsize);
            reset_array(tmp_act, inp_rows);
            multiply(tmp_act, mapped_act[k], alpha, inp_rows);
            //printf("mapped_act ptr bef %i: %p w: %p\n", k, (void*)mapped_act[k], (void*)mapped_weights[k]);
            //print_array("dw bef", mapped_dw[k], kernelsize, 6);
            multiply(mapped_dw[k], delta_buf_t, tmp_act, inp_rows, kernelsize);
            //print_array("dw after", mapped_dw[k], kernelsize, 6);
            //printf("mapped_act ptr aft %i: %p w: %p\n", k, (void*)mapped_act[k], (void*)mapped_weights[k]);
        /*
            //print_array("dw", dw[k], kernelsize, 6);

        */
        }
        //print_matrix("dw", mapped_dw, kernelsize, 1,6);
        add(mapped_weights, mapped_weights, mapped_dw, kernelsize, numkernels);
        //add(mapped_weights, mapped_dw, kernelsize, numkernels);
        //print_matrix("map weights", mapped_weights, kernelsize, 1);
        // map back
        MapTo4d(w, mapped_weights, som_size_x, som_size_y, rf_size_x, rf_size_y);
        // note:really necessary?
        //MapTo4d(activity[str], mapped_act, som_size_x, som_size_y, map_size_x, map_size_y);
        //destroy_array(act_array);
    }

    
}

// Balanced - som configuration controls horizontal inhibition
void
CSOM_PCA::UpdateWeights_mmlt_blnc()
{

    float **tmp_act_b = create_matrix(inp_rows, som_size_x);
    float **tmp_act_b_t = create_matrix(som_size_x, inp_rows);
    float **tmp_dw_t = create_matrix(som_size_x, kernelsize);

    for (int str = 0; str < upstreams; ++str)
    {
        // map activity and weights
        MapFrom4d(mapped_weights, w, som_size_x, som_size_y, rf_size_x, rf_size_y);
        MapFrom4d(mapped_act, activity[str], som_size_x, som_size_y, map_size_x, map_size_y);
        //print_matrix("mapped_weights", mapped_weights, kernelsize, numkernels, 6);
        //print_matrix("mapped_act", mapped_act, inp_rows, numkernels, 6);
        
        float **mapped_inp = learning_buffer[str]; //learning_buffer; // result of im2row in forward activation
        //reset_matrix(inh_buffer[0], kernelsize, inp_rows*numkernels);
        reset_matrix(mapped_dw, kernelsize, numkernels);

        reset_matrix(inh_prev, kernelsize, inp_rows);

        // try transpose both act and weights and transpose inh prev
        //multiply(inh_prev, &mapped_act[0], &mapped_weights[0], 
        //    kernelsize, inp_rows, som_size_x);
        transpose(tmp_act_b_t, &mapped_act[0], som_size_x, inp_rows);
        //transpose(tmp_dw_t, &mapped_weights[0], som_size_x, kernelsize);

        multiply(inh_prev, tmp_act_b_t, &mapped_weights[0], 
            kernelsize, inp_rows, som_size_x);
        //transpose(inh_prev, tmp_act_b_t, inp_rows, som_size_x);
        //printf("=====tick : %i=====\n", ctr);
        //print_matrix("inh_prev", inh_prev, kernelsize, inp_rows, 6);
        //for (int k = 0; k < numkernels; ++k)
        //    printf("mapped_act ptr initial %i: %p\n", k, (void*)mapped_act[k]);
        int startix=0;
        for (int kj = 0; kj < som_size_y; kj++)
        {
            reset_matrix(tmp_outer, kernelsize, inp_rows); 
            transpose(tmp_act_b_t, &mapped_act[startix], som_size_x, inp_rows); 
            multiply(tmp_outer, tmp_act_b_t, &mapped_weights[startix], 
                    kernelsize, inp_rows, som_size_x);
            
            // ib += tmp_outer
            add(inh_prev, //inh_buffer[kj], 
                tmp_outer,
                //inh_prev,
                kernelsize, inp_rows);
            
            //print_matrix("inh_buffer", inh_buffer[kj], kernelsize, inp_rows,6);
            //copy_matrix(inh_prev, inh_buffer[kj], kernelsize, inp_rows);
            // delta_buf = input-inh_prev
            reset_matrix(delta_buf, kernelsize, inp_rows);
            subtract(delta_buf, mapped_inp, inh_prev, kernelsize, inp_rows);
            //print_matrix("delta_buf", delta_buf, kernelsize, inp_rows, 6);
            //reset_matrix(delta_buf_t, inp_rows, kernelsize);
            transpose(delta_buf_t, delta_buf, inp_rows, kernelsize);
            
            // tmp_act_b = mapped_act * alpha
            //reset_matrix(tmp_act_b, inp_rows, som_size_x);
            //multiply(tmp_act_b, &mapped_act[startix], alpha, inp_rows, som_size_x);
            
            //reset_matrix(tmp_act_b_t, som_size_x, inp_rows);
            //transpose(tmp_act_b_t, tmp_act_b, som_size_x, inp_rows);
            transpose(tmp_act_b_t, &mapped_act[startix], som_size_x, inp_rows);
            //printf("mapped_act ptr bef %i: %p w: %p\n", k, (void*)mapped_act[k], (void*)mapped_weights[k]);
            //print_array("dw bef", mapped_dw[k], kernelsize, 6);

            // tmp_dw_t =  delta_buf_t * tmp_act_b_t
            reset_matrix(tmp_dw_t, som_size_x, kernelsize);
            multiply(tmp_dw_t, alpha, delta_buf_t, tmp_act_b_t, som_size_x, kernelsize, inp_rows);
            //print_matrix("tmp_dw_t", tmp_dw_t, som_size_x, kernelsize, 6);
            transpose(&mapped_dw[startix], tmp_dw_t, kernelsize, som_size_x);
            //print_array("dw after", mapped_dw[k], kernelsize, 6);
            //print_matrix("mapped_dw", mapped_dw, kernelsize, numkernels, 6);

            //printf("mapped_act ptr aft %i: %p w: %p\n", k, (void*)mapped_act[k], (void*)mapped_weights[k]);
            startix += som_size_x;
        /*
            //print_array("dw", dw[k], kernelsize, 6);
        */
        }
        
        //print_matrix("delta_buf", delta_buf, kernelsize, 5, 6);
        //print_matrix("tmp_act_b", tmp_act_b, inp_rows, som_size_x, 5);
        //print_matrix("dw", mapped_dw, kernelsize, 4,6);
        add(mapped_weights, mapped_weights, mapped_dw, kernelsize, numkernels);
        //add(mapped_weights, mapped_dw, kernelsize, numkernels);
        //if (ctr == 1400)
        //    print_matrix("map weights", mapped_weights, kernelsize, numkernels, 5);
        // map back
        MapTo4d(w, mapped_weights, som_size_x, som_size_y, rf_size_x, rf_size_y);
        // note: really necessary?:
        // MapTo4d(activity[str], mapped_act, som_size_x, som_size_y, map_size_x, map_size_y);
        //destroy_array(act_array);
    }
    destroy_matrix(tmp_act_b);
    destroy_matrix(tmp_act_b_t);
    destroy_matrix(tmp_dw_t);

    ctr++;

    
}

void
CSOM_PCA::UpdateWeights_mmlt_blnc_b()
{
    float **row_sum = create_matrix(kernelsize, inp_rows);
    float **row_sum_tmp = create_matrix(kernelsize, inp_rows);
    for (int str = 0; str < upstreams; ++str)
    {
        // map activity and weights
        MapFrom4d(mapped_weights, w, som_size_x, som_size_y, rf_size_x, rf_size_y);
        MapFrom4d(mapped_act, activity[str], som_size_x, som_size_y, map_size_x, map_size_y);
        //print_matrix("mapped_weights", mapped_weights, kernelsize, numkernels, 6);
        //print_matrix("mapped_act", mapped_act, inp_rows, numkernels, 6);
        
        float **mapped_inp = learning_buffer[str]; // result of im2row in forward activation
        //reset_matrix(inh_buffer[0], kernelsize, inp_rows*numkernels);
        reset_matrix(mapped_dw, kernelsize, numkernels);

        reset_matrix(inh_prev, kernelsize, inp_rows);

        int startix = 0;
        int endix = som_size_x;
        printf("\n\n=======counter: %i ========\n", ctr);
        print_matrix("mapped_act", mapped_act, 10, 2, 2);
        print_matrix("mapped_inp", mapped_inp, kernelsize, 2, 2);
        printf("---\n");
        for (int kj = 0; kj < som_size_y; kj++)
        {
            // first add activity and weight across row: inhbuf
            reset_matrix(row_sum, kernelsize, inp_rows);
            for (int i = startix; i < endix; ++i)
            {
                reset_matrix(row_sum_tmp, kernelsize, inp_rows);
                multiply(row_sum_tmp, mapped_act[i], mapped_weights[i], kernelsize, inp_rows);
                add(row_sum, row_sum_tmp, kernelsize, inp_rows);
            }
            add(inh_prev, row_sum, kernelsize, inp_rows);
            print_matrix("inh_prev", inh_prev, kernelsize, 2);
            // deltabuf = input - inhbuf
            subtract(delta_buf, mapped_inp, inh_prev, kernelsize, inp_rows);
            print_matrix("delta_buf", delta_buf, kernelsize, 2);
            // dw = alpha *(activity(kj))
            for (int i = startix; i < endix; ++i)
            {
                reset_matrix(row_sum_tmp, kernelsize, inp_rows);
                multiply(row_sum_tmp, mapped_act[i], delta_buf, kernelsize, inp_rows);
                multiply(row_sum_tmp, alpha, kernelsize, inp_rows);
                sum(mapped_dw[i], row_sum_tmp, kernelsize, inp_rows);
            }
            
            //printf("mapped_act ptr aft %i: %p w: %p\n", k, (void*)mapped_act[k], (void*)mapped_weights[k]);
            startix += som_size_x;
            endix += som_size_x;
        /*
            //print_array("dw", dw[k], kernelsize, 6);
        */
        }
        print_matrix("mapped_dw", mapped_dw, kernelsize, numkernels);
        //print_matrix("delta_buf", delta_buf, kernelsize, 5, 6);
        //print_matrix("dw", mapped_dw, kernelsize, 4,6);
        add(mapped_weights, mapped_dw, kernelsize, numkernels);
        print_matrix("mapped_weights", mapped_weights, kernelsize, numkernels);
        //add(mapped_weights, mapped_dw, kernelsize, numkernels);
        //if (ctr == 1400)
        //    print_matrix("map weights", mapped_weights, kernelsize, numkernels, 5);
        // map back
        MapTo4d(w, mapped_weights, som_size_x, som_size_y, rf_size_x, rf_size_y);
        //MapTo4d(activity, mapped_act, som_size_x, som_size_y, map_size_x, map_size_y);
        //destroy_array(act_array);
    }
    destroy_matrix(row_sum);
    destroy_matrix(row_sum_tmp);

    ctr++;

}

void
CSOM_PCA::UpdateWeights_4d() // apply ReceptiveField learning rule
{
    float ** input_buffer = create_matrix(rf_size_x, rf_size_y);
    // inhibition buffer is the size of the receptive field
    float ** inhibition_buffer = create_matrix(rf_size_x, rf_size_y);
    float ** delta_buffer = create_matrix(rf_size_x, rf_size_y);
    int rf_size = rf_size_x*rf_size_y;
    
    // iterate over activity-map "cells", line by line
    /*
    for each cell_index in activitymap:
        inputbuf = input(cell_index)
        for each som_index in som_lattice:
            reset inhibition buffer
            for each som_index_b <= som_index:
                activity = som_lattice[som_index_b][cell_index]
                inhibition buffer += activity * weight_lattice[som_index_b]
            deltabuffer = inputbuf - inhibition_buffer
            dw[som_index] = deltabuffer * (alpha*som_lattice[som_index][cell_index])
        for each som_index in som_lattice
            weight_lattice[som_index] += dw[som_index]
    */
    for (int str = 0; str < upstreams; ++str)
        for(int mj=0; mj<map_size_y; mj++){
            for(int mi=0; mi<map_size_x; mi++)
            {
            //    if(random(0.0, 1.0) > 0.05) break;
            
            // Perform learning once for each position in the map using the same SOm
            // since we are using weight sharing
            
            // fetch input to temporary (linear) buffer to simplify calculations
            // TODO: could this be done automatically by using a different data structure for the input?
            
            // input buffer is the size of the receptive field
            // TODO add span: compose input from blocks of size s with n cells between
            int rl = rf_inc_y*mj; // start position is moved by increment
            int rk = rf_inc_x*mi;

            for(int y=0; y<rf_size_y; y++){
                int dv_y = y/block_size_y;
                int offset_y = dv_y*span_size_y;
                for(int x=0; x<rf_size_x; x++){
                    int dv_x = x/block_size_x;
                    int offset_x = dv_x*span_size_x;
                    input_buffer[y][x] = input[str][rl+y+offset_y][rk+x+offset_x] * arbor[y][x]; 
                }
            }
            
            // with current "cell" iterate over each map in the SOM
            // and calculate weight change
            for(int sj=0; sj<som_size_y; sj++){
                for(int si=0; si<som_size_x; si++){
                    reset_array(*inhibition_buffer, rf_size);

                    // for each pixel and map, calc inhibition 
                    for(int ssj=0; ssj<som_size_y; ssj++)
                        for(int ssi=0; ssi<som_size_x; ssi++) // 5%
                        {
                            // for shortcutting:
                            float a = activity[str][ssj][ssi][mj][mi]; // 18% activity at a specific cell
                            float * ww = w[ssj][ssi][0];    // 14% weights for a specific som index
                            float * ib = inhibition_buffer[0];
                            // Calculate inhibition, but only do it if
                            // indeces are less or equal to current map:
                            // ie last map is inhibited by all the others,
                            // but first one is inhibited by none - this 
                            // is the principal component sorting?
                            // ib = activity of som units + weights  

                            // note: this is anti-hebbian learning - when several
                            // pixels in different maps 
                            // are active simultanously, inhibition increases.
                            // this means that the different weight maps are decorrelated -
                            // simultaneous activation becomes less likely
                            if(ssj <= sj || (sj == ssj && ssi <= si)) // TODO: fix ranges in loop instead; two dimensional version of i<=j
                                // r = r + alpha * a
                                // multiply the activity at a given pixel in a given map with the 
                                // weight of that map and add that to inhibition
                                add(ib, a, ww, rf_size); // 28%
                        }
                    
                    //for(int r=0; r<rf_size; r++)
                    //    dw[sj][si][0][r] = alpha * activity[sj][si][mj][mi] * (input_buffer[0][r] - inhibition_buffer[0][r]); // 44%
                    
                    // subtract inhibition from contents of
                    // input buffer to get change in weights
                    subtract(*delta_buffer, *input_buffer, *inhibition_buffer, rf_size); // 16%
                    // calculate weight change: delta* (alpha*activity)
                    multiply(*dw[sj][si], *delta_buffer, alpha * activity[str][sj][si][mj][mi], rf_size); // 6%               
                    // TAT 2015-11-08: moved from outside loop - 
                    // looks like it works, but may give diff results
                    // is now cumulated for each pixel instead of just last
                    //add(*w[sj][si], *dw[sj][si], rf_size);
                } // for si
            } // for sj
            
            // update the actual weights by adding weight change
            // note: could this have been moved into previous loop?    

            // TAT 2015-11-08: moved into inner loop
            for(int sj=0; sj<som_size_y; sj++)
                for(int si=0; si<som_size_x; si++)
                    add(*w[sj][si], *dw[sj][si], rf_size);

//                    for(int r=0; r<rf_size; r++)
//                        w[sj][si][0][r] += dw[sj][si][0][r]; // 35.9%
        } // end map-x
    }// end map-y
    
    destroy_matrix(input_buffer);
    destroy_matrix(inhibition_buffer);
    destroy_matrix(delta_buffer);
}

// target[kernels][inner] = source[sy][sy][innery][innerx]
void        
CSOM_PCA::MapFrom4d(float **target, float ****source, 
        int sx, int sy, int kx, int ky)
{
    int k=0;
    for (int ssy = 0; ssy < sy; ++ssy)
    {
        for (int ssx = 0; ssx < sx; ++ssx)
        {
            int k_ix=0;
            for (int kky = 0; kky < ky; ++kky)
            {
                for (int kkx = 0; kkx < kx; ++kkx)
                {
                    target[k][k_ix] = source[ssy][ssx][kky][kkx];
                    k_ix++;
                }
            }
            k++;
        }
    }
}


void           
CSOM_PCA::MapTo4d(float ****target, float **source,
        int sx, int sy, int kx, int ky)
{
    int k=0;
    for (int ssy = 0; ssy < sy; ++ssy)
    {
        for (int ssx = 0; ssx < sx; ++ssx)
        {
            int k_ix=0;
            for (int kky = 0; kky < ky; ++kky)
            {
                for (int kkx = 0; kkx < kx; ++kkx)
                {
                    target[ssy][ssx][kky][kkx] = source [k][k_ix];
                    k_ix++;
                }
            }
            k++;
        }
    }    
}


static InitClass init("CSOM_PCA", &CSOM_PCA::Create, "Source/Modules/ANN/CSOM/CSOM_PCA/");

