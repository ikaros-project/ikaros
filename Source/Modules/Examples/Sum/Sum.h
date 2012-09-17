//
//	Sum.h		This file is a part of the IKAROS project
// 				Old example for version 1.1 and earlier.
//
//    Copyright (C) 2001-2007 Jan Moren
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

#ifndef SUM
#define SUM

#include "IKAROS.h"

class Sum: public Module
{

public:

    // This is all boilerplate code to declare the needed methods for
    // initialization of the module. Just change 'Sum' to whatever name your
    // module has.

    Sum(Parameter * p);
    virtual ~Sum();

    static Module * Create(Parameter * p);

    void SetSizes();

    void Init();
    void Tick();

    // Declare some variables.

    int     theNoOfInputs1;
    int     theNoOfInputs2;
    int     theNoOfOutputs;

    // the input and output vectors. The storage is declared and handled by
    // Ikaros so you do not have to.

    float * input1;
    float * input2;
    float * output;
};

#endif
