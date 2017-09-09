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
        	if(i!=j)
	        	w[j][i] += learning_rate * (2*t_input[j]-1) * (2*t_input[i]-1);
//            if (i==10 && j==11)
//            	printf("%f %f => %f\n", t_input[i], t_input[j], (2*t_input[j]-1) * (2*t_input[i]-1));
        }
    
//	clip(w, -1, 1, input_size, output_size);
    
    if(u)
    {
		for(int j=0; j<output_size; j++)
    		for(int i=0; i<aux_input_size; i++)
        		u[j][i] += learning_rate * (2*t_input[j]-1) * (2*aux_t_input[i]-1);

		clip(u, -1, 1, aux_input_size, output_size);
	}
    
/*
	for(int j=0; j<output_size; j++)
    	for(int i=0; i<input_size; i++)
        	w[j][i] += learning_rate * t_input[j] * t_input[i] * (1 - w[j][i]);

	for(int j=0; j<output_size; j++)
    	for(int i=0; i<aux_input_size; i++)
        	u[j][i] += learning_rate * t_input[j] * aux_t_input[i] * (1 - u[j][i]);
*/
}



void
Autoassociator::Init()
{
    LearningModule::Init();
    
    energy = GetOutputArray("ENERGY");
}



static float f(float x)
{
	return 1/(1+exp(-2*x));
}



void
Autoassociator::Tick()
{
//	print_array("input", t_input, input_size);

	Learn();
    
    if(GetTick() % 100 == 0)
    	reset_array(output, output_size);
    
    // Calculate output
    
    float old[output_size];
    copy_array(old, output, output_size);
	for(int j=0; j<output_size; j++)
    {
    	float net_j = 0;
        
    	for(int i=0; i<input_size; i++)
        	net_j += w[j][i] * old[i];
        
    	for(int i=0; i<aux_input_size; i++)
        	net_j += u[j][i] * aux_input[i];
        
        output[j] += 0.1 * (f(net_j + input[j]) - old[j]);
    }
    
    // Calculate energy
    
    float e = 0;
    for(int j=0; j<output_size; j++)
    	for(int i=0; i<input_size; i++)
        {
        	e += output[i]*output[j]*w[j][i];
        }
	*energy = -0.5*e;
}



static InitClass init("Autoassociator", &Autoassociator::Create, "Source/Modules/ANN/Autoassociator/");

