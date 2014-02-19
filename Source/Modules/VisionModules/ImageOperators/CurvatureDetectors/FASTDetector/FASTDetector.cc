//
//	FASTDetector.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2013  Christian Balkenius
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

#include "FASTDetector.h"

extern "C"
{
    #include "fast.h"
}

using namespace ikaros;

void
FASTDetector::Init()
{
    Bind(threshold, "threshold");
    
    input	= GetInputMatrix("INPUT");
    output	= GetOutputMatrix("OUTPUT");
    
    corners         = GetOutputMatrix("POINTS");
    corner_count    = GetOutputArray("POINT_COUNT");
    
    size_x  = GetInputSizeX("INPUT");
    size_y  = GetInputSizeY("INPUT");
}



void
FASTDetector::Tick()
{
    copy_matrix(output, input, size_x, size_y);
    
    unsigned char * data = new unsigned char [size_x*size_y];
    
    float_to_byte(data, *input, 0, 1, size_x*size_y);
    
    int numcorners;
    xy * points = fast9_detect_nonmax(data, size_x, size_y, size_x, threshold, &numcorners);
    
    reset_matrix(corners, 2, 5000);
    for(int i=0; i<numcorners; i++)
    {
        corners[i][0] = float(points[i].x)/float(size_x);
        corners[i][1] = float(points[i].y)/float(size_y);
    }
    
    *corner_count = float(numcorners);
    
    free(points);
    delete data;
}



static InitClass init("FASTDetector", &FASTDetector::Create, "Source/Modules/VisionModules/ImageOperators/CurvatureDetectors/FASTDetector/");

