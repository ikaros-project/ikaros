//
//	PIDController.cc	This file is a part of the IKAROS project
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
//

#include "PIDController.h"

using namespace ikaros;

FILE * f = NULL;


PIDController::~PIDController()
{
	destroy_array(input_last);
    
    if(f) fclose(f);
}



void PIDController::Init()
{
    Bind(Kb, "Kb");
    Bind(Kp, "Kp");
    Bind(Ki, "Ki");
    Bind(Kd, "Kd");
    
    Bind(Fs, "Fs");
    Bind(Fm, "Fm");
    Bind(Fp, "Fp");
    Bind(Fi, "Fi");
    Bind(Fd, "Fd");
    Bind(Fc, "Fc");

    Bind(Cmin, "Cmin");
    Bind(Cmax, "Cmax");

	size			= GetInputSize("INPUT");
    
	input			= GetInputArray("INPUT");
	set_point		= GetInputArray("SETPOINT");
	output          = GetOutputArray("OUTPUT");
	delta			= GetOutputArray("DELTA");
	
    filtered_set_point  = GetOutputArray("FILTERED_SETPOINT");
    filtered_input      = GetOutputArray("FILTERED_INPUT");
    filtered_error_p    = GetOutputArray("FILTERED_ERROR_P");
    filtered_error_i    = GetOutputArray("FILTERED_ERROR_I");
    filtered_error_d    = GetOutputArray("FILTERED_ERROR_D");
    
	input_last		= create_array(size);
	integral		= GetOutputArray("INTEGRAL");
}



void PIDController::Tick()
{	
	for(int i=0; i<size; i++)
    {
        filtered_set_point[i]   = (1-Fs)*set_point[i]+(Fs)*filtered_set_point[i];
        filtered_input[i]       = (1-Fm)*input[i]+(Fm)*filtered_input[i];
		delta[i]                = filtered_set_point[i]- filtered_input[i];
        filtered_error_p[i]     = (1-Fp)*delta[i]+(Fp)*filtered_error_p[i];
        filtered_error_i[i]     = (1-Fi)*delta[i]+(Fi)*filtered_error_i[i];
        filtered_error_d[i]     = (1-Fd)*delta[i]+(Fd)*filtered_error_d[i];

		integral[i]             += filtered_error_i[i];
	
        float co = Kb + Kp*filtered_error_p[i] + Ki*integral[i] + Kd*(filtered_input[i] - input_last[i]);
        
        if(co > Cmax)
        {
            co = Cmax;
            integral[i] -= filtered_error_i[i];
        }
        else if(co < Cmin)
        {
            co = Cmin;
            integral[i] -= filtered_error_i[i];
        }
        
		output[i] = (1-Fc) * co + (Fc) * output[i];
    }

	copy_array(input_last, filtered_input, size);
}



