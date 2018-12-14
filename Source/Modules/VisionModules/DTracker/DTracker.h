//
//    DTracker.h		This file is a part of the IKAROS project
// 							
//
//    Copyright (C) 2017 Christian Balkenius
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

#ifndef _DTracker_
#define _DTracker_

#include "IKAROS.h"

//#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing.h>

class DTracker: public Module
    {
    public:
        DTracker(Parameter * p) : Module(p) {}
        virtual ~DTracker();
        
        static Module * Create(Parameter * p) { return new DTracker(p); }

        float **    input;

        int         size_x;
        int         size_y;
        
        float *     tracker_bounds;
        float *     tracker_position;

        dlib::correlation_tracker tracker;
        void 		Init();
        void 		Tick();

        // Command
        void        Command(std::string s, float x, float y, std::string value);
        bool        tracking = false;
    };

#endif

