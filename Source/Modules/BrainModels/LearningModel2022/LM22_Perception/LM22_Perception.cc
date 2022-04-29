//
//		Perception.cc		This file is a part of the IKAROS project
//				
//
//    Copyright (C) 2016 Christian Balkenius
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


#include "Perception.h"

using namespace ikaros;

void
Perception::Init()
{
    Bind(alpha, "alpha");
    Bind(beta, "beta");
    Bind(phi, "phi");

    Bind(interval, "interval");

    io(location_in, o_size, "LOCATION_IN");
    io(environment, p_size, o_size, "ENVIRONMENT");
    io(features, f_size, p_size, "FEATURES");
    io(location_out, o_size, "LOCATION_OUT");
    io(output, f_size, "OUTPUT");
}


void
Perception::Tick()
{
//    if(interval && GetTick() % interval != 0)
//        return;

    float r = random(0.0, 1.0);

    if(r > 1/float(interval)) // epsilon
        return;

//
    // uniform selection if location_in is not connected

    if(!location_in)
    {
        // Select an object
        int o = random_int(0, o_size);
        // Select a random property - causes infinite loop if object row is zero!
        int p = random_int(0, p_size);
        while(environment[o][p] == 0)
            p = random_int(0, p_size);

        // Get the feature vector for the property
        copy_array(output, features[p], f_size);
        set_one(location_out, o, o_size);
    }

    else
    {
        for(int i=0; i<o_size; i++)
            location_in[i] = location_in[i]*location_in[i];

        printf("%f %f\n", location_in[0], location_in[1]);
        multiply(location_in, phi, o_size);
        add(location_in, 1, o_size);
        normalize1(location_in, o_size);
        float r = random(0.0f, 1.0f);
        
        int o = 0;
        float s = 0;
        for(int i=0; i<o_size; i++)
        {
            s += location_in[i];
            if(s > r)
            {
                o = i;
                break;
            }
        }

        // Select a random property - causes infinite loop if object row is zero!
        int p = random_int(0, p_size);
        while(environment[o][p] == 0)
            p = random_int(0, p_size);

        // Get the feature vector for the property
        copy_array(output, features[p], f_size);
        set_one(location_out, o, o_size);
    }
}

static InitClass init("Perception", &Perception::Create, "Source/Modules/BrainModels/DecisionModel2020/Perception/");


