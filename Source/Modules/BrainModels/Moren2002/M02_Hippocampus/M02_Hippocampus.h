//
//	M02_Hippocampus.h
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
// The module defines the model of context processing descibed in
// Balkenius & Morén, SAB, 2000
//
// Inputs
//
//    STIMULUS
//    LOCATION
//
// Outputs
//
//    BIND			(bindsize = 4)
//    CONTEXT		(contextsize = 4)
//    RESET
//



#include "IKAROS.h"

class M02_Hippocampus: public Module
{
public:

	M02_Hippocampus(Parameter * p): Module(p) {}
	virtual ~M02_Hippocampus();
	
	static Module * Create(Parameter * p) {return new M02_Hippocampus(p);}

	void Init();
	void Tick();

	int		theNoOfStimuli;
	int		theNoOfLocations;
	int		theNoOfBinds;
	int		theNoOfContexts;

	float 	*	input;
	float	*	location;
	float 	*	cs;
	float	*	bind;
	float	*	bind_last;
	float	*	bind_delta;
	float	*	context;
	float	*	reset;

	int			theUsedBind;
	int			theUsedContexts;
	
	bool		theNovelty;
	bool		theBindTrig;

	float **	W;

	int			theCurrentContext;

	float *		CXReset;
	float **	CXW;			// Weight for each binding node to the context
	
	float ***	CXMem;			// CX, Loc -> Obj
};



