//
//	MedianFilter.h    This file is a part of the IKAROS project
//	
//
//    Copyright (C) 2013  Christian Balkenius
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

#ifndef MedianFilter_
#define MedianFilter_

#include "IKAROS.h"



class MedianFilter: public Module
{
public:

    MedianFilter(Parameter * p) : Module(p) {};
    virtual ~MedianFilter();

    static Module * Create(Parameter * p) { return new MedianFilter(p); };

    void    SetSizes();
    void    ComputeKernel();
    
    void    Init();
    void    Tick();

    int     size_x;
    int     size_y;
    int     kernel_size;

    float **	input;
    float **	output;
    float  *    buffer;
};


#endif
