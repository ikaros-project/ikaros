//
//	MTPupilDemo.h		This file is a part of the IKAROS project
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

#ifndef MTPupilDemo_
#define MTPupilDemo_

#include "IKAROS.h"

class MTPupilDemo: public Module
{
public:
    static Module * Create(Parameter * p) { return new MTPupilDemo(p); }

    MTPupilDemo(Parameter * p) : Module(p) {}
    virtual ~MTPupilDemo();

    void 		Init();
    void 		Tick();

    // pointers to inputs and outputs
    // and integers to represent their sizes

    float *     id_array;
    int         id_array_size;

    float *     img_pos_array;
    int         img_pos_array_size;

    float *     output_array;
    int         output_array_size;

    float *     robot_output_array;
    int         robot_output_array_size;

    float *     eye_led_output_array;
    int         eye_led_output_array_size;
    
    Timer blinkTimer;
    float TimeToNextBlink = 0;

    Timer markerTimer;
    float trackPos[2];

};

#endif
