//
//    MPIFaceDetector.h		This file is a part of the IKAROS project
// 							Wrapper for MPISearch and MPIEyeFinder from the Machine Perception Toolbox
//
//    Copyright (C) 2009-2012 Christian Balkenius
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
//	Created: 2009-01-27
//

#ifndef _MPIFaceDetector_
#define _MPIFaceDetector_

#include "IKAROS.h"


class MPIFaceDetector: public Module
    {
    public:
        MPIFaceDetector(Parameter * p) : Module(p) {}
        virtual ~MPIFaceDetector() {}
        
        static Module * Create(Parameter * p) { return new MPIFaceDetector(p); }
        
        float **    input;
        float **    output;
        
        float **    face_table;
        float **    face;
        float **    eye_left;
        float **    eye_right;

        float *    face_position;
        float *    eye_left_position;
        float *    eye_right_position;

        int         size_x;
        int         size_y;
        
        void 		Init();
        void 		Tick();
    };

#endif
