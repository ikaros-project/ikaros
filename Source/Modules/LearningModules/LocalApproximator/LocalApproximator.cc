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



void
LocalApproximator::Init()
{
    output_table = GetInputArray("OUTPUT_TABLE");
    input_table = GetInputArray("InPUT_TABLE");
    
    input = GetInputArray("INPUT");
    output = GetOutputArray("OUTPUT");
}



void
LocalApproximator::Tick()
{
/*
    if (distance_table[GetClosestIndex()] == 0)
        class_output[0] = output_table[GetClosestIndex()];
    else{
        if (categorical)
            class_output[0] = 0;
        else
            class_output[0] = CalculateMean();
    }
*/
}


float
LocalApproximator::CalculateMean()
{
/*
    float tot = 0, instances = 0;
    int i;

    for (i = 0; i < k; i++){
        tot += output_table[i] * GetWeightFactor(distance_table[i]);
        instances += 1 * GetWeightFactor(distance_table[i]);
    }

    return tot / instances;
*/
}



int
LocalApproximator::GetClosestIndex()
{
/*    int i, closest_index;

    closest_index = 0;

    for (i = 1; i < k; i++)
        if (distance_table[i] < distance_table[closest_index])
            closest_index = i;

    return closest_index;
*/
}



void
LocalApproximator::CheckParameters()
{
/*
    categorical = GetBoolValue("categorical", true);
    weighed = GetBoolValue("weighed", false);
    weight_divisor = GetFloatValue("weight_divisor", 1.0);
*/
}



static InitClass init("LocalApproximator", &LocalApproximator::Create, "Source/Modules/LearningModules/LocalApproximator/");
