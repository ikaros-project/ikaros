//
//	CircleDetector .cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2002-2016  Christian Balkenius
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

#include "CircleDetector.h"

using namespace ikaros;


void
CircleDetector ::Init()
{
    Bind(min_radius, "min_radius");
    Bind(max_radius, "max_radius");
    
    
    edge_list       = GetInputMatrix("EDGE_LIST");
    edge_list_size  = GetInputArray("EDGE_LIST_SIZE");

	position		= GetOutputArray("POSITION");
	diameter		= GetOutputArray("DIAMETER");

	hist_x			= GetOutputArray("HISTX");
	hist_dx			= GetOutputArray("HISTDX");
}



void
CircleDetector::Tick()
{
    int size_x = 500;
    int size_y = 500;
    float ** g = create_matrix(500, 500); // scale appropriately later; allocate once
    
	float estimated_x, estimated_y;
	int i,j;
    int edgeCount = int(*edge_list_size);

    estimated_x = 0;
    estimated_y = 0;

	if(edgeCount < 2)
	{
		return;
	}
	
	// Test distance between points
	
    float radius = 0;
	long intCnt = 0;
	for(i=0; i<edgeCount; i++)
		for(j=0; j<edgeCount; j++)
		{
			float dist = sqrt(sqr(edge_list[i][0] - edge_list[j][0]) + sqr(edge_list[i][1] - edge_list[j][1]));
			if(i != j && dist < 2*max_radius && dist > 2*min_radius)
			{
				// Calculate intersection point
				
				float x0 = edge_list[i][0];
				float y0 = edge_list[i][1];
				float a0 = edge_list[i][2];
				float b0 = edge_list[i][3];

				float x1 = edge_list[j][0];
				float y1 = edge_list[j][1];
				float a1 = edge_list[j][2];
				float b1 = edge_list[j][3];
				
				if(-a0*b1 + b0*a1 != 0)
				{
					float ix = x0 + (a0*(-y0*a1+y1*a1+b1*x0-b1*x1))/(-a0*b1 + b0*a1);
					float iy = y0 + (b0*(-y0*a1+y1*a1+b1*x0-b1*x1))/(-a0*b1 + b0*a1);
				
					// Reject intersection on the wrong side of edges
					
					bool reject = false;
					
					if(a0 < 0 && ix > x0)
						reject = true;

					if(a0 > 0 && ix < x0)
						reject = true;
					
					if(a1 < 0 && ix > x1)
						reject = true;

					if(a1 > 0 && ix < x1)
						reject = true;
					
					if(b0 < 0 && iy > y0)
						reject = true;

					if(b0 > 0 && iy < y0)
						reject = true;
					
					if(b1 < 0 && iy > y1)
						reject = true;

					if(b1 > 0 && iy < y1)
						reject = true;
					
					float tilesize = 10;
					if(!reject && 0 < ix && ix < size_x && 0 < iy && iy < size_y)
					{
						int dx, dy;
						float increment = (a0*a0+b0*b0)*(a1*a1+b1*b1);
						for(dx=0; dx<tilesize; dx++)
							for(dy=0; dy<tilesize; dy++)
								g[(int)clip(iy+dy, 0, float(size_y-1))][(int)clip(ix+dx, 0, float(size_x-1))] += increment;

                        radius += hypot(ix-x0, iy-y0);
						intCnt++;
					}
				}
			}
		}
		
    radius /= float(intCnt);
    
    printf("Radius: %f\n", radius);
    radius /= 128.0;
    
	// Find maximum
	
	float e = 0;
	for(j=0; j<size_y; j++)
		for(i=0; i<size_x; i++)
			if(g[j][i] > e)
			{
				e = g[j][i];
				estimated_x = i;
				estimated_y = j;
			}

//	estimated_x -= 3;   // edge correction - should be removed
//	estimated_y -= 3;
	
	// Recalculate estimation
	
	float ex = 0;
	float ey = 0;
	float ww = 0;
	
	for(i=0; i<edgeCount; i++)
		for(j=0; j<edgeCount; j++)
		{
			float dist = sqrt(sqr(edge_list[i][0] - edge_list[j][0]) + sqr(edge_list[i][1] - edge_list[j][1]));
			if(i != j && dist < 2*max_radius && dist > 2*min_radius)
			{
				// Calculate intersection point
				
				float x0 = edge_list[i][0];
				float y0 = edge_list[i][1];
				float a0 = edge_list[i][2];
				float b0 = edge_list[i][3];

				float x1 = edge_list[j][0];
				float y1 = edge_list[j][1];
				float a1 = edge_list[j][2];
				float b1 = edge_list[j][3];
				
				if(-a0*b1 + b0*a1 != 0)
				{
					float ix = x0 + (a0*(-y0*a1+y1*a1+b1*x0-b1*x1))/(-a0*b1 + b0*a1);
					float iy = y0 + (b0*(-y0*a1+y1*a1+b1*x0-b1*x1))/(-a0*b1 + b0*a1);
				
					bool reject = false;
					
					if(a0 < 0 && ix > x0)
						reject = true;

					if(a0 > 0 && ix < x0)
						reject = true;
					
					if(a1 < 0 && ix > x1)
						reject = true;

					if(a1 > 0 && ix < x1)
						reject = true;
					
					if(b0 < 0 && iy > y0)
						reject = true;

					if(b0 > 0 && iy < y0)
						reject = true;
					
					if(b1 < 0 && iy > y1)
						reject = true;

					if(b1 > 0 && iy < y1)
						reject = true;
					
					if(!reject  && sqrt(sqr(estimated_x-ix)+sqr(estimated_y-iy)) < 10)
					{
						ex += ix;
						ey += iy;
						ww += 1.0;
					}
				}
			}
		}
		
	estimated_x = ex/ww;
	estimated_y = ey/ww;


	// Recalculate estimation
	{
	float ex = 0;
	float ey = 0;
	float ww = 0;
	
	for(i=0; i<edgeCount; i++)
		for(j=0; j<edgeCount; j++)
		{
			float dist = sqrt(sqr(edge_list[i][0] - edge_list[j][0]) + sqr(edge_list[i][1] - edge_list[j][1]));
			if(i != j && dist < 2*max_radius && dist > 2*min_radius)
			{
				// Calculate intersection point
				
				float x0 = edge_list[i][0];
				float y0 = edge_list[i][1];
				float a0 = edge_list[i][2];
				float b0 = edge_list[i][3];

				float x1 = edge_list[j][0];
				float y1 = edge_list[j][1];
				float a1 = edge_list[j][2];
				float b1 = edge_list[j][3];
				
				if(-a0*b1 + b0*a1 != 0)
				{
					float ix = x0 + (a0*(-y0*a1+y1*a1+b1*x0-b1*x1))/(-a0*b1 + b0*a1);
					float iy = y0 + (b0*(-y0*a1+y1*a1+b1*x0-b1*x1))/(-a0*b1 + b0*a1);
				
					int reject = false;
					
					if(a0 < 0 && ix > x0)
						reject = true;

					if(a0 > 0 && ix < x0)
						reject = true;
					
					if(a1 < 0 && ix > x1)
						reject = true;

					if(a1 > 0 && ix < x1)
						reject = true;
					
					if(b0 < 0 && iy > y0)
						reject = true;

					if(b0 > 0 && iy < y0)
						reject = true;
					
					if(b1 < 0 && iy > y1)
						reject = true;

					if(b1 > 0 && iy < y1)
						reject = true;
					
					if(!reject && sqrt(sqr(estimated_x-ix)+sqr(estimated_y-iy)) < 5)
					{
						ex += ix;
						ey += iy;
						ww += 1.0;
					}
				}
			}
		}
	
        estimated_x = ex/ww;
        estimated_y = ey/ww;
	}
	
    position[0] = 4.0/128.0+estimated_x/(128.0-2*2.0);
	position[1] = 4.0/128.0+estimated_y/(128.0-2*2.0);
	position[2] = radius;

    printf("%f %f\n", position[0], position[1]);
}



static InitClass init("CircleDetector", &CircleDetector::Create, "Source/Modules/VisionModules/CircleDetector/");

