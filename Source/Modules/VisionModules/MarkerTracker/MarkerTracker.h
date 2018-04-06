//
//	MarkerTracker		This file is a part of the IKAROS project
//                          Wrapper module for ARToolKitPlus available at:
//                          https://launchpad.net/artoolkitplus
//
//    Copyright (C) 2011-2014 Christian Balkenius
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

#ifndef MarkerTracker_
#define MarkerTracker_


#include "IKAROS.h"

class MFTracker;

class MarkerTracker: public Module
{
public:
    
    MarkerTracker(Parameter * p) : Module(p) {}
    virtual ~MarkerTracker();
    
    static Module * Create(Parameter * p) { return new MarkerTracker(p); }
    
    void 		Init();
    void 		Tick();

    float **    matrix;
    float *     object_id;
    float *     frame_id;
    float *     confidence;
    float **    image_position;
    float **    edges;

    float **    markers;
    float *     marker_count;
    int         max_markers;
    
    float **    projection;
    float **    model_view;
    
    float       marker_size;
    float **    marker_sizes;
    int         marker_size_ranges;

    int         frame_id_constant;

    bool        sort_markers;
    bool        use_history;
    bool        auto_threshold;
    int         threshold;
    
    int         coordinate_system;
    
    float *     calibration;
    
    int         size_x;
    int         size_y;
    
    float **    input;
    int         max_positions;
    
    MFTracker     * tracker;
    unsigned char * buffer;
	
	int         distanceUnit;
};

#endif
