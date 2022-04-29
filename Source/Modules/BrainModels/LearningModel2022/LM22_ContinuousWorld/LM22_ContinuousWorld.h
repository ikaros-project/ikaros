//
//	ContinuousWorld.h
//
//    Copyright (C) 2022 Christian Balkenius
///
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
 
#ifndef ContinuousWorld_
#define ContinuousWorld_


#include "IKAROS.h"

class ContinuousWorld: public Module
{
public:
    static Module * Create(Parameter * p) { return new ContinuousWorld(p); }

    ContinuousWorld(Parameter * p) : Module(p) {}
    virtual ~ContinuousWorld() {}

    float agent_angle;
    float agent_length;


int place_obstacles_at;

    // Visualization outputs
    matrix  agent; // as path
    matrix  obstacles;// as paths
    matrix obstacles_pos;

    bool    obstacle_present[2];

    float * obstacle; // closest
    matrix  goal;
    // outputs

    float * visible;
    float * object;
    float * reward;

    // inputs

    float * attend; // input 
    float * approach;

    void 		Init();
    void 		Tick();

};

#endif

