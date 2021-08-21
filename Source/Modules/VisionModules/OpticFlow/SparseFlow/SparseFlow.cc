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
    //Bind(threshold, "threshold");
    //Bind(max_points, "max_points");

    input               = GetInputMatrix("INPUT");
    input_last  	    = GetInputMatrix("INPUT_LAST");

    points             = GetInputMatrix("POINTS");
    points_last        = GetInputMatrix("POINTS_LAST");

    points_count        = GetInputArray("POINTS_COUNT");
    points_count_last   = GetInputArray("POINTS_COUNT_LAST");


    displacements       = GetOutputMatrix("DISPLACEMENTS");
    max_displacements   = GetOutputSizeY("DISPLACEMENTS");

    feature_locations   = GetInputMatrix("FEATURE_LOCATIONS");
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

    float max_dist = 0.05;
    int s = int(*points_count);
    int sl = int(*points_count_last);

    int hit_count = 0;
    int pix = 3;
    int fsize = 2*pix+1;
    int margin_top = 0;
    int margin_left = 0;
    int margin_bottom = size_y-fsize;
    int margin_right = size_x-fsize;
    float max_diff = float(fsize*fsize); // Add factor here

    for(int i=0; i<s; i++)    
        {
            int x1 = int(float(size_x)*points[i][0])-pix;
            int y1 = int(float(size_y)*points[i][1])-pix;
            int x00 = 0;
            int y00 = 0;
            if(0 <= x1 && x1 < margin_right && 0 <= y1 && y1 < margin_bottom)
            {
                float min_diff = max_diff;
                for(int j=0; j<sl; j++)
                if(hypot(points[i][0]-points_last[j][0], points[i][1]-points_last[j][1]) <max_dist)
                {
                    int x0 =int(float(size_x)*points_last[j][0])-pix;
                    int y0 =int(float(size_y)*points_last[j][1])-pix;  
                    if(0 <= x0 && x0 < margin_right && 0 <= y0 && y0 < margin_bottom)
                    {
                        float diff = 0;
                        for(int j0=y0, j1=y1; j0<y0+fsize; j0++,j1++)
                            for(int i0=x0, i1=x1; i0<x0+fsize; i0++,i1++)
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
                    float x_0 = float(x00+pix)/float(size_x);
                    float y_0 = float(y00+pix)/float(size_y);
                    float x_1 = float(x1+pix)/float(size_x);
                    float y_1 = float(y1+pix)/float(size_y);

                    displacements[c][0] = x_1;
                    displacements[c][1] = y_1;
                    displacements[c][2] = x_1+3*(x_1-x_0);
                    displacements[c][3] = y_1+3*(y_1-y_0);
                    c++;
                }
            }
        }    *flow_count=float(c);
}

static InitClass init("SparseFlow", &SparseFlow::Create, "Source/Modules/VisionModules/OpticFlow/SparseFlow/");


