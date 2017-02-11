//
//    CSOM.cc   This file is a part of the IKAROS project
//				Self-Organizing Convolution Network
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


#include "CSOM.h"

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

    rf_size_x = GetIntValue("rf_size_x");
    rf_size_y = GetIntValue("rf_size_y");
   
    rf_inc_x = GetIntValue("rf_inc_x");
    rf_inc_y = GetIntValue("rf_inc_y");
   
    som_size_x = GetIntValue("som_size_x");
    som_size_y = GetIntValue("som_size_y");
   
    AddInput("INPUT");
    AddInput("TOP_DOWN");
    AddOutput("OUTPUT");
    AddOutput("RECONSTRUCTION");

    AddOutput("WEIGHTS", rf_size_x*som_size_x, rf_size_y*som_size_y);

    AddOutput("OUTPUT_RED");
    AddOutput("OUTPUT_GREEN");
    AddOutput("OUTPUT_BLUE");

    output_type = GetIntValueFromList("output_type");
}



void
CSOM::SetSizes()
{
    int sx = GetInputSizeX("INPUT");
    int sy = GetInputSizeY("INPUT");
    if(sx == unknown_size || sy == unknown_size)
        return;
	
    int map_size_x = (sx-rf_size_x) / rf_inc_x + 1;
    int map_size_y = (sy-rf_size_y) / rf_inc_y + 1;

    SetOutputSize("OUTPUT", som_size_x*map_size_x, som_size_y*map_size_y);
    SetOutputSize("OUTPUT_RED", map_size_x, map_size_y);
    SetOutputSize("OUTPUT_GREEN", map_size_x, map_size_y);
    SetOutputSize("OUTPUT_BLUE", map_size_x, map_size_y);

	SetOutputSize("RECONSTRUCTION", sx, sy);
}



void
CSOM::Init()
{
    input_size_x = GetInputSizeX("INPUT");
    input_size_y = GetInputSizeY("INPUT");

    output_size_x = GetOutputSizeX("OUTPUT");
    output_size_y = GetOutputSizeY("OUTPUT");

    map_size_x = (input_size_x-rf_size_x) / rf_inc_x + 1;
    map_size_y = (input_size_y-rf_size_y) / rf_inc_y + 1;

    buffer_size_x = rf_size_x * rf_size_y;
    buffer_size_y = map_size_x * map_size_y;
    buffer = create_matrix(buffer_size_x, buffer_size_y); // These should be allocated only once in Init()
    
    input           = GetInputMatrix("INPUT");
    top_down        = GetInputMatrix("TOP_DOWN", false);    // FIXME: Should be parameter in ikc file required="NO"
    reconstruction  = GetOutputMatrix("RECONSTRUCTION");
    backward_gain   = create_matrix(input_size_x, input_size_y);
    
    output		= GetOutputMatrix("OUTPUT");
    weights     = GetOutputMatrix("WEIGHTS");

    output_red	= GetOutputMatrix("OUTPUT_RED");
    output_green= GetOutputMatrix("OUTPUT_GREEN");
    output_blue	= GetOutputMatrix("OUTPUT_BLUE");

    activity    = create_matrix(map_size_x, map_size_y, som_size_x, som_size_y);

    float r = (rf_size_x+0.5)/2;
    float c = (rf_size_x)/2;
    
    // Quadratic arbor function
    
    arbor = create_matrix(rf_size_x, rf_size_y);
	set_matrix(arbor, 1.0, rf_size_x, rf_size_y);
    if(use_arbor)
	{
		for(int y=0; y<rf_size_y; y++)
			for(int x=0; x<rf_size_x; x++)
				arbor[y][x] = max(0.0f, 1.0f-sqr(hypot(x-c, y-c))/sqr(r));
	}
    
    // Init weights to random values

    w	= new float *** [som_size_y];
    for(int j=0; j<som_size_y; j++)
    {
        w[j] = new float ** [som_size_x];
        for(int i=0; i<som_size_x; i++)
            w[j][i] = multiply(random(create_matrix(rf_size_x, rf_size_y), 0.0, 0.0001, rf_size_x, rf_size_y), arbor, rf_size_x, rf_size_y);
//            w[j][i] = normalize(multiply(random(create_matrix(rf_size_x, rf_size_y), 0.0, 0.2, rf_size_x, rf_size_y), arbor, rf_size_x, rf_size_y), rf_size_x, rf_size_y);
    }
	
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
}



CSOM::~CSOM()
{
    return;
    
    // FIXME: destroy w and dw here ***

    destroy_matrix(backward_gain);
    destroy_matrix(activity);
    destroy_matrix(arbor);
    
    for(int j=0; j<som_size_y; j++)
    {
        for(int i=0; i<som_size_x; i++)
            destroy_matrix(w[j][i]);
        delete [] w[j];
    }    
    delete w;

    for(int j=0; j<som_size_y; j++)
    {
        for(int i=0; i<som_size_x; i++)
            destroy_matrix(dw[j][i]);
        delete [] dw[j];
    }    
    delete dw;
        
}



//
// Visualization functions
//

void
CSOM::GenerateOutput()  // Rearrange activity for output output [Copy activity to output [use fast copy to submatrix later]]
{
    if(output_type == 0) // combined
        for(int j=0; j<som_size_y; j++)
            for(int i=0; i<som_size_x; i++)
                for(int l=0; l<map_size_y; l++)
                    for(int k=0; k<map_size_x; k++)
                        output[l*som_size_y+j][k*som_size_x+i] = activity[j][i][l][k];
    
    else if(output_type == 1) // separate
        for(int j=0; j<som_size_y; j++)
            for(int i=0; i<som_size_x; i++)
                for(int l=0; l<map_size_y; l++)
                    for(int k=0; k<map_size_x; k++)
                        output[j*map_size_y+l][i*map_size_x+k] = activity[j][i][l][k];
}



void
CSOM::GenerateColorCodedOutput()
{
    float minimum = maxfloat;
    float maximum = 0;
    for(int mj=0; mj<map_size_y; mj++)
        for(int mi=0; mi<map_size_x; mi++)
        {
            int max_i = 0;
            int max_j = 0;
            float m = activity[0][0][mj][mi];
            float t;
            for(int j=0; j<som_size_y; j++)
                for(int i=0; i<som_size_x; i++)
                    if((t = activity[j][i][mj][mi]) > m)
                    {
                        max_i = i;
                        max_j = j;
                        m = t;
                    }
            
            output_red[mj][mi]      =  m * ((float(max_j)/float(som_size_y-1)));
            output_green[mj][mi]    =  m * ((float(max_i)/float(som_size_x-1)));
            output_blue[mj][mi]     =  m * (1 - 0.5*(float(max_j)/float(som_size_y-1)) - 0.5*(float(max_i)/float(som_size_x-1)));
            
            if(m > maximum)
                maximum = m;
            else if(m < minimum)
                minimum = m;
        }
    
    if(maximum > 0)
    {
        multiply(output_red, 1/maximum, map_size_x, map_size_y);
        multiply(output_green, 1/maximum, map_size_x, map_size_y);
        multiply(output_blue, 1/maximum, map_size_x, map_size_y);
    }
}



void
CSOM::GenerateWeightOutput()
{
    for(int j=0; j<som_size_y; j++)
        for(int i=0; i<som_size_x; i++)
            for(int l=0; l<rf_size_y; l++)
                for(int k=0; k<rf_size_x; k++)
                    weights[j*rf_size_y+l][i*rf_size_x+k] = w[j][i][l][k];
  
    normalize_max(weights, rf_size_x*som_size_x, rf_size_y*som_size_y); // TODO: Check that this is ok
}



// CalculateForwardActivation() calculates the linear forward mapping
// ONLY WORKS FOR *COMBINED* OUTPUT
//

/*/
void
CSOM::CalculateForwardActivation() // 3.3%
{
    if(rf_inc_x == 1 && rf_inc_y == 1) // inc == 1: do fast convolution
    {
        for(int sj=0; sj<som_size_y; sj++)
            for(int si=0; si<som_size_x; si++)                
                convolve(activity[sj][si], input, w[sj][si], map_size_x, map_size_y, rf_size_x, rf_size_y);
    }

    else
    {
        for(int sj=0; sj<som_size_y; sj++)
            for(int si=0; si<som_size_x; si++)
                for (int mj=0; mj<map_size_y; mj++)
                    for (int mi=0; mi<map_size_x; mi++)
                    {
                        float s = 0.0;
                        for (int rj=0; rj<rf_size_y; rj++)
                            for (int ri=0; ri<rf_size_x; ri++)                                  // 48%        
                                s += w[sj][si][rj][ri] * input[rf_inc_y*mj+rj][rf_inc_x*mi+ri]; // 49%
     
                        activity[sj][si][mj][mi] = s;
                    }
    }
}
*/


// New version using im2row + multiply for convolution
// Works for both increment 1 and larger strides

void
CSOM::CalculateForwardActivation()
{
    for(int sj=0; sj<som_size_y; sj++)
        for(int si=0; si<som_size_x; si++)
        {
            im2row(buffer, input, map_size_x, map_size_y, input_size_x, input_size_y, rf_size_x, rf_size_y, rf_inc_x, rf_inc_y);
            multiply(*activity[sj][si], buffer, *w[sj][si], buffer_size_x, buffer_size_y);
        }
}



void
CSOM::CalculateBackwardActivation() // 22.2% -> 15.8% -> 4.3% -> 1.4%
{
    if(!top_down) return;
    
    reset_matrix(reconstruction, input_size_x, input_size_y);

    for (int mj=0; mj<map_size_y; mj++)
        for (int mi=0; mi<map_size_x; mi++)
        {
            for(int sj=0; sj<som_size_y; sj++)
                for(int si=0; si<som_size_x; si++)
                    for(int ri=0; ri<rf_size_x; ri++)
                        for(int rj=0; rj<rf_size_y; rj++)
                            reconstruction[rf_inc_y*mj+rj][rf_inc_x*mi+ri] += top_down[mj*som_size_y+sj][mi*som_size_x+si] * w[sj][si][rj][ri];
        }

    for(int i=0; i<input_size_x; i++)
        for(int j=0; j<input_size_y; j++)
            reconstruction[j][i] *= backward_gain[j][i];        // 7.6%
    
    normalize_max(reconstruction, input_size_x, input_size_y); // TODO: Check that this is ok
}

// reconstruction[rf_inc_y*mj+rj][rf_inc_x*mi+ri] += top_down[mj*som_size_y+sj][mi*som_size_x+si] * w[sj][si][rj][ri];     // 90%



//
// Tick
//

void
CSOM::Tick()
{
    // Alpha decay
    
    alpha = max(alpha_min, alpha_decay * alpha);
//    printf("%f\n", alpha);
    
    // Categorization Layer

    CalculateForwardActivation();
    UpdateWeights();
    CalculateBackwardActivation();
    
    // Outputs

    GenerateOutput();
    GenerateColorCodedOutput();
    GenerateWeightOutput();
}



static InitClass init("CSOM", &CSOM::Create, "Source/Modules/ANN/CSOM/CSOM/");
