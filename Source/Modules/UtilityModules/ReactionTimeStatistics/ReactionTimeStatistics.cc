//
//		ReactionTimeStatistics.cc		This file is a part of the IKAROS project
//				
//
//    Copyright (C) 2020 Christian Balkenius
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


#include "ReactionTimeStatistics.h"

using namespace ikaros;

void
ReactionTimeStatistics::Init()
{
    Bind(bins, "bins");
    Bind(max_rt, "max_rt");

    io(start, start_size, "START");
    io(stop, size, "STOP");

    io(rt_histogram, rt_size_x, rt_size_y, "RT-HISTOGRAM");
    io(rt_outlier, "CHOICE-COUNT");
    io(rt_mean, "RT-MEAN");
    io(choice_count, "CHOICE-COUNT");
    io(choice_probability, "CHOICE-PROBABILITY");
    io(startbalance_out, "STARTBALANCE");
    io(rt_out, "REACTIONTIME");

    rt_sum = create_array(size);
}



void
ReactionTimeStatistics::Tick()
{
    if(max(stop, size) > 0)
    {

        int c = arg_max(stop, size);
        int b = float(reaction_time)*(bins/max_rt);
        if(0 <= b && b < bins)
            rt_histogram[b][c] += 1;
        else
            rt_outlier[c] += 1;
       
        choice_count[c] += 1;
        copy_array(choice_probability, choice_count, size);
        normalize1(choice_probability, size);
        rt_sum[c] += reaction_time;
        for(int i=0; i<size; i++)
            if(choice_count[i]> 0)
                rt_mean[i] = rt_sum[i] / choice_count[i];
            else
                rt_mean[i] = 0;
        startbalance--;
    }

    if(max(start, start_size) > 0)
    {
        reaction_time = 0;
        startbalance++;
    }
    startbalance_out[0] = startbalance;
    reaction_time += 1;
}


void
ReactionTimeStatistics::Command(std::string s, float x, float y, std::string value)
{
    if(s == "reset")
    {
        reset_matrix(rt_histogram, rt_size_x, rt_size_y);
        reset_array(rt_sum, size);
        reset_array(rt_mean, size);
        reset_array(rt_outlier, size);
        reset_array(choice_count, size);
        reset_array(choice_probability, size);
    }
}

static InitClass init("ReactionTimeStatistics", &ReactionTimeStatistics::Create, "Source/Modules/UtilityModules/ReactionTimeStatistics/");


