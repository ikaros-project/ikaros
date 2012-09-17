//
//	HarrisDetector.cc		This file is a part of the IKAROS project
//						A module to estimate curvature in an image
//
//    Copyright (C) 2004  Christian Balkenius
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

#include "HarrisDetector.h"
#include "ctype.h"

using namespace ikaros;

#define limit(x, x0, x1)	((x) < (x0) ? (x0) : ((x) > (x1) ? (x1) : (x)))



Module *
HarrisDetector::Create(Parameter * p)
{
    return new HarrisDetector(p);
}



HarrisDetector::HarrisDetector(Parameter * p):
        Module(p)
{
    dx				= NULL;
    dy				= NULL;
    output			= NULL;

    // Add inputs and outputs

    AddInput("DX");
    AddInput("DY");

    AddOutput("OUTPUT");
}



void
HarrisDetector::SetSizes()
{
    int sx = GetInputSizeX("DX");
    int sy = GetInputSizeY("DX");
    if (sx != unknown_size && sy != unknown_size)
    {
        SetOutputSize("OUTPUT", sx, sy);
    }
}



void
HarrisDetector::Init()
{
    size_x	 	= GetInputSizeX("DX");
    size_y	 	= GetInputSizeY("DX");

    dx			= GetInputMatrix("DX");
    dy			= GetInputMatrix("DY");

    output		= GetOutputMatrix("OUTPUT");

    dx2 = create_matrix(size_x, size_y);
    dy2 = create_matrix(size_x, size_y);
    dxy = create_matrix(size_x, size_y);
    dxs = create_matrix(size_x, size_y);
    dys = create_matrix(size_x, size_y);
    dxys = create_matrix(size_x, size_y);
}



HarrisDetector::~HarrisDetector()
{
    destroy_matrix(dx2);
    destroy_matrix(dy2);
    destroy_matrix(dxy);
    destroy_matrix(dxs);
    destroy_matrix(dys);
    destroy_matrix(dxys);
}



void
HarrisDetector::Tick()
{
    for (int j =0; j<size_y; j++)
        for (int i=0; i<size_x; i++)
        {
            dxy[j][i] = dx[j][i] * dy[j][i];
            dx2[j][i] = dx[j][i] * dx[j][i];
            dy2[j][i] = dy[j][i] * dy[j][i];
        }

    // Local propagation (region size = 2+2+1 = 5)

    reset_matrix(dxs, size_x, size_y);
    reset_matrix(dys, size_x, size_y);
    reset_matrix(dxys, size_x, size_y);

    for (int j =2; j<size_y-2; j++)
        for (int i=2; i<size_x-2; i++)
        {
            for (int l=-2; l<2; l++)
                for (int k=-2; k<2; k++)
                {
                    dxs[j][i] += dx2[j+l][i+k];
                    dys[j][i] += dy2[j+l][i+k];
                    dxys[j][i] += dxy[j+l][i+k];
                }
        }

    // Set corner strength as output; do not find local maxima yet

    reset_matrix(output, size_x, size_y);
    for (int j =2; j<size_y-2; j++)
        for (int i=2; i<size_x-2; i++)
            output[j][i] = (dxs[j][i]*dys[j][i] - dxys[j][i]*dxys[j][i]) / (dxs[j][i] + dys[j][i]  + 0.00001);
}


