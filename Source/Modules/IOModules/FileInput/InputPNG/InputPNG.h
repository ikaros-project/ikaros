//
//	InputPNG.h	This file is a part of the IKAROS project
//				A module for reading PNG files
//
//    Copyright (C) 2007 Jan Moren
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

#ifndef INPUTPNG
#define INPUTPNG

#include "IKAROS.h"

#ifdef USE_LIBPNG

class InputPNG: public Module {
    public:
	long	iterations;
	long	iteration;
	int     max_images;
	int     cur_image;

	const char *	file_name;

	int	size_x;
	int	size_y;

	float *	intensity;
	float *	red;
	float *	green;
	float *	blue;

	InputPNG(Parameter * p);
	virtual ~InputPNG();

	static Module * Create(Parameter * p);

	void SetSizes();
	void Init();
	void Tick();							
};

#endif

#ifndef USE_LIBPNG

class InputPNG {
    public:
	static Module * Create(Parameter * p)
	{
	    printf("This version does not support PNG files.\n");
	    return NULL;
	}
};

#endif

#endif

