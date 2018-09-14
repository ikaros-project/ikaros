//
//    ActionCompetition.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2018  Christian Balkenius
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


#include "ActionCompetition.h"

using namespace ikaros;

void
ActionCompetition::Init()
{
    output          = GetOutputArray("OUTPUT");
    output_size     = GetOutputSize("OUTPUT");

    trigger         = GetOutputArray("TRIGGER");

    input          = GetInputArray("INPUT");
    input_size     = GetInputSize("INPUT");

    complete        = GetInputArray("COMPLETE");
    complete_size    = GetInputSize("COMPLETE");
    
    counter         = GetIntValue("initial_delay");
    
    duration    = GetIntArray("duration", output_size, true);

    rest            = GetArray("rest", output_size, true);
    min_            = GetArray("min", output_size, true);
    max_             = GetArray("max", output_size, true);
    passive         = GetArray("passive", output_size, true);
    bias            = GetArray("bias", output_size, true);
    completion_bias  = GetArray("completion_bias", output_size, true);

    current_winner = -1;
}



void
ActionCompetition::Tick()
{
    for(int i=0; i<output_size; i++)
    {
        if(i<input_size)
            output[i] += input[i]*bias[i];

        if(i<complete_size)
            output[i] += complete[i]*completion_bias[i];

        output[i] += (rest[i]-output[i])*passive[i]; // rest
        output[i] = clip(output[i], min_[i], max_[i]);
    }

    if(counter == 0)
    {
        int last_winner = current_winner;
        current_winner = arg_max(output, input_size);
        reset_array(trigger, output_size);
        trigger[current_winner] = 1;
        counter = duration[current_winner];
    }
    else
        counter--;
}



static InitClass init("ActionCompetition", &ActionCompetition::Create, "Source/Modules/RobotModules/ActionCompetition/");

