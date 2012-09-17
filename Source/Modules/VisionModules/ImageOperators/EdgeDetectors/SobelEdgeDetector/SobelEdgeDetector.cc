//
//	SobelEdgeDetector.cc	This file is a part of the IKAROS project
//						A module to find edges in an image
//
//    Copyright (C) 2003  Christian Balkenius
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

#include "SobelEdgeDetector.h"


using namespace ikaros;


Module *
SobelEdgeDetector::Create(Parameter * p)
{
    return new SobelEdgeDetector(p);
}



SobelEdgeDetector::SobelEdgeDetector(Parameter * p):
    Module(p)
{
    AddInput("INPUT");
    AddOutput("OUTPUT");

    type = GetIntValueFromList("type");

    input	= NULL;
    output	= NULL;

    dx_temp = NULL;
    dy_temp = NULL;
}



void
SobelEdgeDetector::SetSizes()
{
    int sx = GetInputSizeX("INPUT");
    int sy = GetInputSizeY("INPUT");
    if (sx != unknown_size && sy != unknown_size)
        SetOutputSize("OUTPUT", sx-2, sy-2);
}



void
SobelEdgeDetector::Init()
{
    inputsize_x	 	= GetInputSizeX("INPUT");
    inputsize_y	 	= GetInputSizeY("INPUT");

    outputsize_x	= GetOutputSizeX("OUTPUT");
    outputsize_y	= GetOutputSizeY("OUTPUT");

    input			= GetInputMatrix("INPUT");
    output			= GetOutputMatrix("OUTPUT");

    if (type == 0 || type == 1)
    {
        dx_temp = create_matrix(outputsize_x, outputsize_y);
        dy_temp = create_matrix(outputsize_x, outputsize_y);
    }

    dx_kernel = create_matrix(3, 3);
    dy_kernel = create_matrix(3, 3);

    dx_kernel[0][0] = 1;
    dx_kernel[1][0] = 2;
    dx_kernel[2][0] = 1;

    dx_kernel[0][2] = -1;
    dx_kernel[1][2] = -2;
    dx_kernel[2][2] = -1;

    dy_kernel[0][0] = 1;
    dy_kernel[0][1] = 2;
    dy_kernel[0][2] = 1;

    dy_kernel[2][0] = -1;
    dy_kernel[2][1] = -2;
    dy_kernel[2][2] = -1;
}



SobelEdgeDetector::~SobelEdgeDetector()
{
    if (dx_temp) destroy_matrix(dx_temp);
    if (dy_temp) destroy_matrix(dy_temp);
    destroy_matrix(dx_kernel);
    destroy_matrix(dy_kernel);
}



void
SobelEdgeDetector::CalculateSqrt()
{
    convolve(dx_temp, input, dx_kernel, outputsize_x, outputsize_y, 3, 3);
    convolve(dy_temp, input, dy_kernel, outputsize_x, outputsize_y, 3, 3);
    hypot(output, dx_temp, dy_temp, outputsize_x, outputsize_y);

}



void
SobelEdgeDetector::CalculateAbs()
{
    /*
    	float ** in = input;	// alias
    	for(int j =1; j<inputsize_y-1; j++)
    		for(int i=1; i<inputsize_x-1; i++)
    		{
    			float dx = 	in[j-1][i-1]	-	in[j-1][i+1]	+	2*in[j][i-1]	-	2*in[j][i+1]	+	in[j-1][i-1]	-	in[j+1][i+1];
    			float dy = 	in[j-1][i-1]	+	2*in[j-1][i]	+	in[j-1][i+1]	-	in[j-1][i-1]	-	2*in[j+1][i]	-	in[j+1][i+1];
    			output[j-1][i-1] = abs(dx)+abs(dy);
    		}
    */
    convolve(dx_temp, input, dx_kernel, outputsize_x, outputsize_y, 3, 3);
    convolve(dy_temp, input, dy_kernel, outputsize_x, outputsize_y, 3, 3);
    abs(dx_temp, outputsize_x, outputsize_y);
    abs(dy_temp, outputsize_x, outputsize_y);
    add(output, dx_temp, dy_temp, outputsize_x, outputsize_y);
}



void
SobelEdgeDetector::CalculateDx()
{
    convolve(output, input, dx_kernel, outputsize_x, outputsize_y, 3, 3);
}



void
SobelEdgeDetector::CalculateDy()
{
    convolve(output, input, dy_kernel, outputsize_x, outputsize_y, 3, 3);
}



void
SobelEdgeDetector::CalculateAbsDx()
{
    convolve(output, input, dx_kernel, outputsize_x, outputsize_y, 3, 3);
    abs(output, output, outputsize_x, outputsize_y);
}



void
SobelEdgeDetector::CalculateAbsDy()
{
    convolve(output, input, dy_kernel, outputsize_x, outputsize_y, 3, 3);
    abs(output, output, outputsize_x, outputsize_y);
}



void
SobelEdgeDetector::Tick()
{
    switch (type)
    {
    case 0:
        CalculateSqrt();	break;
    case 1:
        CalculateAbs();		break;
    case 2:
        CalculateDx();		break;
    case 3:
        CalculateDy();		break;
    case 4:
        CalculateAbsDx();   break;
    case 5:
        CalculateAbsDy();   break;
    default:
        break;
    }
}



