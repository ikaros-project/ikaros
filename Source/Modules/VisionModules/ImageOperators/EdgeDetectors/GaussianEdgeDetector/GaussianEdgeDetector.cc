//
//	GaussianEdgeDetector.cc		This file is a part of the IKAROS project
//								A module to find edges in an image including their orientation
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



#include "GaussianEdgeDetector.h"

using namespace ikaros;



Module *
GaussianEdgeDetector::Create(Parameter * p)
{
    return new GaussianEdgeDetector(p);
}



GaussianEdgeDetector::GaussianEdgeDetector(Parameter * p):
        Module(p)
{
    scale           = GetFloatValue("scale", 1.0);
    filterradius	= int(2*scale+0.5);
    filtersize		= 1+2*filterradius;

    AddInput("INPUT");

    AddOutput("OUTPUT");			// Edge intensity
    AddOutput("MAXIMA");			// Edge maxima
    AddOutput("NODES");             // Edge nodes
    AddOutput("ORIENTATION");		// Orientation estimation
    AddOutput("dx");				// The filters used
    AddOutput("dy");
    AddOutput("dGx", filtersize, filtersize);	// The filters used
    AddOutput("dGy", filtersize, filtersize);

    input			= NULL;
    output			= NULL;
    maxima			= NULL;
    orientation		= NULL;
    dx				= NULL;
    dy				= NULL;
    dGx				= NULL;
    dGy				= NULL;
}



void
GaussianEdgeDetector::SetSizes()
{
    int sx = GetInputSizeX("INPUT");
    int sy = GetInputSizeY("INPUT");
    if (sx != unknown_size && sy != unknown_size)
    {
        SetOutputSize("OUTPUT", sx-2*filterradius, sy-2*filterradius);
        SetOutputSize("MAXIMA", sx-2*filterradius, sy-2*filterradius);
        SetOutputSize("NODES", sx-2*filterradius, sy-2*filterradius);
        SetOutputSize("ORIENTATION", sx-2*filterradius, sy-2*filterradius);
        SetOutputSize("dx", sx-2*filterradius, sy-2*filterradius);
        SetOutputSize("dy", sx-2*filterradius, sy-2*filterradius);
    }
}



void
GaussianEdgeDetector::Init()
{
    inputsize_x	 = GetInputSizeX("INPUT");
    inputsize_y	 = GetInputSizeY("INPUT");

    outputsize_x	= GetOutputSizeX("OUTPUT");
    outputsize_y	= GetOutputSizeY("OUTPUT");

    input		= GetInputMatrix("INPUT");
    output		= GetOutputMatrix("OUTPUT");
    maxima		= GetOutputMatrix("MAXIMA");
    nodes		= GetOutputMatrix("NODES");
    orientation	= GetOutputMatrix("ORIENTATION");

    dGx			= GetOutputMatrix("dGx");
    dGy			= GetOutputMatrix("dGy");

    dx			= GetOutputMatrix("dx");
    dy			= GetOutputMatrix("dy");

    // Initialize Gaussian Filters

    float Gsize = 1.0/scale;

    for (int j=0; j<filtersize; j++)
        for (int i=0; i<filtersize; i++)
        {
            float x = Gsize * float(i-filterradius);
            float y = Gsize * float(j-filterradius);
            dGx[j][i] = -2*x*exp(-(x*x + y*y));
            dGy[j][i] = -2*y*exp(-(x*x + y*y));
        }
}



GaussianEdgeDetector::~GaussianEdgeDetector()
{

}



void
GaussianEdgeDetector::Tick()
{
    int inc = 1;

    // Calculate dx

    for (int j =filterradius; j<inputsize_y-filterradius; j+=inc)
        for (int i=filterradius; i<inputsize_x-filterradius; i+=inc)
        {
            dx[j-filterradius][i-filterradius] = 0;
            for (int jj=-filterradius; jj<=filterradius; jj++)
                for (int ii=-filterradius; ii<=filterradius; ii++)
                    dx[j-filterradius][i-filterradius] += dGx[jj+filterradius][ii+filterradius] * input[j+jj][i+ii];
        }

    // Calculate dy

    for (int j =filterradius; j<inputsize_y-filterradius; j+=inc)
        for (int i=filterradius; i<inputsize_x-filterradius; i+=inc)
        {
            dy[j-filterradius][i-filterradius] = 0;
            for (int jj=-filterradius; jj<=filterradius; jj++)
                for (int ii=-filterradius; ii<=filterradius; ii++)
                    dy[j-filterradius][i-filterradius] += dGy[jj+filterradius][ii+filterradius] * input[j+jj][i+ii];
        }

    // Calculate edge intensity

    for (int j=0; j<outputsize_y; j++)
        for (int i=0; i<outputsize_x; i++)
            output[j][i] = hypot(dx[j][i], dy[j][i]);

    // Calculate Orientation

    for (int j=0; j<outputsize_y; j++)
        for (int i=0; i<outputsize_x; i++)
            orientation[j][i] = atan2(dx[j][i], dy[j][i]);

    // Non-maximum supression (TEST)

    copy_matrix(maxima, output, outputsize_x, outputsize_y);

    for (int j=1; j<outputsize_y-1; j++)
        for (int i=1; i<outputsize_x-1; i++)
            if (dx[j][i] != 0.0 || dy[j][i] != 0.0)
            {
                float h = hypot(dx[j][i], dy[j][i]);

                float d_x = dx[j][i]/h;
                float d_y = dy[j][i]/h;

                d_x = (d_x > 0.3827 ? 1.0 : (d_x < -0.3827 ? -1 : 0));
                d_y = (d_y > 0.3827 ? 1.0 : (d_y < -0.3827 ? -1 : 0));

                if (output[j][i] < output[j+int(d_y)][i+int(d_x)] || output[j][i] < output[j-int(d_y)][i-int(d_x)])
                    maxima[j][i] = 0.0;
            }
}



