//
//	CircleDetector3 .h			This file is a part of the IKAROS project
//							
//
//    Copyright (C) 2002  Christian Balkenius
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

#ifndef CIRCLEDETECTOR3
#define CIRCLEDETECTOR3

#include "IKAROS.h"



class CircleDetector3 : public Module
{
public:

	CircleDetector3 (char * name, Parameter * p);
	virtual ~CircleDetector3 ();
	
	static Module * Create(char * name, Parameter * p);

	void		SetSizes();
	void 		Init();
	
	void 		Tick();

	float		threshold;

	int		inputsize_x;
	int		inputsize_y;
	
	int		outputsize_x;
	int		outputsize_y;
	
	float **	input;
	float	**	output;
	float	**	image;
	float **	dx;
	float **	dy;
	float **	g;
	float **	edge;
	float **	accumulator;
	
	float	**	dGx;	// Derivate gaussian filters
	float	**	dGy;
	
	float	*	hist_dx;
	float	*	hist_x;
	
	float		estimated_x;
	float		estimated_y;
};


#endif
