//
//	CircleDetector .cc		This file is a part of the IKAROS project
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

#include "CircleDetector.h"
#include "ctype.h"
#include "IKAROS_Utils.h"


#include "EyeTracker.h"

using namespace ikaros;

CircleDetector ::CircleDetector (Parameter * p):
	Module(p)
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
	
	threshold		=	GetFloatValue("threshold", 2000.0);
}



void
CircleDetector ::SetSizes()
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
CircleDetector ::Init()
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
	
	hist_x			= GetOutputArray("HISTX");
	hist_dx			= GetOutputArray("HISTDX");
	
	dGx				= create_matrix(7, 7);
	dGy				= create_matrix(7, 7);
	
	// Initialize Gaussian Filters
	
	float Gsize = 0.5;
	
	for(int j=0; j<7; j++)
		for(int i=0; i<7; i++)
		{
			float x = Gsize * float(i-3);
			float y = Gsize * float(j-3);
			dGx[j][i] = -2*x*exp(-(x*x + y*y));
			dGy[j][i] = -2*y*exp(-(x*x + y*y));
		}
		
	EyeTracker_Init(inputsize_x, inputsize_y, 2, 0.5, 5, 40, 5, 15, 2); // ********************
}



CircleDetector ::~CircleDetector ()
{
}



void
CircleDetector ::Tick()
{
	for(int j =0; j<inputsize_y; j++)
		for(int i=0; i<inputsize_x; i++)
			image[j][i] = input[j][i];

	float ex, ey;
	
	int intimage[128*128];
	
//	for(int i=0; i<128; i++)
//		intimage[i] = &a[i*128];

	int p=0;
	for(int j =0; j<inputsize_y; j++)
		for(int i=0; i<inputsize_x; i++)
			intimage[p++] =int(input[j][i]);
	
	EyeTracker_FindIris(intimage, &ex, &ey);
	
	printf("%4.1f %4.1f\n", ex, ey);

	// Draw results

	if(ex > 0 && ex < outputsize_x && ey > 0 && ey < outputsize_y)
	{
		for(int j =0; j<inputsize_y; j++)
			image[j][3+int(ex+0.5)] = 255-image[j][-3+int(ex+0.5)];

		for(int i=0; i<inputsize_x; i++)
			image[3+int(ey+0.5)][i] = 255-image[-3+int(ey+0.5)][i];
	}
}


static InitClass init("CircleDetector", &CircleDetector::Create, "Source/Modules/VisionModules/CircleDetector/");

