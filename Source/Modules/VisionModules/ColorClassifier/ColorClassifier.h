//
//	  ColorClassifier.h		This file is a part of the IKAROS project
//
//
//    Copyright (C) 2005-2007 Christian Balkenius
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

#ifndef COLORCLASSIFIER
#define COLORCLASSIFIER

#include "IKAROS.h"

class ColorClassifier: public Module
{
public:
    ColorClassifier(Parameter * p) : Module(p) {}
	virtual			~ColorClassifier();

	static Module *	Create(Parameter * p) { return new ColorClassifier(p); }

    void            InitColorSpace();
    void			Init();

	void			Tick_Slow();
	void			Tick_Fast();
	void			Tick();

 	float **	r;
	float **	g;
	float **	I;
	float **	r_prim;
	float **	g_prim;

    float **    Dm;
    
	float **	output;

	float **	colorspace_store_r;
	float **	colorspace_store_g;
	float **	colorspace_store_b;

	float **	colorspace_r;
	float **	colorspace_g;
	float **	colorspace_b;

	int         size_x;
	int         size_y;

	float		color;
	float		width;
	float		saturation_min;
	float		saturation_max;

	float		cr;
	float		cg;

	bool		compensation;
    bool        diagnostics;
};

#endif
