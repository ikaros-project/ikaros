//
//	SaliencyMap.cc		This file is a part of the IKAROS project
//						A generic attention map
//
//    Copyright (C) 2005-2007  Christian Balkenius
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

//	Normalize everything for absolute area
//	Coordinates between 0-1 everywhere

//      2006-01-20  Heavy optimization - now 100 times faster - CBA


#include "SaliencyMap.h"

using namespace ikaros;

#include "ctype.h" // TODO: remove when GetArrayValue is used


Module *
SaliencyMap::Create(Parameter * p)
{
    return new SaliencyMap(p);
}



SaliencyMap::SaliencyMap(Parameter * p):
        Module(p)
{
    spatial_bias		= NULL;
    input				= NULL;
    focus				= NULL;
    estimation			= NULL;
    initial_gain		= NULL;

    // Add the requested number of inputs ** function for Module::AddMultipleInputs(name, n) return n

    no_of_inputs		= GetIntValue("no_of_inputs", 1);

    for (int i=0; i<no_of_inputs; i++)
    {
        char nm[32];
        sprintf(nm, "INPUT%d", i);
        AddInput(create_string(nm));		// FIXME: Important to allocate new string here!!! ***
    }

    AddInput("REINFORCEMENT");
    AddInput("SPATIAL_BIAS");

    AddOutput("GAIN", no_of_inputs);
    AddOutput("SALIENCE");
    AddOutput("FOCUS", 2);
    AddOutput("ESTIMATION", 1);

    integration_radius		= GetIntValue("integration_radius");
    reinforcement_delay	= GetIntValue("reinforcement_delay");
    type					= GetIntValueFromList("type");
    alpha				= GetFloatValue("alpha");
    temperature			= GetFloatValue("temperature");

    // Set beta constants or read them from file if available

    beta = create_array(no_of_inputs);	// Get values from XML later

    for (int i=0; i<no_of_inputs; i++)
        beta[i] = 1.0;

    const char * beta_constants = GetValue("beta");
    if (beta_constants != NULL)
    {
        for (int i=0; i<no_of_inputs; i++)
        {
            for (; isspace(*beta_constants) && *beta_constants != '\0'; beta_constants++) ;
            if (sscanf(beta_constants, "%f", &beta[i])==-1)
            {
                Notify(msg_warning, "Too few constants (0 assumed).\n");
                beta[i] = 0;
            }
            for (; !isspace(*beta_constants) && *beta_constants != '\0'; beta_constants++) ;
        }
    }

    // Read initial weights

    const char * iw = GetValue("gain");
    if (iw != NULL)
    {
        initial_gain = create_array(no_of_inputs);

        for (int i=0; i<no_of_inputs; i++)
        {
            for (; isspace(*iw) && *iw != '\0'; iw++) ;
            if (sscanf(iw, "%f", &initial_gain[i])==-1)
            {
                Notify(msg_warning, "Too few constants (0 assumed).\n");
                initial_gain[i] = 0;
            }
            for (; !isspace(*iw) && *iw != '\0'; iw++) ;
        }
    }
}



void
SaliencyMap::SetSizes()
{
    int sx = GetInputSizeX("INPUT0");
    int sy = GetInputSizeY("INPUT0");
    if (sx != unknown_size && sy != unknown_size)
    {
        SetOutputSize("SALIENCE", sx-2*integration_radius, sy-2*integration_radius);
        SetOutputSize("OUTPUT", sx-2*integration_radius, sy-2*integration_radius);
    }
}



void
SaliencyMap::Init()
{
    input_size_x	 	= GetInputSizeX("INPUT0");
    input_size_y	 	= GetInputSizeY("INPUT0");

    output_size_x	 	= GetOutputSizeX("SALIENCE");
    output_size_y	 	= GetOutputSizeY("SALIENCE");

    input				= new float ** [no_of_inputs];
    input_store			= new DelayLine * [no_of_inputs];

    if (InputConnected("SPATIAL_BIAS"))
        spatial_bias = GetInputArray("SPATIAL_BIAS");

    gain				= GetOutputArray("GAIN");

    bool incorrect_sizes = false;
    for (int i=0; i<no_of_inputs; i++)
    {
        char name[32];
        sprintf(name, "INPUT%d", i);
        input[i] = GetInputMatrix(name);
        incorrect_sizes |= (input_size_x != GetInputSizeX(name) || input_size_y != GetInputSizeY(name));
        gain[i] = 1.0 / (float(no_of_inputs)*float(sqr(1+2*integration_radius)));	// Estimate of maximum if all channels are 1
        input_store[i] = new DelayLine(reinforcement_delay+1);
    }

    if (incorrect_sizes)
    {
        Notify(msg_fatal_error, "Inputs to module \"%s\" (SaliencyMap) have different sizes:\n", GetName());
        for (int i=0; i<no_of_inputs; i++)
        {
            char name[32];
            sprintf(name, "INPUT%d", i);
            Notify(msg_fatal_error, "Size of \"%s\" is %d x %d.\n", name, GetInputSizeX(name), GetInputSizeY(name));
        }
    }

    if (initial_gain != NULL)
        copy_array(gain, initial_gain, no_of_inputs);

    input_sum		= create_matrix(input_size_x, input_size_y);

    salience		= GetOutputMatrix("SALIENCE");
    focus			= GetOutputArray("FOCUS");

    estimation		= GetOutputArray("ESTIMATION");
    estimation_store= new DelayLine(reinforcement_delay+1);

    reinforcement	= GetInputArray("REINFORCEMENT");
}



SaliencyMap::~SaliencyMap()
{
    delete [] input;
    delete [] input_store;
    
    destroy_matrix(input_sum);
}



void
SaliencyMap::Tick()
{
    reset_matrix(input_sum, input_size_x, input_size_y);

    for (int n=0; n<no_of_inputs; n++)
        add(input_sum, gain[n], input[n], input_size_x, input_size_y);

    box_filter(salience, input_sum, output_size_x, output_size_y, 1+2*integration_radius, false);

    // SPATIAL BIAS (triangular)

    if (spatial_bias != NULL)
    {
        for (int j=0; j<output_size_y; j++)
            for (int i=0; i<output_size_x; i++)
            {
                float d = hypot(float(j)-float(output_size_y)*spatial_bias[1], float(i)-float(output_size_x)*spatial_bias[0]);
                salience[j][i] *= 100.0/(100.0+d);
            }
    }

    clip(salience, 0.0f, 10.0f, output_size_x, output_size_y);

    int mx = output_size_x/2;
    int my = output_size_y/2;

    if (type == 0)
    {
        arg_max(mx, my, salience, output_size_x, output_size_y);	// Coordinate in input (pixels)
    }
    else if (type == 1)
    {
        select_boltzmann(mx, my, salience, output_size_x, output_size_y, temperature);	// Coordinate in input (pixels)
        ascend_gradient(mx, my, salience, output_size_x, output_size_y);
    }
    else // type = 2 ERROR
    {
        printf("SaliencyMap: unimplemented - select exponential\n");
//		SelectExponential(mx, my, salience, output_size_x, output_size_y, temperature);	// Coordinate in input (pixels) ***
//		AscendGradient(mx, my, salience, output_size_x, output_size_y);
    }

    focus[0] = float(mx+0.5f*(input_size_x-output_size_x)) / float(input_size_x);	// Coordinate in input (0-1)
    focus[1] =  float(my+0.5f*(input_size_y-output_size_y)) / float(input_size_y);

    estimation[0] = salience[my][mx];

    if (reinforcement != NULL)
    {
        float delta = (reinforcement[0] - estimation_store->get());

        for (int n=0; n<no_of_inputs; n++)
            gain[n] += alpha * beta[n] * delta * input_store[n]->get();

        for (int n=0; n<no_of_inputs; n++)
            input_store[n]->put(input[n][my][mx]);

        estimation_store->put(estimation[0]);
    }
}

