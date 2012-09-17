//
//	InputRawImage.h	This file is a part of the IKAROS project
//					A module for reading from files with raw image data
//
//    Copyright (C) 2001-2002  Christian Balkenius
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

#ifndef INPUTRAWIMAGE
#define INPUTRAWIMAGE

#include "IKAROS.h"


class InputRawImage: public Module {
public:
    long		iterations;	// How many iterations of the whole sequence?
    long		iteration;
    int		repeats;		// How many repeats of each image?
    int		repeat;
    int		max_images;
    int		cur_image;

    const char *	file_name;

    int		size_x;
    int		size_y;


    float *	image;

    InputRawImage(Parameter * p);
    virtual ~InputRawImage();

    static Module * Create(Parameter * p);

    void Init();
    void Tick();
private:
    FILE	*	file;
};

#endif

