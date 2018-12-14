//
//    DFaceDetector.cc	This file is a part of the IKAROS project
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

#include "DFaceDetector.h"

using namespace ikaros;
using namespace dlib;


void
DFaceDetector::Init()
{
    detector = get_frontal_face_detector();
    
    Bind(max_faces, "max_faces");

    input = GetInputMatrix("INPUT");
    size_x = GetInputSizeX("INPUT");
    size_y = GetInputSizeY("INPUT");
    
    face_position = GetOutputMatrix("FACE_POSITION");
    face_size = GetOutputMatrix("FACE_SIZE");
    face_bounds = GetOutputMatrix("FACE_BOUNDS");
    face_count = GetOutputArray("FACE_COUNT");
}


DFaceDetector::~DFaceDetector()
{
}


void
DFaceDetector::Tick()
{
    reset_matrix(face_bounds, 8, max_faces);
    reset_matrix(face_position, 2, max_faces);
    reset_matrix(face_size, 2, max_faces);

    array2d<unsigned char> img;
    dlib::assign_image(img, 255*mat(*input, size_y, size_x));

//    pyramid_up(img);

    std::vector<rectangle> dets = detector(img);

    for(int i=0; i<dets.size(); i++)
    {
        face_bounds[i][0] = dets[i].left() / float(size_x);
        face_bounds[i][1] = dets[i].top() / float(size_y);
        
        face_bounds[i][2] = dets[i].right() / float(size_x);
        face_bounds[i][3] = dets[i].top() / float(size_y);
        
        face_bounds[i][4] = dets[i].right() / float(size_x);
        face_bounds[i][5] = dets[i].bottom() / float(size_y);

        face_bounds[i][6] = dets[i].left() / float(size_x);
        face_bounds[i][7] = dets[i].bottom() / float(size_y);
        
        face_position[i][0] = (dets[i].left() + dets[i].right())/(2*float(size_x));
        face_position[i][1] = (dets[i].top() + dets[i].bottom())/(2*float(size_x));

        face_size[i][0] = (dets[i].right() - dets[i].left())/size_x;
        face_size[i][1] = (dets[i].bottom() - dets[i].top())/size_y;
    }

    *face_count = float(dets.size());
 }



static InitClass init("DFaceDetector", &DFaceDetector::Create, "Source/Modules/VisionModules/DFaceDetector/");

