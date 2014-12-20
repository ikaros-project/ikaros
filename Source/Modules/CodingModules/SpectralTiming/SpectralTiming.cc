//
//	SpectralTiming.cc	This file is a part of the IKAROS project
//
//
//    Copyright (C) 2014  Christian Balkenius
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



#include "SpectralTiming.h"

using namespace ikaros;

void
SpectralTiming::Init()
{
    Bind(sigma, "sigma");
    Bind(decay, "decay");
    
    input       =   GetInputArray("INPUT");
    trace       =   GetOutputArray("TRACE");
    output      =   GetOutputMatrix("OUTPUT");

    size_x      =   GetOutputSizeX("OUTPUT");
    size_y      =   GetOutputSizeY("OUTPUT");
}



void
SpectralTiming::Tick()
{
    multiply(trace, decay, size_x);
    max(trace, input, size_x);

    if(sigma == 0 || size_y < 2)
        return;
    
    for(int i=0; i<size_x; i++)
    {
        float yt = trace[i];

        for(int j=0; j<size_y; j++)
            output[j][i] = yt*exp(-sqr(yt-float(j+1)/float(size_y))/(2*sigma*sigma));
    }
}



static InitClass init("SpectralTiming", &SpectralTiming::Create, "Source/Modules/CodingModules/SpectralTiming/");

