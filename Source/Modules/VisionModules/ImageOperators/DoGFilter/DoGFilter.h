//
//	DoGFilter.h    This file is a part of the IKAROS project
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

#ifndef DoGFilter_
#define DoGFilter_

#include "IKAROS.h"



class DoGFilter: public Module
{
public:

    DoGFilter(Parameter * p) : Module(p) {};
    virtual ~DoGFilter() {}

    static Module * Create(Parameter * p) { return new DoGFilter(p); };

    void    SetSizes();
    void    ComputeKernel();
    
    void    Init();
    void    Tick();

    float   sigma1;
    float   sigma1_last;
    
    float   sigma2;
    float   sigma2_last;

    bool    normalize;
    
    int     kernel_size;
    int		size_x;
    int		size_y;

    float **	input;
    float **	output;
    float **    kernel;
    float *     kernel_profile;
};


#endif
