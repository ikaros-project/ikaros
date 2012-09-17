//
// M02_Cortex.cc
// 2009-06-20 Moved to Ikaros 1.2
//


#include "M02_Cortex.h"

#include "IKAROS.h"



Module *
M02_Cortex::Create(Parameter * p)
{ 
	return new M02_Cortex(p);
}



M02_Cortex::M02_Cortex(Parameter * p):
	Module(p)
{
	AddInput("INPUT");
	AddOutput("OUTPUT", GetIntValue("outputsize", 4));

	input			= NULL;
	output			= NULL;
}



void
M02_Cortex::Init()
{
	theNoOfInputs	 	= GetInputSize("INPUT");
	theNoOfOutputs		= GetInputSize("OUTPUT");

	input				= GetInputArray("INPUT");
	output              = GetInputArray("OUTPUT");
}



M02_Cortex::~M02_Cortex()
{
}



void
M02_Cortex::Tick()
{
	// Get input
	
	copy_array(output, input, theNoOfOutputs);
}



