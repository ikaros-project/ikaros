//
//	MyModule.h		This file is a part of the IKAROS project
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

#ifndef TrainHead_
#define TrainHead_

#include "IKAROS.h"

class TrainHead: public Module
{
public:
    static Module * Create(Parameter * p) { return new TrainHead(p); }

    TrainHead(Parameter * p) : Module(p) {}
    virtual ~TrainHead();

    void 		Init();
    void 		Tick();

    int     t;
    int     segment_size;
    int     iteration;
    bool    first;

    float *    write;
    float *    head1;
    float *    head2;
    float *    out_movement;
    float *    past_movement;
    float *    now_movement;

    float *    mean_value;
    float *    variance_value;
};

#endif
