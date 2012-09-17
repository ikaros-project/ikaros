//
//	CannyEdgeDetector.cc	This file is a part of the IKAROS project
//							A module to find edges in an image
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

#include "CannyEdgeDetector.h"

using namespace ikaros;



Module *
CannyEdgeDetector::Create(Parameter * p)
{
    return new CannyEdgeDetector(p);
}



CannyEdgeDetector::CannyEdgeDetector(Parameter * p):
        Module(p)
{
    scale			= GetFloatValue("scale", 1.0);
    filterradius	= int(2*scale+0.5);
    filtersize      = 1+2*filterradius;

    AddInput("INPUT");

    AddOutput("EDGES");				// Edge magnitude
    AddOutput("MAXIMA");				// Orientation estimation
    AddOutput("OUTPUT");				// Final edges
    AddOutput("dx");					// Gradient estimation and categorization
    AddOutput("dy");
    AddOutput("dGx", filtersize, filtersize);	// The filters used
    AddOutput("dGy", filtersize, filtersize);

    input			= NULL;
    edges			= NULL;
    maxima			= NULL;
    output			= NULL;
    dx				= NULL;
    dy				= NULL;
    dGx				= NULL;
    dGy				= NULL;

    T0	= GetFloatValue("T0", 100);
    T1	= GetFloatValue("T1", 200);
    T2	= GetFloatValue("T2", 800);
}



void
CannyEdgeDetector::SetSizes()
{
    int sx = GetInputSizeX("INPUT");
    int sy = GetInputSizeY("INPUT");
    if (sx != unknown_size && sy != unknown_size)
    {
        SetOutputSize("EDGES", sx-2*filterradius, sy-2*filterradius);	// CHECK LATER!!! ***
        SetOutputSize("MAXIMA", sx-2*filterradius, sy-2*filterradius);
        SetOutputSize("OUTPUT", sx-2*filterradius, sy-2*filterradius);
        SetOutputSize("dx", sx-2*filterradius, sy-2*filterradius);
        SetOutputSize("dy", sx-2*filterradius, sy-2*filterradius);
    }
}



void
CannyEdgeDetector::Init()
{
    inputsize_x	 	= GetInputSizeX("INPUT");
    inputsize_y	 	= GetInputSizeY("INPUT");

    outputsize_x	 	= GetOutputSizeX("OUTPUT");
    outputsize_y	 	= GetOutputSizeY("OUTPUT");

    input				= GetInputMatrix("INPUT");

    edges			= GetOutputMatrix("EDGES");
    maxima			= GetOutputMatrix("MAXIMA");
    output			= GetOutputMatrix("OUTPUT");

    dGx				= GetOutputMatrix("dGx");
    dGy				= GetOutputMatrix("dGy");

    dx				= GetOutputMatrix("dx");
    dy				= GetOutputMatrix("dy");

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



CannyEdgeDetector::~CannyEdgeDetector()
{
}



void
CannyEdgeDetector::Tick()
{
    // Calculate dx

    for (int j =filterradius; j<inputsize_y-filterradius; j+=1)
        for (int i=filterradius; i<inputsize_x-filterradius; i+=1)
        {
            dx[j-filterradius][i-filterradius] = 0;
            for (int jj=-filterradius; jj<=filterradius; jj++)
                for (int ii=-filterradius; ii<=filterradius; ii++)
                    dx[j-filterradius][i-filterradius] += dGx[jj+filterradius][ii+filterradius] * input[j+jj][i+ii];
        }

    // Calculate dy

    for (int j =filterradius; j<inputsize_y-filterradius; j+=1)
        for (int i=filterradius; i<inputsize_x-filterradius; i+=1)
        {
            dy[j-filterradius][i-filterradius] = 0;
            for (int jj=-filterradius; jj<=filterradius; jj++)
                for (int ii=-filterradius; ii<=filterradius; ii++)
                    dy[j-filterradius][i-filterradius] += dGy[jj+filterradius][ii+filterradius] * input[j+jj][i+ii];
        }

    // Calculate edge intensity

    for (int j=0; j<outputsize_y; j++)
        for (int i=0; i<outputsize_x; i++)
            edges[j][i] = hypot(dx[j][i], dy[j][i]);

    // Classify Orientation (compare to sin(pi/8))

    float t;
    for (int j=0; j<outputsize_y; j++)
        for (int i=0; i<outputsize_x; i++)
            if (edges[j][i] > T0)
            {
                t = dx[j][i]  / edges[j][i];
                dx[j][i] = (t > 0.3827 ? 1.0 : (t < -0.3827 ? -1 : 0));
                t = dy[j][i]  / edges[j][i];
                dy[j][i] =  (t > 0.3827 ? 1.0 : (t < -0.3827 ? -1 : 0));
            }
            else
            {
                edges[j][i] = 0;
                dx[j][i] = 0;
                dy[j][i] = 0;
            }

    // Nonmaximum supression (rotate gradient vector by exchanging coordinates and signs)

    copy_matrix(maxima, edges, outputsize_x, outputsize_y);
    for (int j=1; j<outputsize_y-1; j++)
        for (int i=1; i<outputsize_x-1; i++)
            if (dx[j][i] != 0.0 || dy[j][i] != 0.0)
                if (edges[j][i] < edges[j+int(dy[j][i])][i+int(dx[j][i])] || edges[j][i] < edges[j-int(dy[j][i])][i-int(dx[j][i])])
                    maxima[j][i] = 0.0;

    // Hysteresis thresholding

    reset_matrix(output, outputsize_y, outputsize_x);
    for (int j=1; j<outputsize_y-1; j++)
        for (int i=1; i<outputsize_x-1; i++)
            if (maxima[j][i] > T2)	// && (int(dx[j][i]) != 0 || int(dy[j][i]) != 0)
            {
                output[j][i] = 1;

                // Extend edge in gradient direction

                int jj = j + int(dx[j][i]);
                int ii = i - int(dy[j][i]);

                while (0<ii && ii<outputsize_x && 0<jj && jj<outputsize_y && T1 < maxima[jj][ii] && maxima[jj][ii]  < T2)
                {
                    output[jj][ii] = 1;
                    jj += int(dx[j][i]);
                    ii -= int(dy[j][i]);
                }

                // Extend edge in opposite gradient direction

                jj = j -  int(dx[j][i]);
                ii = i +  int(dy[j][i]);

                while (0<ii && ii<outputsize_x && 0<jj && jj<outputsize_y && T1 < maxima[jj][ii] && maxima[jj][ii]  < T2)
                {
                    output[jj][ii] = 1;
                    jj -= int(dx[j][i]);
                    ii += int(dy[j][i]);
                }
            }
}



