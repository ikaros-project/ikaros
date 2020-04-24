//
//	BlackBox.h		This file is a part of the IKAROS project
//
//    Copyright (C) 2012 <Author Name>
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
//    See http://www.ikaros-project.org/ for more information.
//

#ifndef BlackBox_
#define BlackBox_

#include "IKAROS.h"

class BlackBox: public Module
{
public:
    static Module * Create(Parameter * p) { return new BlackBox(p); }

    BlackBox(Parameter * p);
    virtual ~BlackBox();
    void SetSizes();

    void 		Init();
    void 		Tick();

    // pointers to inputs and outputs
    // and integers to represent their sizes

    char **     input_name;
    char **     output_name;
    float ***   input;
    float ***   output;
    int         size_x;
    int         size_y;
    int         ins;
    int         outs;
    
    // internal data storage
    float *     internal_array;

    // parameter values

    float       parameter;
	bool       	debugmode;
};

#endif
