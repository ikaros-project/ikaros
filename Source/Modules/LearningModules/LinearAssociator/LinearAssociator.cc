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
}



void
LinearAssociator::Init()
{
	Bind(alpha, "alpha");
	Bind(beta, "beta");

    mode = GetIntValueFromList("mode");

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

	memory_max = GetIntValue("memory_max");
	memory_used = 0;
	memory_training = GetIntValue("memory_training");
	memory_t_input = create_matrix(input_size, memory_max);
	memory_t_output = create_matrix(output_size, memory_max);

	error = GetOutputArray("ERROR");
	confidence = GetOutputArray("CONFIDENCE");
	learning_rate = GetInputArray("LEARNING", false);

	delta = create_array(output_size);
	momentum = create_array(output_size);
	
	matrix = GetOutputMatrix("MATRIX");

    if(mode==1 && memory_max==1)
    {
        Notify(msg_warning, "Mode is set to LMS but memory_max is 1.");
    }
}



void
LinearAssociator::Train(float * x, float * y)
{
    printf("%f \t %f -> \t %f\n", input[0], t_input[0], output[0]);
	multiply(output, matrix, x, input_size, output_size);
	subtract(delta, y, output, output_size);
				
	add(momentum, beta, momentum, (1.0f-beta), delta, output_size);
				
	// Learn [ add_outer(alpha, delta, t_input) ]
				
	for(int j=0; j<output_size; j++)
		for(int i=0; i<input_size; i++)
			matrix[j][i] += alpha * momentum[j] * x[i];
}



void
LinearAssociator::CalculateError()
{
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

	*confidence = 1 - *error;
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

    if(mode == 0) // gradient descent
    {
        for(int t=0; t<memory_training; t++)    // -1
            for(int mem=0; mem<memory_used; mem++)
                Train(memory_t_input[mem], memory_t_output[mem]);
	}

    else // LMS
    {
        mldivide(matrix, memory_t_input, memory_t_output, input_size, output_size, memory_used);
    }


    CalculateError();

	// Calculate normal output
	
	multiply(output, matrix, input, input_size, output_size);
}



static InitClass init("LinearAssociator", &LinearAssociator::Create, "Source/Modules/LearningModules/LinearAssociator/");

