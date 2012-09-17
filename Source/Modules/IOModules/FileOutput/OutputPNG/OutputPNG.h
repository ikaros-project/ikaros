//
//	OutputPNG.h		This file is a part of the IKAROS project
//                  A module for writing PNG images to files
//
//    Copyright (C) 2007  Jan Moren
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

#ifndef OUTPUTPNG
#define OUTPUTPNG

#include "IKAROS.h"

#ifdef USE_LIBPNG

class OutputPNG: public Module {
    public:
	int	    cur_image;	
	const char *	    file_name;

//	float	    scale;
//	int	    quality;

	int	    supress;
	int	    offset;

	int	    size_x;
	int	    size_y;


	float * writesig;	// Writing or not
	float **    input_intensity;		// Connect this one

	float **    input_red;			// Or all of these
	float **    input_green;
	float **    input_blue;

	OutputPNG(Parameter * p);
	virtual ~OutputPNG();

	static Module * Create(Parameter * p);

	void	Init();
	void	Tick();

	void	WriteGrayPNG(FILE * file, float ** image);
	void	WriteRGBPNG(FILE * file, float ** r, float ** g, float ** b);

    private:
	FILE*	file;
};
#endif

#ifndef USE_LIBPNG

class OutputPNG {
    public:
	static Module * Create(Parameter * p)
	{
	    printf("This version of IKAROS does not support PNG files.\n");
	    return NULL;
	}
};

#endif

#endif

