//
//	SparseFlow.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2021  Christian Balkenius
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

#include "SparseFlow.h"

using namespace ikaros;

void
SparseFlow::Init()
{
    Bind(magnification, "magnification");
    Bind(feature_radius, "feature_radius");
    Bind(feature_threshold, "feature_threshold");
    Bind(search_radius, "search_radius");

    input               = GetInputMatrix("INPUT");
    input_last  	    = GetInputMatrix("INPUT_LAST");

    points             = GetInputMatrix("POINTS");
    points_last        = GetInputMatrix("POINTS_LAST");

    points_count        = GetInputArray("POINTS_COUNT");
    points_count_last   = GetInputArray("POINTS_COUNT_LAST");


    displacements       = GetOutputMatrix("DISPLACEMENTS");
    max_displacements   = GetOutputSizeY("DISPLACEMENTS");

    feature_locations   = GetOutputMatrix("FEATURE_LOCATIONS");
    flow                = GetOutputMatrix("FLOW");
    flow_count          = GetOutputArray("FLOW_COUNT");


    size_x              = GetInputSizeX("INPUT");
    size_y              = GetInputSizeY("INPUT");
}


void
SparseFlow::Tick()
{
    int c = 0;
    reset_matrix(displacements, 4, max_displacements);

    int s = int(*points_count);
    int sl = int(*points_count_last);

    int hit_count = 0;
    int feature_size = 2*feature_radius+1;
    int margin_top = 0;
    int margin_left = 0;
    int margin_bottom = size_y-feature_size;
    int margin_right = size_x-feature_size;
    float max_diff = feature_threshold*float(feature_size*feature_size);

    long ticklength = GetTickLength();

    float x_factor = ticklength ? 1.0/float(size_x)*1000/float(ticklength) : 1.0/float(size_x); // Vector length as pixels/width/s; scale invariant in time and space
    float y_factor = ticklength ? 1.0/float(size_y)*1000/float(ticklength) : 1.0/float(size_y); ; // Vector length pixels/height/s; scale invariant in time and space

    for(int i=0; i<s; i++)    
        {
            int x1 = int(float(size_x)*points[i][0])-feature_radius;
            int y1 = int(float(size_y)*points[i][1])-feature_radius;
            int x00 = 0;
            int y00 = 0;
            if(0 <= x1 && x1 < margin_right && 0 <= y1 && y1 < margin_bottom)
            {
                float min_diff = max_diff;
                for(int j=0; j<sl; j++)
                if(hypot(points[i][0]-points_last[j][0], points[i][1]-points_last[j][1])<search_radius)
                {
                    int x0 =int(float(size_x)*points_last[j][0])-feature_radius;
                    int y0 =int(float(size_y)*points_last[j][1])-feature_radius;  
                    if(0 <= x0 && x0 < margin_right && 0 <= y0 && y0 < margin_bottom)
                    {
                        float diff = 0;
                        for(int j0=y0, j1=y1; j0<y0+feature_size; j0++,j1++)
                            for(int i0=x0, i1=x1; i0<x0+feature_size; i0++,i1++)
                                diff += abs(input[j1][i1] - input_last[j0][i0]);
                        if(diff < min_diff)
                        {
                            min_diff = diff;
                            x00 = x0;
                            y00 = y0;
                        }
                        hit_count+=1;
                    }
                }
                if(x00!=0 && c<max_displacements)
                {
                    float x_0 = float(x00+feature_radius)/float(size_x);
                    float y_0 = float(y00+feature_radius)/float(size_y);
                    float x_1 = float(x1+feature_radius)/float(size_x);
                    float y_1 = float(y1+feature_radius)/float(size_y);

                    displacements[c][0] = x_1;
                    displacements[c][1] = y_1;
                    displacements[c][2] = x_1+magnification*x_factor*(x_1-x_0);
                    displacements[c][3] = y_1+magnification*y_factor*(y_1-y_0);

                    feature_locations[c][0] = x_1;
                    feature_locations[c][1] = y_1;

                    flow[c][0] = x_factor*(x_1-x_0);
                    flow[c][1] = y_factor*(y_1-y_0);                 

                    c++;
                }
            }
        }    *flow_count=float(c);
}

static InitClass init("SparseFlow", &SparseFlow::Create, "Source/Modules/VisionModules/OpticFlow/SparseFlow/");


