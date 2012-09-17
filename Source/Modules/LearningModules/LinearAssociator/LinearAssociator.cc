//
//	  LinearAssociator.cc	This file is a part of the IKAROS project
//
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
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//

#include "LinearAssociator.h"

using namespace ikaros;


LinearAssociator::~LinearAssociator()
{
	destroy_array(delta);
	destroy_matrix(m);
}


void
LinearAssociator::Init()
{
	alpha =	GetFloatValue("alpha", 0.1);
	beta =	GetFloatValue("beta", 0.1);

	// Test that INPUT and T-INPUT have the same sizes
	
	if(GetInputSize("INPUT") != GetInputSize("T-INPUT"))
	{
		Notify(msg_fatal_error, "INPUT and T-INPUT must have the same sizes. (%d != %d)", GetInputSize("INPUT"), GetInputSize("T-INPUT"));
		return;
	}
	
	input_size	= GetInputSize("INPUT");
	output_size	= GetOutputSize("OUTPUT");
	
	input  = GetInputArray("INPUT");
	t_input  = GetInputArray("T-INPUT");
	output = GetOutputArray("OUTPUT");
	t_output = GetInputArray("T-OUTPUT");

	memory_max = GetIntValue("memory_max", 1);
	memory_used = 0;
	memory_training = GetIntValue("memory_training", 1);
	memory_t_input = create_matrix(input_size, memory_max);
	memory_t_output = create_matrix(output_size, memory_max);

	error = GetOutputArray("ERROR");
	confidence = GetOutputArray("CONFIDENCE");
	learning_rate = GetInputArray("LEARNING", false);

	delta = create_array(output_size);
	momentum = create_array(output_size);
	
	m = create_matrix(input_size, output_size);
}



void
LinearAssociator::Train(float * x, float * y)
{
	multiply(output, m, x, input_size, output_size);
	subtract(delta, y, output, output_size);
				
	add(momentum, beta, momentum, (1.0f-beta), delta, output_size);
				
	// Learn [ add_outer(alpha, delta, t_input) ]
				
	for(int j=0; j<output_size; j++)
		for(int i=0; i<input_size; i++)
			m[j][i] += alpha * momentum[j] * x[i];
}



void
LinearAssociator::Tick() 
{
	// Add sample to memory
	
	if(memory_used < memory_max)
	{
		copy_array(memory_t_input[memory_used], t_input, input_size);
		copy_array(memory_t_output[memory_used], t_output, output_size);
		memory_used++;
	}
	else
	{
		int i = int(random(0, memory_max-1));
		copy_array(memory_t_input[i], t_input, input_size);
		copy_array(memory_t_output[i], t_output, output_size);
	}
	
	// Train whole data set 'memory_training' times
	
	for(int t=0; t<memory_training-1; t++)
		for(int mem=0; mem<memory_used; mem++)
			Train(memory_t_input[mem], memory_t_output[mem]);
	
	// Calculate errors on last iteration over the memory set

	*error = 0;
	if(memory_training>0 && memory_used>0)
	{
		for(int mem=0; mem<memory_used; mem++)
		{
			Train(memory_t_input[mem], memory_t_output[mem]);
			*error += norm(delta, output_size);
		}
		*error /= float(memory_used);
	}
	*confidence = 1- *error;

	// Calculate normal output
	
	multiply(output, m, input, input_size, output_size);
}


