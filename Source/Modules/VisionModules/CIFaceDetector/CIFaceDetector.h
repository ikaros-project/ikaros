//
//    CIFaceDetector.h		This file is a part of the IKAROS project
// 							
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

#ifndef _CIFaceDetector_
#define _CIFaceDetector_

#include "IKAROS.h"


class CIFaceDetector: public Module
    {
    public:
        CIFaceDetector(Parameter * p) : Module(p) {}
        virtual ~CIFaceDetector() {}
        
        static Module * Create(Parameter * p) { return new CIFaceDetector(p); }
        
        float       min_size;
        bool        use_tracking;
        
        float *     image_data;
        
        float **    input;
        float **    output;
        
        float **    face_table;
        float **    face;
        float **    eye_left;
        float **    eye_right;

        float *     face_position;
        float *     eye_left_position;
        float *     eye_right_position;

        float **    edges;
        float **    features;

        float *     face_count;
        float *     smile_count;

        int         max_faces;

        int         size_x;
        int         size_y;
        
        void 		Init();
        void 		Tick();
    };

#endif

