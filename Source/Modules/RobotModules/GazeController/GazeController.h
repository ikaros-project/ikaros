//
//	  GazeController.h		This file is a part of the IKAROS project
//
//    Copyright (C) 2014 Christian Balkenius
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

#ifndef GazeController_
#define GazeController_

#include "IKAROS.h"

class GazeController: public Module
{
public:
    static Module * Create(Parameter * p) { return new GazeController(p); }

    GazeController(Parameter * p) : Module(p) {}
    virtual ~GazeController();

    void 		Init();
    void 		Tick();

    float       A;
    float       B;
    float       C;
    float       D;
    float       E;
    
    float *     offset; // TODO: These could possibly be moved to Dynamixel class
    float       center_override;
    
    float *     target;
    float       target_override;

    float       gamma;
    
    float *     input;
    float *     output;
    
    float **    view_side;
    float **    view_top;
	
	int         angle_unit;

};

#endif

