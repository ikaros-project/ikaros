//
//	M02_Cortex.h
//


//
// This module defines the M02_Cortex model descibed in
// Balkenius & Morén, Amygdala, CybSyst etc, 2001
//
// The model does absolutely nothing!!!
// It is simply a placeholder.
//
// Inputs
//
//    INPUT
//
// Outputs
//
//   OUTPUT	(outputsize = 4)
//
// The size of the output must be equal to the size of the input
//


#include "IKAROS.h"



class M02_Cortex: public Module
{
public:

	M02_Cortex(Parameter * p);
	virtual ~M02_Cortex();
	
	static Module * Create(Parameter * p);

	void Init();
	void Tick();

	int		theNoOfInputs;
	int		theNoOfOutputs;

	float *	input;
	float	*	output;
};



