//
//	MinimalModule.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2012 <Author Name>
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
//    See http://www.ikaros-project.org/ for more information.
//


#include "SaliencePoints.h"

using namespace ikaros;

void
SaliencePoints::Init()
{
    Bind(sigma, "sigma");
    Bind(scale, "scale");
    
    select = GetFloatValue("select");
    if(equal_strings(GetValue("select_salience"),"none"))
        select_salience = -1;
    else
        select_salience = GetIntValue("select_salience");
    
    count = GetInputArray("COUNT", false);
    input = GetInputMatrix("INPUT");
    input_size_x = GetInputSizeX("INPUT");
    input_size_y = GetInputSizeY("INPUT");
    
    output = GetOutputMatrix("OUTPUT");
    size_x = GetOutputSizeX("OUTPUT");
    size_y = GetOutputSizeY("OUTPUT");
}



void
SaliencePoints::Tick()
{
    int c = input_size_y;
    if(count)
        c = int(*count);
    
    reset_matrix(output, size_x, size_y);
    
    for(int p=0; p<c; p++)
    {
        float x = input[p][select]*float(size_x);
        float y = input[p][select+1]*float(size_y);
        float s = (select_salience == -1 ? scale : scale * input[p][select_salience]);
        
        for (int j=0; j<size_y; j++)
			for (int i=0; i<size_x; i++)
				output[j][i] += s * gaussian1(hypot(x-float(i), y-float(j)), sigma);
    }
}



static InitClass init("SaliencePoints", &SaliencePoints::Create, "Source/Modules/VisionModules/SaliencePoints/");


