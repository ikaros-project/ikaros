//
//	PIDController.h		This file is a part of the IKAROS project
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
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#ifndef PIDCONTROLLER
#define PIDCONTROLLER

#include "IKAROS.h"

class PIDController: public Module 
{
public:
    // Control constants
    
    float       Kb;     // Bias (CO bias)
	float		Kp;		// Proportional Gain
	float		Ki;		// Intergral Gain
	float		Kd;		// Differential Gain

    // Signal filters
   
    float       Fs;     // Set point filter
    float       Fm;     // Measurement filter (PB filter)
    float		Fp;		// Error filter for Proportional Gain
	float		Fi;		// Error filter for Intergral Gain
	float		Fd;		// Error filter for Differential Gain
    float       Fc;     // Control filter (CO filter)
    
    // Min/Max Output
    
    float       Cmin;
    float       Cmax;

	int		size;		// Size of the inputs
	
	float *	input;		// Current state of the system
	float *	input_last;	// Last state of the system
	float *	set_point;  // Desired set point

	
    float *	filtered_set_point;
    float *	filtered_input;
    float *	filtered_error_p;
    float *	filtered_error_i;
    float *	filtered_error_d;

    float *	output;		// Control output
	float *	delta;		// Current state error
	float *	integral;   // The integrated error

    static Module * Create(Parameter * p) { return new PIDController(p); }

	PIDController(Parameter * p) : Module(p) {}
	virtual ~PIDController();
		
	void		Init();
	void		Tick();
};

#endif

