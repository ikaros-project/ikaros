//
//	VariableRadiusAttentionalFocus.cc		This file is a part of the IKAROS project
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
//  This example is intended as a starting point for writing new Ikaros modules
//  The example includes most of the calls that you may want to use in a module.
//  If you prefer to start with a clean example, use he module MinimalModule instead.
//

#include "VariableRadiusAttentionalFocus.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;
const int cCENTER = 1;
const int cTOPLEFT = 0;

VariableRadiusAttentionalFocus::VariableRadiusAttentionalFocus(Parameter *p):
    Module(p)
{
    Bind(output_matrix_size_x, "size_x");
    Bind(output_matrix_size_y, "size_y");
    Bind(origo, "origo");
}

VariableRadiusAttentionalFocus::~VariableRadiusAttentionalFocus()
{
    // Destroy data structures that you allocated in Init.
    destroy_matrix(window);
}

void VariableRadiusAttentionalFocus::SetSizes()
{
    SetOutputSize("OUTPUT", output_matrix_size_x, output_matrix_size_y);

}

void VariableRadiusAttentionalFocus::Init()
{
    Bind(debugmode, "debug");

    input_matrix = GetInputMatrix("INPUT");
    input_matrix_size_x = GetInputSizeX("INPUT");
    input_matrix_size_y = GetInputSizeY("INPUT");
    x_pos = GetInputArray("X");
    y_pos = GetInputArray("Y");
    radius = GetInputArray("RADIUS");

    output_matrix = GetOutputMatrix("OUTPUT");

    window = create_matrix(output_matrix_size_x, output_matrix_size_y);
}

void
VariableRadiusAttentionalFocus::Tick()
{
    // map from fractions to size of input
    float x, y, rad_x, rad_y;
    map(&rad_x, radius, 0.f, 1.f, 0.f, (float)input_matrix_size_x, 1);
    map(&rad_y, radius, 0.f, 1.f, 0.f, (float)input_matrix_size_y, 1);
    int start_x, start_y;
    if(origo==cTOPLEFT)
    {
        map(&x, x_pos, 0.f, 1.f, 0.f, (float)input_matrix_size_x, 1);
        map(&y, y_pos, 0.f, 1.f, 0.f, (float)input_matrix_size_y, 1);
        start_x = min((int)x, input_matrix_size_x - (int)rad_x);
        start_y = min((int)y, input_matrix_size_y - (int)rad_y);
    }
    else // TODO fix centered
    {
        map(&x, x_pos, 0.f, 1.f, output_matrix_size_x/2, 
            (float)input_matrix_size_x - output_matrix_size_x/2, 1);
        map(&y, y_pos, 0.f, 1.f, output_matrix_size_y/2, 
            (float)input_matrix_size_y - output_matrix_size_y/2, 1);
        start_x = min((int)x - output_matrix_size_x/2, input_matrix_size_x - (int)rad_x/2);
        start_y = min((int)y - output_matrix_size_y/2, input_matrix_size_y - (int)rad_y/2);
    }    
    // calculate span
    int span_x = max((int)(rad_x/(float)output_matrix_size_x), 1);
    int span_y = max((int)(rad_y/(float)output_matrix_size_y), 1);
    

    // set start ix dept on span size - push left
    
    

    // calculate indices
    for(int j=0; j < output_matrix_size_y; ++j)
        for(int i=0; i < output_matrix_size_x; ++i)
        {
            int inj = min(start_y + j*span_y, input_matrix_size_y - 1);
            int ini = min(start_x + i*span_y, input_matrix_size_x - 1);
            output_matrix[j][i] = input_matrix[inj][ini];    
        }
    // copy values using take()
	if(debugmode)
	{
		// print out debug info
        printf("startx=%i; starty=%i; radiusx=%i; radiusy=%i; spanx=%i; spany=%i\n", 
            start_x, start_y, (int)rad_x, (int)rad_y, span_x, span_y );
	}
}



// Install the module. This code is executed during start-up.

static InitClass init("VariableRadiusAttentionalFocus", &VariableRadiusAttentionalFocus::Create, "Source/UserModules/VariableRadiusAttentionalFocus/");


