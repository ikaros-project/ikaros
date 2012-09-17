//
//	OutputJPEG.h		This file is a part of the IKAROS project
//					A module for writing JPEG images to files
//
//    Copyright (C) 2005  Christian Balkenius
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

#ifndef OUTPUTJPEG
#define OUTPUTJPEG

#include "IKAROS.h"


class OutputJPEG: public Module
{
public:
    int			cur_image;
    const char *		file_name;

    float			scale;
    int			quality;

    int			supress;
    int			offset;

    int			size_x;
    int			size_y;


    float *		writesig;		// If you want to write selectively
    float **		input_intensity;		// Connect this one

    float **		input_red;			// Or all of these
    float **		input_green;
    float **		input_blue;

    OutputJPEG(Parameter * p);
    virtual ~OutputJPEG();

    static Module * Create(Parameter * p);

    void Init();
    void Tick();

    void	WriteGrayJPEG(FILE * filename, float ** image);
    void	WriteRGBJPEG(FILE * filename, float ** r, float ** g, float ** b);

private:
    FILE	*	file;
};

#endif

