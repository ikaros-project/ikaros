//
//	  BlockMatching.cc		This file is a part of the IKAROS project
//
//
//    Copyright (C) 2006  Christian Balkenius
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

#include "BlockMatching.h"

using namespace ikaros;

void
BlockMatching::Init()
{
	Bind(block_radius, "block_radius");
	Bind(search_window_radius, "search_window_radius");
	Bind(search_method, "search_method");
	Bind(metric, "metric");

	size_x	 	= GetInputSizeX("INPUT");
	size_y	 	= GetInputSizeY("INPUT");

	input		= GetInputMatrix("INPUT");
	input_last	= GetInputMatrix("INPUT-LAST");

	points		= GetInputMatrix("POINTS");
	no_of_points= GetInputArray("NO-OF-POINTS");
    max_points  = GetInputSizeY("POINTS");
    
	flow		= GetOutputMatrix("FLOW");
	flow_size	= GetOutputArray("FLOW-SIZE");
    
    debug 		= GetOutputMatrix("DEBUG");
}


/*
static inline void
to_pixels(int & x, int & y, float image_x, float image_y, int width, int height)
{
	x = image_x*float(width-1);
	y = image_y*float(height-1);
}



static inline void
to_position(float & image_x, float & image_y, float x, float y, int width, int height)
{
	image_x = x/float(width-1);
	image_y = y/float(height-1);
}
*/

static inline void
to_pixels(int & x, float image_x, int size)
{
	x = image_x*float(size-1);
}



static inline void
to_position(float & image_x, float x, int size)
{
	image_x = x/float(size-1);
}

/*
static inline void
crop_rect(int & x0, int & y0, int & x1, int & y1, int crop_x0, int crop_y0, int crop_x1, int crop_y1, int margin)
{
	if(x0 < crop_x0+margin)
    	x0 = crop_x0+margin;

	if(x1 > crop_x1-margin)
    	x1 = crop_x1-margin;

	if(y0 < crop_y0+margin)
    	y0 = crop_y0+margin;

	if(y1 > crop_y1-margin)
    	y1 = crop_y1-margin;
}
*/

static inline void
crop_interval(int & x0, int & x1, int crop_x0, int crop_x1, int margin)
{
	if(x0 < crop_x0+margin)
    	x0 = crop_x0+margin;

	if(x1 > crop_x1-margin)
    	x1 = crop_x1-margin;
}


/*
void
BlockMatching::MatchBlock(int i)
{
	// BlockMatch point i between INPUT and INPUT-LAST
    
    int x, y;
    
    to_pixels(x, y, points[i][0], points[i][1], size_x, size_y);
    
    int wx0 = x - search_window_radius;
    int wx1 = x + search_window_radius;
    int wy0 = y - search_window_radius;
    int wy1 = y + search_window_radius;
    
    crop_rect(wx0, wy0, wx1, wy1, 0, 0, size_x, size_y, block_radius+search_window_radius+1);
    
    
    int min_x = x;
    int min_y = y;
    float min_v = maxfloat;
    for(int j=wy0; j<wy1; j++)
    	for(int i=wx0; i<wx1; i++)
    	{
        	float d = 0;
    		for(int dj=-block_radius; dj<=block_radius; dj++)
    			for(int di=-block_radius; di<=block_radius; di++)
                {
            		float q = input[y+dj][x+di] - input_last[j+dj][i+di];
                    d += q*q;
                }
            if(d < min_v)
            {
            	min_v = d;
                min_x = i;
                min_y = j;
            }
            debug[j-wy0][i-wx1] = d;
        }
    
    flow[i][0] = points[i][0];
    flow[i][1] = points[i][1];
    to_position(flow[i][2], flow[i][3], min_x, min_y, size_x, size_y);
}
*/

/*
void
BlockMatching::MatchBlock(int i) // Correlation
{
	// BlockMatch point i between INPUT and INPUT-LAST
    
    int x, y;
    
    to_pixels(x, points[i][0], size_x);
    to_pixels(y, points[i][1], size_y);
    
    int wx0 = x - search_window_radius;
    int wx1 = x + search_window_radius;
    int wy0 = y - search_window_radius;
    int wy1 = y + search_window_radius;
    
    crop_interval(wx0, wx1, 0, size_x, block_radius+search_window_radius+1);
    crop_interval(wy0, wy1, 0, size_y, block_radius+search_window_radius+1);
    
    float d = 0;
    float L0 = 0;
    float L1 = 0;
    for(int dj=-block_radius; dj<=block_radius; dj++)
        for(int di=-block_radius; di<=block_radius; di++)
        {
            d += input_last[y+dj][x+di] * input[y+dj][x+di];
            L0 += input_last[y+dj][x+di]  * input_last[y+dj][x+di];
            L1 += input[y+dj][x+di] * input[y+dj][x+di];
        }
    float dz = d/(sqrt(L0)*sqrt(L1));
    
    // zero displacement

    // dispalced
    
    int min_x = x;
    int min_y = y;
    float min_v = 0;
    for(int j=wy0; j<wy1; j++)
    	for(int i=wx0; i<wx1; i++)
    	{
        	float d = 0;
            float L0 = 0;
            float L1 = 0;
    		for(int dj=-block_radius; dj<=block_radius; dj++)
    			for(int di=-block_radius; di<=block_radius; di++)
                {
            		d += input_last[y+dj][x+di] * input[j+dj][i+di];
                    L0 += input_last[y+dj][x+di]  * input_last[y+dj][x+di];
                    L1 += input[j+dj][i+di] * input[j+dj][i+di];
                }
			d /= (sqrt(L0)*sqrt(L1));
            if(d > min_v)
            {
            	min_v = d;
                min_x = i;
                min_y = j;
            }
//            debug[j][i] = d;
        }
    
//	printf("%f\n", min_v/dz);
    
    flow[i][0] = points[i][0];
    flow[i][1] = points[i][1];

	if(min_v/dz > 1.0) // motion detected
    {
    	to_position(flow[i][2], min_x, size_x);
    	to_position(flow[i][3], min_y, size_y);
    }
    else
    {
    	flow[i][2] = points[i][0];
    	flow[i][3] = points[i][1];
    }
}
*/



void
BlockMatching::MatchBlock(int i) // Correlation
{
	// BlockMatch point i between INPUT and INPUT-LAST
    
    int x, y;
    
    to_pixels(x, points[i][0], size_x);
    to_pixels(y, points[i][1], size_y);
    
    if(x-block_radius < 0 || x+block_radius >= size_x)
        return;
    
    if(y-block_radius < 0 || y+block_radius >= size_y)
        return;
    
    int wx0 = x - search_window_radius;
    int wx1 = x + search_window_radius;
    int wy0 = y - search_window_radius;
    int wy1 = y + search_window_radius;
    
    crop_interval(wx0, wx1, 0, size_x, block_radius+search_window_radius); // was +1
    crop_interval(wy0, wy1, 0, size_y, block_radius+search_window_radius);
    
    float d = 0;
    float L0 = 0;
    float L1 = 0;
    for(int dj=-block_radius; dj<=block_radius; dj++)
        for(int di=-block_radius; di<=block_radius; di++)
            {
                d += input_last[y+dj][x+di] * input[y+dj][x+di];
                L0 += input_last[y+dj][x+di]  * input_last[y+dj][x+di];
                L1 += input[y+dj][x+di] * input[y+dj][x+di];
            }
    float dz = d/(sqrt(L0)*sqrt(L1));
    
    // zero displacement

    // displaced
    
    int min_x = x;
    int min_y = y;
    float min_v = 0;
    for(int j=wy0; j<wy1; j++)
    	for(int i=wx0; i<wx1; i++)
    	{
        	float d = 0;
            float L1 = 0;
            int j_dj = j-block_radius;
            int y_dj = y-block_radius;
    		for(int dj=-block_radius; dj<=block_radius; dj++)
            {
    			for(int di=-block_radius; di<=block_radius; di++)
                {
            		d += input_last[y_dj][x+di] * input[j_dj][i+di];
                    L1 += input[j_dj][i+di] * input[j_dj][i+di];
                }
                j_dj++;
                y_dj++;
            }
			d /= (sqrt(L0)*sqrt(L1));
            if(d > min_v)
            {
            	min_v = d;
                min_x = i;
                min_y = j;
            }
//            debug[j][i] = d;
        }
    
//	printf("%f\n", min_v/dz);
    
    flow[i][0] = points[i][0];
    flow[i][1] = points[i][1];

	if(min_v/dz > 1.0) // motion detected
    {
    	to_position(flow[i][2], min_x, size_x);
    	to_position(flow[i][3], min_y, size_y);
    }
    else
    {
    	flow[i][2] = points[i][0];
    	flow[i][3] = points[i][1];
    }
}


void
BlockMatching::Tick()
{
	reset_matrix(flow, 4, max_points);
    
	int c = int(*no_of_points);
    
    for(int i=0; i<c; i++)
    	MatchBlock(i);
    
    *flow_size = *no_of_points;
}



static InitClass init("BlockMatching", &BlockMatching::Create, "Source/Modules/VisionModules/ImageOperators/BlockMatching/");
