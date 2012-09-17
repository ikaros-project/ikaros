//
// M02_Hippocampus.cc
//
//    Copyright (C) 2002, 2003 Christian Balkenius, Jan Moren
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
// 2009-06-20 Moved to Ikaros 1.2

#include "M02_Hippocampus.h"

using namespace ikaros;



// Neural network primitives

static int
categorize_or_store_euclidean(float * a, float ** m, int size_a, int & size_r, int max_r, float threshold, bool * new_category=NULL, float * match=NULL)
{
	float best_match = 0;
	int	best_index = 0;
	
	if(size_r > 0)
	{
		// Find closest prototype
		
		best_match = dist(a, m[0], size_a);
		best_index = 0;
		
		float t=0;
		for(int i=1; i<size_r; i++)
			if((t = dist(a, m[i], size_a)) < best_match)
			{
				best_match= t;
				best_index = i;
			}
        
		// Add new prototype if not closer than threshold
        
		if(new_category != NULL)
			*new_category = false;
	}
	
	if((best_match > threshold && size_r < max_r) || size_r == 0)
	{
		copy_array(m[size_r], a, size_a); // FIXME: Is this correct?
		best_index = size_r;
		best_match = 0;
		size_r++;
		if(new_category != NULL)
			*new_category = true;
	}
	
	if(match != NULL)
		*match = best_match;
    
	return best_index;
}



static bool
match_expectations(float * e, float * a, int size)
{
	for(int i=0; i<size; i++)
		if(e[i] > a[i])
			return true;
	return false;
}



// Matrix operations

static void
matrix_product(float * r, float * a, float ** m, int size_a, int size_r) // FIXME: use Ikaros_Math functions instead
{
	if(size_a < 1 || size_r < 1)
		return;
    
	for(int j=0; j<size_r; j++)
	{
		r[j] = 0;
		for(int i=0; i<size_a; i++)
			r[j] += a[i] * m[j][i];
	}
}



static void
copy_array_l(float * target, float * source, int size, int target_offset, int source_offset=0)
{
	for(int i = 0; i<size; i++)
		target[i+target_offset] = source[i+source_offset];
}



void
M02_Hippocampus::Init()
{
	theNoOfStimuli	 	= GetInputSize("STIMULUS");
	theNoOfLocations	= GetInputSize("LOCATION");
	theNoOfBinds		= GetOutputSize("BIND");
	theNoOfContexts     = GetOutputSize("CONTEXT");

	input				= GetInputArray("STIMULUS");
	location			= GetInputArray("LOCATION");
	cs                  = create_array(theNoOfStimuli+theNoOfLocations);
	bind				= GetOutputArray("BIND");
    bind_delta          = GetOutputArray("BIND_DELTA");
    bind_last           = create_array(theNoOfBinds);
	context             = GetOutputArray("CONTEXT");
	reset               = GetOutputArray("RESET");
	
	theUsedBind         = 0;
	theUsedContexts     = 1;
	theCurrentContext   = 0;

	theNovelty          = 0;
	theBindTrig         = 0;

	W                   = create_matrix(theNoOfBinds, theNoOfStimuli+theNoOfLocations);
	CXReset             = create_array(theNoOfContexts);	
	CXW                 = create_matrix(theNoOfBinds, theNoOfContexts);
	CXMem               = create_matrix(theNoOfStimuli, theNoOfLocations, theNoOfContexts);
}



M02_Hippocampus::~M02_Hippocampus()
{
	destroy_array(cs);
	destroy_matrix(W);
	destroy_array(CXReset);
	destroy_matrix(CXW);
	destroy_matrix(CXMem);
    destroy_array(bind_last);
}


void
M02_Hippocampus::Tick()
{	
	// Get input
	
	copy_array(cs, input, theNoOfStimuli);
	copy_array_l(cs, location, theNoOfLocations, theNoOfStimuli);
	
	reset[0] = 0;	// Reset reset output

	// Do nothing if there are no inputs

	if(norm1(cs, theNoOfStimuli+theNoOfLocations) == 0)
		return;
		
	int currentLocation = arg_max(location, theNoOfLocations);

	// BIND : Calculate binding nodes

	int best_bind = categorize_or_store_euclidean(cs, W, theNoOfStimuli+theNoOfLocations, theUsedBind, theNoOfBinds, 0.5, &theBindTrig);
	bind[best_bind] = 1.0;

	// MATCH : Check Working Memory

	theNovelty = match_expectations(CXMem[theCurrentContext][currentLocation], input, theNoOfStimuli);	// expectation > input => novelty

	// RESET: Reset Context

	if(theNovelty)
	{
//        printf("NOVELTY\n"); // TODO: remove
		reset[0] = 1;

		reset_array(context, theUsedContexts);
		theCurrentContext = -1;

		reset_array(bind, theUsedBind);
		bind[best_bind] = 1;
		
		if(theBindTrig) // Create new context
		{
//            printf("NEW CONTEXT\n"); // TODO: remove
			if(theUsedContexts == theNoOfContexts-1)
			{
				Notify(msg_fatal_error, "ERROR: M02_Hippocampus: Not enough context nodes. Increase and run again.");
			}
			else
			{
				theCurrentContext = theUsedContexts;
				copy_array(CXW[theCurrentContext], bind, theUsedBind);
				theUsedContexts++;
			}
		}
	}
	else
	{
		for(int j=0; j<theUsedBind; j++)
			CXW[theCurrentContext][j] = max(bind[j], CXW[theCurrentContext][j]);
	}
	
	// CONT : Calculate Context Nodes
	
	matrix_product(context, bind, CXW, theUsedBind, theUsedContexts);

	// CONT: Calculate Current Context
	
	theCurrentContext = arg_max(context, theUsedContexts);
	
	// CONT: Normalize Context Output
    
//    float temp;
//	temp = context[theCurrentContext];	
//	for(int i=0; i<theUsedContexts; i++)
//		context[i] = (context[i] / temp);

	for(int i=0; i<theUsedContexts; i++)
		context[i] = 0;
	context[theCurrentContext] = 1;

	// MEM : Store in Working Memory
    
	copy_array_l(CXMem[theCurrentContext][currentLocation], input, theNoOfStimuli, 0);
    
    // BIND_DELTA - added 2009 (CB)
    
    clip(subtract(bind_delta, bind, bind_last, theNoOfBinds), 0, 1, theNoOfBinds);
    copy_array(bind_last, bind, theNoOfBinds);
}


