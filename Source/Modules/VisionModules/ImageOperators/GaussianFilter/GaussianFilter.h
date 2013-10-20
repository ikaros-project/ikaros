//
//	GaussianFilter.h    This file is a part of the IKAROS project
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

#ifndef GaussianFilter_
#define GaussianFilter_

#include "IKAROS.h"



class GaussianFilter: public Module
{
public:

    GaussianFilter(Parameter * p) : Module(p) {};
    virtual ~GaussianFilter() {}

    static Module * Create(Parameter * p) { return new GaussianFilter(p); };

    void    SetSizes();
    void    ComputeKernel();
    
    void    Init();
    void    Tick();

    float   sigma;
    float   sigma_last;
    
    int     kernel_size;
    int		size_x;
    int		size_y;

    float **	input;
    float **	output;
    float **    kernel;
    float *     kernel_profile;
};


#endif
