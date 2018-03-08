//
//      Autoassociator.cc		This file is a part of the IKAROS project
//				
//
//    Copyright (C) 2016 Christian Balkenius
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


#include "Autoassociator.h"

using namespace ikaros;

void
Autoassociator::Learn()
{
	for(int j=0; j<output_size; j++)
    	for(int i=0; i<input_size; i++)
        {
         	if(i!=j && (t_input[i]>0))
	        	w[i][j] += learning_rate_const * (2*t_input[j]-1) * (2*t_input[i]-1);
        }
    
    if(u)
    {
		for(int j=0; j<output_size; j++)
    		for(int i=0; i<aux_input_size; i++)
                if(i!=j && (aux_t_input[i]>0))
        		    u[i][j] += learning_rate_const * (2*t_input[j]-1) * (2*aux_t_input[i]-1);
	}
}



void
Autoassociator::Init()
{
    Bind(learning_rate_const, "learning_rate");
    Bind(depression_rate, "depression_rate");
    Bind(activation_gain_const, "activation_gain");
    Bind(noise_level, "noise_level");

    input = GetInputArray("INPUT");
    reset = GetInputArray("RESET");
    output = GetOutputArray("OUTPUT");

    t_input = GetInputArray("T-INPUT");
    t_output = GetOutputArray("T-OUTPUT");
    
    delayed_t_input = GetInputArray("DELAYED-T-INPUT");
    top_down_input = GetInputArray("TOP-DOWN-INPUT");

    top_down_error = GetInputArray("TOP-DOWN-ERROR");
    error_output = GetOutputArray("ERROR-OUTPUT");

    learning_gain = GetInputArray("LEARNING-GAIN");
    activation_gain = GetInputArray("ACTIVATION-GAIN");
    
    aux_input = GetInputArray("AUX-INPUT");
    aux_output = GetOutputArray("AUX-OUTPUT");
    aux_t_input = GetInputArray("AUX-T-INPUT");
    aux_t_output = GetInputArray("AUX-T-OUTPUT");

    input_size        = GetInputSize("INPUT");
    aux_input_size    = GetInputSize("AUX-T-INPUT");
    output_size        = GetOutputSize("OUTPUT");
    
    w = GetOutputMatrix("W"); //create_matrix(input_size, output_size);    // size(input) x size(output)
    u = GetOutputMatrix("U"); //create_matrix(aux_input_size, output_size);    // size(aux_input) x size(output)

    w_depression = GetOutputMatrix("W_DEPRESSION");
    u_depression = GetOutputMatrix("U_DEPRESSION");

    net = GetOutputArray("NET");
    energy = GetOutputArray("ENERGY");
}



static float f(float x, float gain)
{
// return (x*x)/(0.01+x*x);
// return 1/(1+exp(-10*x));

	if(x < 0)
    	return 0;
    else if(gain*x > 1)
    	return 1;
    else
    	return gain*x;
}



void
Autoassociator::Tick()
{
	Learn();
    
    // Resst
    
    if(reset && *reset>0)
        reset_array(net, output_size);

    // Calculate output
    
    float old[output_size];
    copy_array(old, output, output_size);

//    reset_array(net, output_size);

	for(int j=0; j<output_size; j++)
    {
    	for(int i=0; i<input_size; i++)
        {
        	net[j] += (1-w_depression[i][j])*w[i][j] * old[i];

	        if(old[i] > 0.5)
                w_depression[i][j] += depression_rate * (1-w_depression[i][j])*abs(w[i][j]);
            else
                w_depression[i][j] *= (1-depression_rate);
        }
        
    	for(int i=0; i<aux_input_size; i++)
        {
	        net[j] += (1-u_depression[i][j])*u[i][j] * aux_input[i];

            if(aux_input[i] > 0.5)
                u_depression[i][j] += depression_rate * (1-u_depression[i][j])*abs(u[i][j]);
            else
                u_depression[i][j] *= (1-depression_rate);
        }
        
        net[j] = clip(net[j], 0, 1);
        
        float input_inhibition = sum(input, input_size)/float(input_size);
        
        output[j] = f(net[j] + input[j] +  random(-noise_level, noise_level), activation_gain_const);
    }

// LATERAL INHIBITION
/*
	int mm = 0;
	for(int i=0; i<3; i++)
	{
	    if(output[i] > output[mm])
            mm = i;
	}
	
	output[0] = 0;
    output[1] = 0;
    output[2] = 0;
    output[mm] = 1;
*/
    // Calculate Energy

    float e = 0;
    for(int j=0; j<output_size; j++)
    	for(int i=0; i<input_size; i++)
        {
        	e += output[i]*output[j]*w[i][j];
        }

	*energy = -0.5*e;
}



static InitClass init("Autoassociator", &Autoassociator::Create, "Source/Modules/ANN/Autoassociator/");

