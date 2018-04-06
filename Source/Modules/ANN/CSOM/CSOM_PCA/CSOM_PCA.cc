//
//	CSOM_PCA.cc     This file is a part of the IKAROS project
//                  Self-Organizing Convolution Network
//
//    Copyright (C) 2008-2017 Christian Balkenius
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



CSOM_PCA::CSOM_PCA(Parameter * p):
	CSOM(p)
{ 
}



CSOM_PCA::~CSOM_PCA()
{
    return;
    CSOM::~CSOM();
}

 

void
CSOM_PCA::UpdateWeights() // apply RF learning rule
{
    float ** input_buffer = create_matrix(rf_size_x, rf_size_y);
    float ** inhibition_buffer = create_matrix(rf_size_x, rf_size_y);
    float ** delta_buffer = create_matrix(rf_size_x, rf_size_y);
    int rf_size = rf_size_x*rf_size_y;
    
    for(int mj=0; mj<map_size_y; mj++)
        for(int mi=0; mi<map_size_x; mi++)
        {
        //    if(random(0.0, 1.0) > 0.05) break;
            
            // Perform learning once for each position in the map using the same SOM
            // since we are using weight sharing
            
            // fetch input to temporary (linear) buffer to simplify calculations
            // TODO: could this be done automatically by using a different data structure for the input?
            
            int rl = rf_inc_y*mj;
            int rk = rf_inc_x*mi;
            for(int y=0; y<rf_size_y; y++)
                for(int x=0; x<rf_size_x; x++)
                    input_buffer[y][x] = input[rl+y][rk+x] * arbor[y][x]; 
            
            // Now iterate over each element in the SOM
            
            for(int sj=0; sj<som_size_y; sj++)
                for(int si=0; si<som_size_x; si++)
                {
                    // Calculate inhibition 
                    
                    reset_array(*inhibition_buffer, rf_size);
                    for(int ssj=0; ssj<som_size_y; ssj++)
                        for(int ssi=0; ssi<som_size_x; ssi++) // 5%
                        {
                            float a = activity[ssj][ssi][mj][mi]; // 18%
                            float * ww = w[ssj][ssi][0];    // 14%
                            float * ib = inhibition_buffer[0];
                            if(ssj <= sj || (sj == ssj && ssi <= si)) // TODO: fix ranges in loop instead; two dimensional version of i<=j
                                 add(ib, a, ww, rf_size); // 28%
                        }
                    
                    //for(int r=0; r<rf_size; r++)
                    //    dw[sj][si][0][r] = alpha * activity[sj][si][mj][mi] * (input_buffer[0][r] - inhibition_buffer[0][r]); // 44%
                    
                    subtract(*delta_buffer, *input_buffer, *inhibition_buffer, rf_size); // 16%
                    multiply(*dw[sj][si], *delta_buffer, alpha * activity[sj][si][mj][mi], rf_size); // 6%               
                }
            
            // update the actual weights
            
            for(int sj=0; sj<som_size_y; sj++)
                for(int si=0; si<som_size_x; si++)
                    add(*w[sj][si], *dw[sj][si], rf_size);
//                    for(int r=0; r<rf_size; r++)
//                        w[sj][si][0][r] += dw[sj][si][0][r]; // 35.9%
        }
    
    destroy_matrix(input_buffer);
    destroy_matrix(inhibition_buffer);
    destroy_matrix(delta_buffer);
}


static InitClass init("CSOM_PCA", &CSOM_PCA::Create, "Source/Modules/ANN/CSOM/CSOM_PCA/");

