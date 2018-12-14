//
//    DTracker.cc	This file is a part of the IKAROS project
//
//
//    Copyright (C) 2018 Birger Johansson
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

#include "DTracker.h"

using namespace ikaros;
using namespace dlib;


void
DTracker::Init()
{
    input = GetInputMatrix("INPUT");
    size_x = GetInputSizeX("INPUT");
    size_y = GetInputSizeY("INPUT");
    
    tracker_bounds = GetOutputArray("TRACKER_BOUNDS");
    tracker_position = GetOutputArray("TRACKER_POSITION");

    tracking = false;
}


DTracker::~DTracker()
{
}


void
DTracker::Tick()
{
    reset_array(tracker_bounds, 8);
    reset_array(tracker_position, 2);

    if (tracking)
    {
        array2d<unsigned char> img;
        dlib::assign_image(img, 255*mat(*input, size_y, size_x));
        tracker.update(img);
        dlib::drectangle pos = tracker.get_position();

        tracker_position[0] = (pos.left() + pos.right())/(2*float(size_x));
        tracker_position[1] = (pos.top() + pos.bottom())/(2*float(size_y));

        tracker_bounds[0] = pos.left() / float(size_x);
        tracker_bounds[1] = pos.top() / float(size_y);
        
        tracker_bounds[2] = pos.right() / float(size_x);
        tracker_bounds[3] = pos.top() / float(size_y);
        
        tracker_bounds[4] = pos.right() / float(size_x);
        tracker_bounds[5] = pos.bottom() / float(size_y);

        tracker_bounds[6] = pos.left() / float(size_x);
        tracker_bounds[7] = pos.bottom() / float(size_y);
    }    
 }


void
DTracker::Command(std::string s, float x, float y, std::string value)
{
    Notify(msg_debug, "Start tracking");
    array2d<unsigned char> img; // For region only.
    dlib::assign_image(img, 255*mat(*input, size_y, size_x));
    tracker.start_track(img, centered_rect(point(x*size_x,y*size_y), 40, 40));
    tracking = true;
}


static InitClass init("DTracker", &DTracker::Create, "Source/Modules/VisionModules/DTracker/");

