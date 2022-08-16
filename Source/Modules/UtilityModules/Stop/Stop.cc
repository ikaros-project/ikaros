//
//	Stop.cc		This file is a part of the IKAROS project
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

#include "Stop.h"
#include <iostream> // print percentage complete
#include <iomanip>
// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;


void
Stop::Init()
{
    Bind(termination_criterion, "termination_criterion");
    Bind(wait, "wait");
    Bind(select, "select");
	Bind(debugmode, "debug");  
    comparator_ix = (eComparator)GetIntValueFromList("comparator", "less/greater/equal");  
    switch (comparator_ix){
        case eGreater:
            Compare = &Stop::Greater;
            CompareInverse = &Stop::LessEqual;
            break;
        case eEqual:
            Compare = &Stop::Equal;
            CompareInverse = &Stop::Equal;
            break;
        case eLess:
        default:
            Compare = &Stop::Less;
            CompareInverse = &Stop::GreaterEqual;
    }

    input_array = GetInputArray("INPUT");
    input_array_size = GetInputSize("INPUT");
    counter = 0;
    complete_perc = 0.f;
    

    
}



Stop::~Stop()
{
    // Destroy data structures that you allocated in Init.
    
}



void
Stop::Tick()
{
	if(debugmode)
	{
		// print out debug info
        printf("abs input: %f; termcrit: %f; counter > wait: %i; abs(input_array[select]) < termination_criterion: %i\n", 
            abs(input_array[select]), termination_criterion, (int)counter > wait, (int)(abs(input_array[select]) < termination_criterion));
	}
    const float tolerance = 0.1f;
    float complete_ratio = comparator_ix == eLess ? 
        termination_criterion / input_array[select] : 
        input_array[select] / termination_criterion;
    float percentage = 100.f * complete_ratio;
    if(!equal(percentage, complete_perc, tolerance))
    {
         std::cout << '\r'
            << std::setw(4) 
            << std::setprecision(3) 
            << percentage << " \% complete ---" << std::flush;
    }
    complete_perc  = (this->*CompareInverse)((percentage-complete_perc), tolerance) ? percentage : complete_perc;

    if((counter > wait) && (this->*Compare)(abs(input_array[select]), termination_criterion))
        Notify(msg_terminate, "Stop: Terminated because criterion was met.");
    counter++;
}

bool 
Stop::Less(float a, float b)
{
    return a < b;
}

bool 
Stop::Greater(float a, float b)
{
    return a > b;
}

bool 
Stop::Equal(float a, float b)
{
    return a == b;
}

bool 
Stop::GreaterEqual(float a, float b)
{
    return a >= b;
}

bool 
Stop::LessEqual(float a, float b)
{
    return a <= b;
}


// Install the module. This code is executed during start-up.

static InitClass init("Stop", &Stop::Create, "Source/UserModules/Stop/");


