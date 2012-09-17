//
//	OutputRawImage.h		This file is a part of the IKAROS project
//								A module for writing raw image data to files
//
//    Copyright (C) 2001-2011  Christian Balkenius
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

#ifndef OUTPUTIMAGE
#define OUTPUTIMAGE

#include "IKAROS.h"


class OutputRawImage: public Module {
public:
    int			cur_image;
    const char *		file_name;
    float			scale;
    int			supress;
    int			offset;

    int			size_x;
    int			size_y;

    float *		image;

    OutputRawImage(Parameter * p);
    virtual ~OutputRawImage();

    static Module * Create(Parameter * p);

    void Init();
    void Tick();

private:
    FILE	*	file;
};

#endif

