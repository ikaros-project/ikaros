//
//	GaborFilter.cc		This file is a part of the IKAROS project
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



#include "GaborFilter.h"

using namespace ikaros;



Module *
GaborFilter::Create(Parameter * p)
{
    return new GaborFilter(p);
}



GaborFilter::GaborFilter(Parameter * p):
        Module(p)
{
    scale			= GetFloatValue("scale", 1.0);
    filterradius	= int(2*scale+0.5);
    filtersize		= 1+2*filterradius;

    gamma			= GetFloatValue("gamma", 0.5);	// aspect ratio
    lambda			= GetFloatValue("lambda", 4);	// wavelength
    theta			= GetFloatValue("theta", 0);		// orientation
    phi				= GetFloatValue("phi", 1.57);	// phase offset
    sigma			= GetFloatValue("sigma", 1);		// width


    AddInput("INPUT");
    AddOutput("OUTPUT");                                // Filter intensity
    AddOutput("FILTER", filtersize, filtersize);		// The filter used
    AddOutput("GAUSSIAN", filtersize, filtersize);
    AddOutput("GRATING", filtersize, filtersize);

    input			= NULL;
    output			= NULL;
    filter			= NULL;
    gaussian		= NULL;
    grating			= NULL;
}



void
GaborFilter::SetSizes()
{
    int sx = GetInputSizeX("INPUT");
    int sy = GetInputSizeY("INPUT");
    if (sx != unknown_size && sy != unknown_size)
    {
        SetOutputSize("OUTPUT", sx-2*filterradius, sy-2*filterradius);
        SetOutputSize("filter", sx-2*filterradius, sy-2*filterradius);
    }
}



void
GaborFilter::Init()
{
    inputsize_x	 	= GetInputSizeX("INPUT");
    inputsize_y	 	= GetInputSizeY("INPUT");

    outputsize_x	= GetOutputSizeX("OUTPUT");
    outputsize_y	= GetOutputSizeY("OUTPUT");

    input			= GetInputMatrix("INPUT");
    output			= GetOutputMatrix("OUTPUT");
    filter			= GetOutputMatrix("FILTER");
    gaussian		= GetOutputMatrix("GAUSSIAN");
    grating			= GetOutputMatrix("GRATING");

    // Initialize Gabor Filter

    for (int j=0; j<filtersize; j++)
        for (int i=0; i<filtersize; i++)
        {
            float x = float(i-filterradius);
            float y = float(j-filterradius);

            float xprime = x *cos(theta) + y*sin(theta);
            float yprime = -x*sin(theta) + y*cos(theta);

            gaussian[j][i] = exp(-(xprime*xprime + gamma*gamma*yprime*yprime)/(2*sigma*sigma)); //*cos(2*M_PI*xprime/lambda+phi);
            grating[j][i] = cos(2*pi*xprime/lambda+phi);
            filter[j][i] = gaussian[j][i]*grating[j][i];
        }
}



GaborFilter::~GaborFilter()
{

}



void
GaborFilter::Tick()
{
    int inc = 1;
    for (int j =filterradius; j<inputsize_y-filterradius; j+=inc)
        for (int i=filterradius; i<inputsize_x-filterradius; i+=inc)
        {
            output[j-filterradius][i-filterradius] = 0;
            for (int jj=-filterradius; jj<=filterradius; jj++)
                for (int ii=-filterradius; ii<=filterradius; ii++)
                    output[j-filterradius][i-filterradius] += filter[jj+filterradius][ii+filterradius] * input[j+jj][i+ii];
        }
    if (true)
    {
        for (int j=0; j<outputsize_y; j++)
            for (int i=0; i<outputsize_x; i++)
                if (output[j][i] < 0)
                    output[j][i] = - output[j][i];
    }
}



