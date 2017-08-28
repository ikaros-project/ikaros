//
//      LearningModule.cc		This file is a part of the IKAROS project
//				
//
//      Copyright (C) 2017 Christian Balkenius
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either version 2 of the License, or
//      (at your option) any later version.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//


#include "LearningModule.h"

using namespace ikaros;

void
LearningModule::Init()
{
    Bind(learning_rate, "learning_rate");

	input = GetInputArray("INPUT");
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

	input_size		= GetInputSize("INPUT");
	aux_input_size	= GetInputSize("AUX-T-INPUT");
	output_size		= GetOutputSize("OUTPUT");
    
	w = GetOutputMatrix("W"); //create_matrix(input_size, output_size);	// size(input) x size(output)
	u = GetOutputMatrix("U"); //create_matrix(aux_input_size, output_size);	// size(aux_input) x size(output)
}



void
LearningModule::Learn()
{
	for(int j=0; j<output_size; j++)
    	for(int i=0; i<input_size; i++)
        	w[j][i] += input[j] * input[i];
}



void
LearningModule::Tick()
{

}



static InitClass init("LearningModule", &LearningModule::Create, "Source/Modules/LearningModules/LearningModule/");


