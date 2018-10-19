//
//	CSOM_L.cc   This file is a part of the IKAROS project
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


#include "CSOM_L.h"

#include <Accelerate/Accelerate.h>
#include <dispatch/dispatch.h>

using namespace ikaros;


CSOM_L::CSOM_L(Parameter * p):
	Module(p)
{ 
    Bind(alpha, "alpha");
    Bind(alpha_min, "alpha_min");
    Bind(alpha_max, "alpha_max");
    Bind(alpha_decay, "alpha_decay");
        
    Bind(sigma, "sigma");
    Bind(sigma_min, "sigma_min");
    Bind(sigma_max, "sigma_max");
    Bind(sigma_decay, "sigma_decay");
    
	Bind(use_arbor, "use_arbor");

    rf_size_x = GetIntValue("rf_size_x");
    rf_size_y = GetIntValue("rf_size_y");
   
    rf_inc_x = GetIntValue("rf_inc_x");
    rf_inc_y = GetIntValue("rf_inc_y");
   
    som_size_x = GetIntValue("som_size_x");
    som_size_y = GetIntValue("som_size_y");
   
    assoc_radius_x = GetIntValue("assoc_radius_x");
    assoc_radius_y = GetIntValue("assoc_radius_y");
   
    assoc_size_x = (1+2*assoc_radius_x)*som_size_x;
    assoc_size_y = (1+2*assoc_radius_y)*som_size_y;
   
    AddInput("INPUT");
    AddOutput("OUTPUT");
    AddOutput("SALIENCE");

    AddOutput("WEIGHTS", rf_size_x*som_size_x, rf_size_y*som_size_y);
    AddOutput("WEIGHTS_A", assoc_size_x, assoc_size_y); // does not depend on (or scale with) input size

    AddOutput("OUTPUT_RED");
    AddOutput("OUTPUT_GREEN");
    AddOutput("OUTPUT_BLUE");

    AddOutput("ERROR", 1);
    AddOutput("PROGRESS", 1);
    
    AddOutput("STAT_DISTRIBUTION", som_size_x, som_size_y);

    output_type = GetIntValueFromList("output_type");
    topology = GetIntValueFromList("topology");
    
    radius = hypot(float(som_size_x)/2.0, float(som_size_y)/2.0);
	
	read_file = GetValue("read_file");
	write_file = GetValue("write_file");
}



void
CSOM_L::SetSizes()
{
    int sx = GetInputSizeX("INPUT");
    int sy = GetInputSizeY("INPUT");
    if(sx == unknown_size || sy == unknown_size)
        return;

    SetOutputSize("OUTPUT", som_size_x*(sx-rf_size_x+1) / rf_inc_x, som_size_y*(sy-rf_size_y+1) / rf_inc_y);
	
    SetOutputSize("OUTPUT_RED", (sx-rf_size_x+1) / rf_inc_x, (sy-rf_size_y+1) / rf_inc_y);
    SetOutputSize("OUTPUT_GREEN", (sx-rf_size_x+1) / rf_inc_x, (sy-rf_size_y+1) / rf_inc_y);
    SetOutputSize("OUTPUT_BLUE", (sx-rf_size_x+1) / rf_inc_x, (sy-rf_size_y+1) / rf_inc_y);
	
	int map_size_x = (sx-rf_size_x+1) / rf_inc_x;
    int map_size_y = (sy-rf_size_y+1) / rf_inc_y;
	SetOutputSize("SALIENCE", map_size_x, map_size_y);
}



void
CSOM_L::Init()
{
    input_size_x = GetInputSizeX("INPUT");
    input_size_y = GetInputSizeY("INPUT");

    output_size_x = GetOutputSizeX("OUTPUT");
    output_size_y = GetOutputSizeY("OUTPUT");

    map_size_x = (input_size_x-rf_size_x+1) / rf_inc_x;
    map_size_y = (input_size_y-rf_size_y+1) / rf_inc_y;
    
    input		= GetInputMatrix("INPUT");
    output		= GetOutputMatrix("OUTPUT");
	salience		= GetOutputMatrix("SALIENCE");
    weights     = GetOutputMatrix("WEIGHTS");

    output_red	= GetOutputMatrix("OUTPUT_RED");
    output_green= GetOutputMatrix("OUTPUT_GREEN");
    output_blue	= GetOutputMatrix("OUTPUT_BLUE");

    error = GetOutputArray("ERROR");
    progress = GetOutputArray("PROGRESS");
    
    stat_histogram      = create_matrix(som_size_x, som_size_y);
    stat_distribution   = GetOutputMatrix("STAT_DISTRIBUTION");
    
    activity            = create_matrix(map_size_x, map_size_y, som_size_x, som_size_y);
    learning_activity   = create_matrix(map_size_x, map_size_y, som_size_x, som_size_y);
  
    float r = (rf_size_x+0.5)/2;
    float c = (rf_size_x)/2;
    
    // Quadratic arbor function
    
    arbor = create_matrix(rf_size_x, rf_size_y);
	set_matrix(arbor, 1.0, rf_size_x, rf_size_y);
    if(use_arbor)
	{
		for(int y=0; y<rf_size_y; y++)
			for(int x=0; x<rf_size_x; x++)
				arbor[y][x] = max(0.0f, 1.0f-sqr(ikaros::hypot(x-c, y-c))/sqr(r));
	}

    w	= new float *** [som_size_y];
    for(int j=0; j<som_size_y; j++)
    {
        w[j] = new float ** [som_size_x];
        for(int i=0; i<som_size_x; i++)
            w[j][i] = normalize(multiply(random(create_matrix(rf_size_x, rf_size_y), 0.0, 0.2, rf_size_x, rf_size_y), arbor, rf_size_x, rf_size_y), rf_size_x, rf_size_y);
    }
	
	// read data?
	
	if(read_file != NULL && !equal_strings(read_file, ""))
	{
		printf("Reading\n");
		FILE * f = fopen(read_file, "r");		
		for(int j=0; j<som_size_y; j++)
			for(int i=0; i<som_size_x; i++)
				for (int rj=0; rj<rf_size_y; rj++)
					for (int ri=0; ri<rf_size_x; ri++)
						fscanf(f, "%f\n", &w[j][i][ri][rj]);
		fclose(f);
	}
	
    // Associative structures
    
    activity_a          = create_matrix(map_size_x, map_size_y, som_size_x, som_size_y);
    w_a                 = create_matrix(assoc_size_x, assoc_size_y, som_size_x, som_size_y);
}



CSOM_L::~CSOM_L()
{
	// write data
	
	if(write_file != NULL && !equal_strings(write_file, ""))
	{
		printf("Writing\n");
		FILE * f = fopen(write_file, "w");		
		for(int j=0; j<som_size_y; j++)
			for(int i=0; i<som_size_x; i++)
				for (int rj=0; rj<rf_size_y; rj++)
					for (int ri=0; ri<rf_size_x; ri++)
							fprintf(f, "%.8f\n", w[j][i][ri][rj]);
		fclose(f);
	}
	
    // destroy w here ***

    destroy_matrix(stat_histogram);
    destroy_matrix(activity);
    destroy_matrix(learning_activity);
    destroy_matrix(arbor);
    
    destroy_matrix(activity_a);
    destroy_matrix(w_a);
}



static inline float
N(int topology, int neighborhood, int i, int j, int mx, int my, int size_x, int size_y, float sigma)
{
    float dx = i-mx;
    float dy = j-my;

    if(topology==1) // torus
    {
        dx = min(ikaros::abs(dx), min(ikaros::abs(dx-size_x), ikaros::abs(dx+size_x)));
        dy = min(ikaros::abs(dy), min(ikaros::abs(dy-size_y), ikaros::abs(dy+size_y)));
    }

    switch(neighborhood)
    {
        case 0: return gaussian1(ikaros::hypot(dx, dy), sigma);
        case 1: return dx*dx+dy*dy < sigma*sigma ? 1 : 0;
        case 2: return ikaros::abs(dx)+ikaros::abs(dy) < sigma ? 1 : 0;
        case 3: return dx*dx+dy*dy < sigma*sigma ? 1/((1+dx*dx+dy*dy)/sigma) : 0;   // ogenomtänkt men ok tror jag
        default: return 0;
    }
}



void
CSOM_L::CalculateActivation()
{
    if(rf_inc_x == 1 && rf_inc_y == 1) // inc == 1: do fast convolution (GCD?)
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
                    //    float s = 0.0;
                        float s = -stat_distribution[sj][si];    // FIXME: CONSCIENCE
                        for (int rj=0; rj<rf_size_y; rj++)
                            for (int ri=0; ri<rf_size_x; ri++)
                                s += w[sj][si][rj][ri] * input[rf_inc_y*mj+rj][rf_inc_x*mi+ri]; // * arbor[rj][ri]; // FIXME: ARBOR +/- *****
     
                        activity[sj][si][mj][mi] = s;
                    }
    }
    
//    multiply(activity[0][0][0], 1.0/float(rf_size_x*rf_size_y), som_size_x*som_size_y*map_size_x*map_size_y); // scale with rf-size; make output independent of rf-size
}



void
CSOM_L::CalculateLearningActivity()   // STANDARD SOM-VERSION
{
    for(int l=0; l<som_size_y; l++)
        for(int k=0; k<som_size_x; k++)
            reset_matrix(learning_activity[l][k], map_size_x, map_size_y);  // OPTIMIZE: Reset all without loops
            
    float _sigma = radius*sigma;
    for(int mj=0; mj<map_size_y; mj++)  // OPTIMIZE: GCD???
        for(int mi=0; mi<map_size_x; mi++)
        {
            int max_i = 0;    // OPTIMIZE: max_xy() -> max of array -> (x, y) calculation
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
            
            // set neighborhood OPTIMIZE: Calculate N once and add from matrix

            for(int sj=0; sj<som_size_y; sj++)
                for(int si=0; si<som_size_x; si++)
                     learning_activity[sj][si][mj][mi] = N(topology, 0, si, sj, max_i, max_j, som_size_x, som_size_y, _sigma);
                
             stat_histogram[max_j][max_i] += 1.0; // statistics: update histogram
             float last_error = *error;
             *error = (1.0f-0.001f)*(*error) + 0.001f*activity[max_j][max_i][0][0];
             *progress =  (1.0f-0.001f)*(*progress) + 0.001 * (*error-last_error);
//             printf("progress = %f\n", *progress);
        }
        
//    copy_matrix(stat_distribution, stat_histogram, som_size_x, som_size_y);
//    normalize(stat_distribution, som_size_x, som_size_y);
}



void
CSOM_L::UpdateConstants()
{
    alpha = clip(alpha*alpha_decay, alpha_min, alpha_max);
    sigma = clip(sigma*sigma_decay, sigma_min, sigma_max);
}


/* // UNOPTIMIZED VERSION - MUST BE CINLUDED FOR COMPATIBILITY

void
CSOM_L::UpdateWeights() // apply RF learning rule (normalized Hebbian learning) - // TODO: OPTIMIZE: collapse one dimension
{
    for(int l=0; l<map_size_y; l++)
        for(int k=0; k<map_size_x; k++)
        
            for(int j=0; j<som_size_y; j++)
                for(int i=0; i<som_size_x; i++)
                {
                    float ** wp = w[j][i];
                    float a = alpha * learning_activity[j][i][l][k];
                    for(int y=0; y<rf_size_y; y++)
                    {
                        float * wpp = wp[y];
                        float * ip  = input[rf_inc_y*l+y];
                        float * ipk = &ip[rf_inc_x*k];
                        float * arb   = arbor[y];
                        for(int x=0; x<rf_size_x; x++)
                        {
                            (*wpp++) +=  a * (*ipk++) * (*arb++); 
                        }
                    }
                }

    for(int j=0; j<som_size_y; j++)  // OPTIMIZE: GCD
        for(int i=0; i<som_size_x; i++)
            normalize(w[j][i], rf_size_x, rf_size_y);
}
*/



void
CSOM_L::UpdateWeights() // apply RF learning rule (normalized Hebbian learning) - // TODO: OPTIMIZE: collapse one dimension
{
    float ** input_buffer = create_matrix(rf_size_x, rf_size_y);
    int rf_size = rf_size_x*rf_size_y;
    float learning_rate = alpha/float(map_size_x*map_size_y);   // divide by number of repeated trials from weight sharing

	if(use_arbor)
    for(int l=0; l<map_size_y; l++)
        for(int k=0; k<map_size_x; k++)
        {
             // fetch input to temporary (linear) buffer
        
            int rl = rf_inc_y*l;
            int rk = rf_inc_x*k;
            for(int y=0; y<rf_size_y; y++)
                for(int x=0; x<rf_size_x; x++)
                    input_buffer[y][x] = input[rl+y][rk+x] * arbor[y][x]; 
            
            // add to weight

            for(int j=0; j<som_size_y; j++)
                for(int i=0; i<som_size_x; i++)
                {
                    float a = learning_rate * learning_activity[j][i][l][k];
                    vDSP_vsma( *input_buffer,           //  const float *A,
                               1,                       //  vDSP_Stride I,
                               &a,                      //  const float *B,
                               w[j][i][0],              //  const float *C,
                               1,                       //  vDSP_Stride K,
                               w[j][i][0],              //  float *D,
                               1,                       //  vDSP_Stride L,
                               rf_size);                //  vDSP_Length N
                }

        }
	
	else
		for(int l=0; l<map_size_y; l++)
			for(int k=0; k<map_size_x; k++)
			{
				// fetch input to temporary (linear) buffer
				
				int rl = rf_inc_y*l;
				int rk = rf_inc_x*k;
				for(int y=0; y<rf_size_y; y++)
					for(int x=0; x<rf_size_x; x++)
						input_buffer[y][x] = input[rl+y][rk+x]; // * arbor[y][x]; 
				
				// add to weight
				
				for(int j=0; j<som_size_y; j++)
					for(int i=0; i<som_size_x; i++)
					{
						float a = learning_rate * learning_activity[j][i][l][k];
						vDSP_vsma( *input_buffer,           //  const float *A,
								  1,                       //  vDSP_Stride I,
								  &a,                      //  const float *B,
								  w[j][i][0],              //  const float *C,
								  1,                       //  vDSP_Stride K,
								  w[j][i][0],              //  float *D,
								  1,                       //  vDSP_Stride L,
								  rf_size);                //  vDSP_Length N
					}
				
			}
	
	
    for(int j=0; j<som_size_y; j++)  // OPTIMIZE: GCD
        for(int i=0; i<som_size_x; i++)
            normalize(w[j][i], rf_size_x, rf_size_y);
            
    destroy_matrix(input_buffer);
}



//
// Visualization functions
//

void
CSOM_L::GenerateOutput()  // Rearrange activity for output output [Copy activity to output [use fast copy to submatrix later]]
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
CSOM_L::CalculateSalience()
{
	reset_matrix(salience, map_size_x, map_size_y);
	for(int j=0; j<som_size_y; j++)
		for(int i=0; i<som_size_x; i++)
			for(int l=0; l<map_size_y; l++)
				for(int k=0; k<map_size_x; k++)
					salience[l][k] += activity[j][i][l][k];
}



void
CSOM_L::GenerateColorCodedOutput()
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
CSOM_L::GenerateWeightOutput()
{
    for(int j=0; j<som_size_y; j++)
        for(int i=0; i<som_size_x; i++)
            for(int l=0; l<rf_size_y; l++)
                for(int k=0; k<rf_size_x; k++)
                    weights[j*rf_size_y+l][i*rf_size_x+k] = w[j][i][l][k];
}



//
//  Associations
//
/*
void
CSOM_L::CalculateAssociations()
{
    reset_matrix(activity_a, assoc_size_x, assoc_size_y, som_size_x, som_size_y);
    
    // Repeat associations for each SOM

    for(int mj = 0; mj < map_size_y; mj++)
        for(int mi = 0; mi < map_size_x; mi++)
        {
            // Repeat associations to each element in the SOM
            // Check that associations are within the map

            for(int sj=0; sj<som_size_y; sj++)
                for(int si=0; si<som_size_x; si++)
                    {
                        // Sum up the associations from the other nodes
                        
                        for(int m_j = 0; m_j < map_size_y; m_j++)
                            for(int m_i = 0; m_i < map_size_x; m_i++)
                                for(int s_j=0; s_j<som_size_y; s_j++)
                                    for(int s_i=0; s_i<som_size_x; s_i++)
                        
                                        activity_a[sj][si][mj][mi] += activity[s_j][s_i][m_j][m_i];
                    }
        }
}



void
CSOM_L::UpdateAssociationWeights()
{

}
*/


//
// Tick
//

void
CSOM_L::Tick()
{
    // Categorization Layer

    CalculateActivation();
    CalculateLearningActivity();
    UpdateConstants();
    UpdateWeights();
    GenerateOutput();
	CalculateSalience();

    // Association Layer

//    CalculateAssociations();
//    UpdateAssociationWeights();
    
    // Outputs

    GenerateColorCodedOutput();
    GenerateWeightOutput();
}


static InitClass init("CSOM_L", &CSOM_L::Create, "Source/Modules/ANN/CSOM/CSOM_L/");
