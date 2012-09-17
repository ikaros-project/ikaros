//
//	ImageConvolution.h		This file is a part of the IKAROS project
//						A module to filter image with a kernel
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

#ifndef IMAGECONVOLUTION
#define IMAGECONVOLUTION

#include "IKAROS.h"



class ImageConvolution: public Module
{
public:

    ImageConvolution(Parameter * p);
    virtual ~ImageConvolution();

    static Module * Create(Parameter * p);

    void	SetSizes();
    void Init();
    void Tick();

    int		inputsize_x;
    int		inputsize_y;

    int		outputsize_x;
    int		outputsize_y;

    int		filtersize_x;
    int		filtersize_y;

    float **	input;
    float **	output;
    float **	filter;

    float		scale;
    float		bias;
    bool		rectify;

    bool		init_ok;
};


#endif
