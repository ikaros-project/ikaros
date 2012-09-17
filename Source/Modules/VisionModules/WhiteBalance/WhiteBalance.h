//
//	WhiteBalance.h		This file is a part of the IKAROS project
//						A module to white balance an image
//
//    Copyright (C) 2003  Christian Balkenius
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

#ifndef WHITEBALANCE
#define WHITEBALANCE

#include "IKAROS.h"

class WhiteBalance: public Module
{
public:

    WhiteBalance(Parameter * p) : Module(p) {}
    virtual ~WhiteBalance() {}

    static Module * Create(Parameter * p) { return new WhiteBalance(p); }

    void Init();
    void Tick();

    int		size_x;
    int		size_y;

    float **	input0;
    float **	input1;
    float **	input2;

    float	**	output0;
    float	**	output1;
    float	**	output2;

    float		red_target;
    float		green_target;
    float		blue_target;

    int		reference_x0;
    int		reference_y0;
    int		reference_x1;
    int		reference_y1;

    int		log_x0;
    int		log_y0;
    int		log_x1;
    int		log_y1;
};


#endif
