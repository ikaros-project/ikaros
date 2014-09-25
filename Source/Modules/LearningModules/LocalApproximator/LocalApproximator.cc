//
//	LocalApproximator.cc	This file is a part of the IKAROS project
//
//
//    Copyright (C) 2014 Christian Balkenius
//    based on KNN_Pick Copyright (C) 2007 Alexander Kolodziej 
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

#include "LocalApproximator.h"



static inline bool
linear_regression(float & a, float & b, float * x, float * y, int c) // TODO: Move to IKAROS_Math
{
    double sx = 0.0;
    double sx2 = 0.0;
    double sxy = 0.0;
    double sy = 0.0;
    double n = double(c);

    for(int i=0; i<c; i++)
    {
        sx  += x[i];
        sx2 += x[i] * x[i];
        sxy += x[i] * y[i];
        sy  += y[i];
    }

    double Sxx = n*sx2 - (sx*sx);
    double Sxy = n*sxy - (sx*sy);
    
    if(Sxx > 0.0 && n > 0)  // calculate y = ax+b
    {
        a = Sxy/Sxx;
        b = (sy-a*sx)/n;
        return true;
    }
    else // too close to the y-axis
    {
        a = 0;
        b = 0;
        return false;
    }
}




void
LocalApproximator::Init()
{
    output_table = GetInputArray("OUTPUT_TABLE");
    input_table = GetInputArray("INPUT_TABLE");
    
    input = GetInputArray("INPUT");
    output = GetOutputArray("OUTPUT");
    
    input_table_size_x = GetInputSizeX("INPUT_TABLE");
    input_table_size_y = GetInputSizeY("INPUT_TABLE");

    output_table_size_x = GetInputSizeX("OUTPUT_TABLE");
    output_table_size_y = GetInputSizeY("OUTPUT_TABLE");
    
    input_size      = GetInputSizeX("INPUT_TABLE");
    output_size     = GetOutputSizeX("OUTPUT_TABLE");
}



void
LocalApproximator::Tick()
{
    for(int j=0; j<output_size; j++)
    {
        float [
    
    }
}



static InitClass init("LocalApproximator", &LocalApproximator::Create, "Source/Modules/LearningModules/LocalApproximator/");
