//
//    Histogram.cc		This file is a part of the IKAROS project
//                  
//
//    Copyright (C) 2015  Christian Balkenius
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


#include "Histogram.h"

void
Histogram::Init()
{
    minimum = GetFloatValue("min");
    maximum = GetFloatValue("max");

    if(minimum == maximum)
        Notify(msg_fatal_error, "Max must be larger than min");

    ignore_outliers = GetBoolValue("ignore_outliers");

    input	= GetInputArray("INPUT");
    output	= GetOutputArray("OUTPUT");

    size = GetOutputSize("OUTPUT");
}



void
Histogram::Tick()
{
    int index = int(float(size) * (*input-minimum)/(maximum-minimum));

    if(index < 0)
    {
        if(ignore_outliers)
            return;
        else
            index = 0;
    }

    if(index >= size)
    {
        if(ignore_outliers)
            return;
        else
            index = size-1;
    }

    output[index] += 1.0;
}


static InitClass init("Histogram", &Histogram::Create, "Source/Modules/UtilityModules/Histogram/");
