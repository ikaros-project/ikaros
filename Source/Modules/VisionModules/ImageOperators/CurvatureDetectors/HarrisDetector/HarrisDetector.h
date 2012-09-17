//
//	HarrisDetector.h		This file is a part of the IKAROS project
//						A module to estimate curvature in an image
//
//    Copyright (C) 2004  Christian Balkenius
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

#ifndef HARRISDETECTOR
#define HARRISDETECTOR

#include "IKAROS.h"


class HarrisDetector: public Module
{
public:

    HarrisDetector(Parameter * p);
    virtual ~HarrisDetector();

    static Module * Create(Parameter * p);

    void		SetSizes();
    void 		Init();
    void 		Tick();

    int			size_x;
    int			size_y;

    float **	input;		// original image; is this needed???
    float **	dx;			// gradient images
    float **	dy;

    float **	output;

    float ** 	dx2;
    float ** 	dy2;
    float ** 	dxy;
    float ** 	dxs;
    float ** 	dys;
    float ** 	dxys;
};


#endif
