//
//	black_box_vision.h		This file is a part of the IKAROS project
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


#ifndef black_box_vision_
#define black_box_vision_

#include "IKAROS.h"

#include <string>
#include <iostream>

class black_box_vision: public Module
{
public:
    static Module * Create(Parameter * p) { return new black_box_vision(p); }

    black_box_vision(Parameter * p) : Module(p) {}
    virtual ~black_box_vision();

    void 		Init();
    void 		Tick();
    
    
    int *   stimuliCounter;
    int     imageTickCounter;
    int     currentImage;
    
    // Parameters
    int numberOfObjects;
    
    // The black box knows everything.
    float ** knowedge;
    int knowedge_size_x;
    int knowedge_size_y;
    
    // Output image stimuli
    float ** s_intensity;
    int s_intensity_size_x;
    int s_intensity_size_y;
    
    // Output unrealistic model
    float * novelty;
    float * emotionPos;
    float * emotionNeg;
    float * objects;
    
    // JPEG
    bool ReadJPEGFromFile(char * filename, float ** intensity);
};

#endif

