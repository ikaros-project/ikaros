//
//    DFaceDetector.h		This file is a part of the IKAROS project
// 							
//
//    Copyright (C) 2018 Christian Balkenius
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

#ifndef _DFaceDetector_
#define _DFaceDetector_

#include "IKAROS.h"

#include <dlib/image_processing/frontal_face_detector.h>


class DFaceDetector: public Module
    {
    public:
        DFaceDetector(Parameter * p) : Module(p) {}
        virtual ~DFaceDetector();
        
        static Module * Create(Parameter * p) { return new DFaceDetector(p); }
        
        float       min_size;
        bool        use_tracking;
        bool        mouth_correction;
        bool        detect_smiles;
        bool        detect_blinks;

        float **    input;

        int         size_x;
        int         size_y;
        
        float *     face_count;

        float **    face_position;
        float **    face_size;
        float **    face_bounds;
        float **    eye_left_position;
        float **    eye_right_position;
        float **    mouth_position;
        float *     rotation;
        float *     smile;
        float *     blink_left;
        float *     blink_right;
        float *     novelty;
        float *     object_id;
        float *     life;

        float **     output;
        
        int         max_faces;

        dlib::frontal_face_detector detector;
    
        void 		Init();
        void 		Tick();
    };

#endif

