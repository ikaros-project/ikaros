//
//	CircleDetector2.cc		This file is a part of the IKAROS project
//								A module to filter image with a kernel
//
//    Copyright (C) 2002  Christian Balkenius
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

//#define DEBUG

#include "CircleDetector2.h"
#include "ctype.h"
#include "IKAROS_Utils.h"



Module *
CircleDetector2::Create(char * name, Parameter * p)
{ 
	return new CircleDetector2(name, p);
}



CircleDetector2::CircleDetector2(char * name, Parameter * p):
	Module(name)
{
	AddInput("INPUT");
	AddOutput("OUTPUT");
	AddOutput("IMAGE");
	AddOutput("DX");
	AddOutput("DY");
	AddOutput("G");
	AddOutput("EDGE");
	AddOutput("ACCUMULATOR");

	AddOutput("HISTDX", 15);
	AddOutput("HISTX", 15);

	input			= NULL;
	output		= NULL;
	image		= NULL;
	dx			= NULL;
	dy			= NULL;
	g			= NULL;
	edge			= NULL;
	accumulator	= NULL;
	
	dGx			= NULL;
	dGy			= NULL;
	
	threshold		=	GetFloatValue(p, "threshold", 2000.0);
}



void
CircleDetector2::SetSizes()
{
	int sx = GetInputSizeX("INPUT");				
	int sy = GetInputSizeY("INPUT");				
	if(sx != unknown_size && sy != unknown_size)
	{
		SetOutputSize("IMAGE", sx, sy);
		SetOutputSize("OUTPUT", sx-4, sy-4);
		SetOutputSize("DX", sx-4, sy-4);
		SetOutputSize("DY", sx-4, sy-4);
		SetOutputSize("G", sx-4, sy-4);
		SetOutputSize("EDGE", sx-4, sy-4);
		SetOutputSize("ACCUMULATOR", sx-4, sy-4);
		
		printf(">>> %d <###\n", sy-2);fflush(NULL);
	}
}



void
CircleDetector2::Init()
{
	inputsize_x	 	= GetInputSizeX("INPUT");
	inputsize_y	 	= GetInputSizeY("INPUT");
	
	outputsize_x	 	= GetOutputSizeX("OUTPUT");
	outputsize_y	 	= GetOutputSizeY("OUTPUT");

	input				= GetInputMatrix("INPUT");
	output			= GetOutputMatrix("OUTPUT");
	image			= GetOutputMatrix("IMAGE");
	dx				= GetOutputMatrix("DX");
	dy				= GetOutputMatrix("DY");
	g				= GetOutputMatrix("G");
	edge				= GetOutputMatrix("EDGE");
	accumulator		= GetOutputMatrix("ACCUMULATOR");
	
	hist_x			= GetOutputData("HISTX");
	hist_dx			= GetOutputData("HISTDX");
	
	dGx				= CreateMatrix(5, 5);
	dGy				= CreateMatrix(5, 5);
	
	// Initialize Gaussian Filters
	
	float Gsize = 0.5;
	
	for(int j=0; j<5; j++)
		for(int i=0; i<5; i++)
		{
			float x = Gsize * float(i-2);
			float y = Gsize * float(j-2);
			dGx[j][i] = -2*x*exp(-(x*x + y*y));
			dGy[j][i] = -2*y*exp(-(x*x + y*y));
		}
}



CircleDetector2::~CircleDetector2()
{
}

#define TSIZE 25000

float edgeTableX[TSIZE];
float edgeTableY[TSIZE];
float edgeTableDX[TSIZE];
float edgeTableDY[TSIZE];
int	edgeCount = 0;


float stack[9];
int	sp = 0;



void
CircleDetector2::ApplyMedianFilter()	// Silly Algorithm
{
	for(int j =1; j<inputsize_y-5; j++)
		for(int i=1; i<inputsize_x-5; i++)
			{
				// Push Data
				
				sp = 0;

				stack[sp++] = input[j-1][i-1];
				stack[sp++] = input[j][i-1];
				stack[sp++] = input[j+1][i-1];

				stack[sp++] = input[j-1][i];
				stack[sp++] = input[j][i];
				stack[sp++] = input[j+1][i];

				stack[sp++] = input[j-1][i+1];
				stack[sp++] = input[j][i+1];
				stack[sp++] = input[j+1][i+1];
			
				// Sort
				
				for(int s=0; s<9; s++)
					for(int t=s; t<9; t++)
						if(stack[t] > stack[s])
						{
							 float tmp = stack[s];
							 stack[s] = stack[t];
							 stack[t] = tmp;
						}
						
				//printf("Sorted:");
				//for(int s=0; s<9; s++)
				//	printf("%f\n", stack[s]);
					
				g[j-1][i-1] = stack[4];	// Get median value
			}

	for(int j =2; j<inputsize_y-8; j++)
		for(int i=2; i<inputsize_x-8; i++)
			{
				// Push Data
				
				sp = 0;

				stack[sp++] = g[j-1][i-1];
				stack[sp++] = g[j][i-1];
				stack[sp++] = g[j+1][i-1];

				stack[sp++] = g[j-1][i];
				stack[sp++] = g[j][i];
				stack[sp++] = g[j+1][i];

				stack[sp++] = g[j-1][i+1];
				stack[sp++] = g[j][i+1];
				stack[sp++] = g[j+1][i+1];
			
				// Sort
				
				for(int s=0; s<9; s++)
					for(int t=s; t<9; t++)
						if(stack[t] > stack[s])
						{
							 float tmp = stack[s];
							 stack[s] = stack[t];
							 stack[t] = tmp;
						}
						
				//printf("Sorted:");
				//for(int s=0; s<9; s++)
				//	printf("%f\n", stack[s]);
					
				input[j-1][i-1] = stack[4];	// Get median value
			}
}



void
CircleDetector2::CalculateEdges()
{
	int inc = 2;
	
	edgeCount = 0;

	for(int j =1; j<inputsize_y-1; j++)
		for(int i=1; i<inputsize_x-1; i++)
			input[j][i] = (input[j][i] > 128 ? 255 : 2*input[j][i]);

	// Estimate gradient - dx
	
	for(int j =2; j<inputsize_y-2; j+=inc)
		for(int i=2; i<inputsize_x-2; i+=inc)
		{
			dx[j-2][i-2] = 0;
			for(int jj=-2; jj<3; jj++)
				for(int ii=-2; ii<3; ii++)
					dx[j-2][i-2] += dGx[jj+2][ii+2] * input[j+jj][i+ii];
		}

	// Estimate gradient - dy
	
	for(int j =2; j<inputsize_y-2; j+=inc)
		for(int i=2; i<inputsize_x-2; i+=inc)
		{
			dy[j-2][i-2] = 0;
			for(int jj=-2; jj<3; jj++)
				for(int ii=-2; ii<3; ii++)
					dy[j-2][i-2] += dGy[jj+2][ii+2] * input[j+jj][i+ii];
		}

	// Test each location for an edge
	
	ResetArray(g, outputsize_y, outputsize_x);
	ResetArray(edge, outputsize_y, outputsize_x);
	
	float dymax = 0;
	for(int j =2; j<inputsize_y-2; j++)
		for(int i=2; i<inputsize_x-2; i++)
			if(dy[j-2][i-2] > dymax)
				dymax = dy[j-2][i-2];

	float dxmax = 0;
	for(int j =2; j<inputsize_y-2; j++)
		for(int i=2; i<inputsize_x-2; i++)
			if(dx[j-2][i-2] > dxmax)
				dxmax = dx[j-2][i-2];

	float dT2 = 0.25 * dxmax * dymax;

	for(int j =0; j<outputsize_y; j+=1)
		for(int i=0; i<outputsize_x; i+=1)
			if((sqr(dx[j][i]) + sqr(dy[j][i])) > dT2 && edgeCount<TSIZE-1)
			{
				// dx[j][i] = 1000.0;
				// dy[j][i] = 1000.0;
				edge[j][i] = sqrt(dx[j][i]*dx[j][i] + dy[j][i]*dy[j][i]);
				edgeTableX[edgeCount] = float(i);
				edgeTableY[edgeCount] = float(j);
				edgeTableDX[edgeCount] = dx[j][i];
				edgeTableDY[edgeCount] = dy[j][i];
				//printf("(%d, %d) = %f %f\n", i, j, dx[j][i], dy[j][i]);
				edgeCount ++;
			}
			
	printf("edgeCount = %d\n", edgeCount);

}



void
CircleDetector2::CalculateIntersections()
{
	// Test distance between points
	
	long intCnt = 0;
	for(int i=0; i<edgeCount; i++)
		for(int j=0; j<edgeCount; j++)
		{
			float dist = sqrt(sqr(edgeTableX[i] - edgeTableX[j]) + sqr(edgeTableY[i] - edgeTableY[j]));
			// if(i != j && (pradius-width) < dist && dist < (pradius+width))	 // pupil size constaint (eg. 12+-3)
			if(i != j && dist < 40 && dist > 5)
			{
				// Calculate intersection point
				
				float x0 = edgeTableX[i];
				float y0 = edgeTableY[i];
				float a0 = edgeTableDX[i];
				float b0 = edgeTableDY[i];

				float x1 = edgeTableX[j];
				float y1 = edgeTableY[j];
				float a1 = edgeTableDX[j];
				float b1 = edgeTableDY[j];
				
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
					if(!reject && 0 < ix && ix < outputsize_x-tilesize-1 && 0 < iy && iy < outputsize_y-tilesize-1)
					{
						float increment = (a0*a0+b0*b0)*(a1*a1+b1*b1);
						for(int dx=0; dx<tilesize; dx++)
							for(int dy=0; dy<tilesize; dy++)
								g[int(iy+dy)][int(ix+dx)] += increment;

						intCnt++;
					}
				}
			}
		}
		
	printf("INT CNT = %ld\n", intCnt);

	// Find maximum
	
	float e = 0;
	for(int j=0; j<outputsize_y; j++)
		for(int i=0; i<outputsize_x; i++)
			if(g[j][i] > e)
			{
				e = g[j][i];
				estimated_x = i;
				estimated_y = j;
			}

	estimated_x -= 3;
	estimated_y -= 3;
	
	// Recalculate estimation
	
	float ex = 0;
	float ey = 0;
	float w = 0;
	
	for(int i=0; i<edgeCount; i++)
		for(int j=0; j<edgeCount; j++)
		{
			float dist = sqrt(sqr(edgeTableX[i] - edgeTableX[j]) + sqr(edgeTableY[i] - edgeTableY[j]));
			if(i != j && 5 < dist && dist < 15)
			{
				// Calculate intersection point
				
				float x0 = edgeTableX[i];
				float y0 = edgeTableY[i];
				float a0 = edgeTableDX[i];
				float b0 = edgeTableDY[i];

				float x1 = edgeTableX[j];
				float y1 = edgeTableY[j];
				float a1 = edgeTableDX[j];
				float b1 = edgeTableDY[j];
				
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
						w += 1.0;
					}
				}
			}
		}
		
	estimated_x = ex/w;
	estimated_y = ey/w;


	// Recalculate estimation
	{
	float ex = 0;
	float ey = 0;
	float w = 0;
	
	for(int i=0; i<edgeCount; i++)
		for(int j=0; j<edgeCount; j++)
		{
			float dist = sqrt(sqr(edgeTableX[i] - edgeTableX[j]) + sqr(edgeTableY[i] - edgeTableY[j]));
			if(i != j && 5 < dist && dist < 15)
			{
				// Calculate intersection point
				
				float x0 = edgeTableX[i];
				float y0 = edgeTableY[i];
				float a0 = edgeTableDX[i];
				float b0 = edgeTableDY[i];

				float x1 = edgeTableX[j];
				float y1 = edgeTableY[j];
				float a1 = edgeTableDX[j];
				float b1 = edgeTableDY[j];
				
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
					
					if(!reject && sqrt(sqr(estimated_x-ix)+sqr(estimated_y-iy)) < 5)
					{
						ex += ix;
						ey += iy;
						w += 1.0;
					}
				}
			}
		}
	
	estimated_x = ex/w;
	estimated_y = ey/w;
	}
	
}



void
CircleDetector2::CalculateCircleTemplate()
{
	// Black on White Circle
/*	
	ResetArray(g, inputsize_y-2, inputsize_x-2);

	for(int j=5; j<inputsize_y-6; j++)
		for(int i=5; i<inputsize_x-6; i++)
		{
			float match = 0;
			for(int jj=-5; jj<5; jj++)
				for(int ii =-5; ii<5; ii++)
					if(sqrt(ii*ii+jj*jj) < 5)
						match -= input[j+jj][i+ii];
					else
						match += input[j+jj][i+ii];
						
			g[j][i] = match;
		}
*/		
	// Circle Contour
	
	int r = 5;
	int w = 2;
	
	ResetArray(g, inputsize_y-2, inputsize_x-2);
	
	for(int j=r; j<inputsize_y-r-1; j++)
		for(int i=r; i<inputsize_x-r-1; i++)
		{
			float match = 0;
			for(int jj=-r; jj<r; jj++)
				for(int ii =-r; ii<r; ii++)
					if(sqrt(ii*ii+jj*jj) < r-w || sqrt(ii*ii+jj*jj) > r+w)
						match -= abs(dx[j+jj][i+ii]) + abs(dx[j+jj][i+ii]);
					else
						match += abs(dx[j+jj][i+ii]) + abs(dx[j+jj][i+ii]);
						
			g[j][i] = match;
		}

}



void
CircleDetector2::CalculateClusters()
{

}



void
CircleDetector2::Tick()
{
//	ApplyMedianFilter();
//	ApplyMedianFilter();
	
	CalculateEdges();
//	CalculateCircleTemplate();
	
	for(int j =0; j<inputsize_y; j++)
		for(int i=0; i<inputsize_x; i++)
			image[j][i] = input[j][i];

	// Calculate Intersections the hard way
	
	CalculateIntersections();

	if(estimated_x > 0 && estimated_x < outputsize_x && estimated_y > 0 && estimated_y < outputsize_y)
	{
		for(int j =0; j<inputsize_y; j++)
			image[j][1+int(estimated_x+0.5)] = 255-image[j][-3+int(estimated_x+0.5)];

		for(int i=0; i<inputsize_x; i++)
			image[1+int(estimated_y+0.5)][i] = 255-image[-3+int(estimated_y+0.5)][i];
	}
}



