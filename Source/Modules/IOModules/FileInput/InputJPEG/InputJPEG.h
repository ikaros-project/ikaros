//
//	InputJPEG.h		This file is a part of the IKAROS project
//					A module for reading from JPEG files
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

#ifndef INPUTJPEG
#define INPUTJPEG

#include "IKAROS.h"

class InputJPEG: public Module
{
public:
    long	iterations;
    long	iteration;
    int	max_images;
    int	cur_image;

    bool	read_once;
    bool	first;

    const char *	file_name;

    int	size_x;
    int	size_y;

    float *	intensity;
    float *	red;
    float *	green;
    float *	blue;

    InputJPEG(Parameter * p);
    virtual ~InputJPEG();

    static Module * Create(Parameter * p);

    bool		GetImageSize(int & x, int & y);
    void		Init();
    void		Tick();
private:
    FILE	*	file;
};

#endif


