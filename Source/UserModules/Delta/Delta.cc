//
//      Delta.cc		This file is a part of the IKAROS project
//				
//
//      Copyright (C) 2016 Christian Balkenius
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either version 2 of the License, or
//      (at your option) any later version.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//


#include "Delta.h"

using namespace ikaros;

void
Delta::Init()
{
    Bind(alpha, "alpha");
    Bind(beta, "beta");
    Bind(gamma, "gamma");
    Bind(delta, "delta");
    Bind(epsilon, "epsilon");
    
    Bind(inverse, "inverse");

    cs	=	GetInputArray("CS");
    us	=	GetInputArray("US");

    cs_size =	GetInputSize("CS");
    us_size =	GetInputSize("US");

    cr		=	GetOutputArray("CR");
    
    w = create_array(cs_size);
}



void
Delta::Tick()
{
    float US = sum(us, us_size);
    float x = dot(cs, w, cs_size);  // No clip!!!
    float R = alpha*(US-x);
    
    if(R>0)
        for(int i=0; i<cs_size; i++)
            w[i] += cs[i]*R;

    *cr = x;
}



/* // AMYGDALA
void
Delta::Tick()
{
    float US = sum(us, us_size);
    float x= clip(dot(cs, w, cs_size), 0, 1);  // Amygdala
    float R = alpha*(US-x);
    
    if(GetName()[0] == 'C')
        printf("%s: %f & %f + %f -> %f\t(%f %f)\n", GetName(), cs[5], cs[4], US, R, w[5], w[4]);
    
    if(R>0)
        for(int i=0; i<cs_size; i++)
            w[i] += cs[i]*R;
    *cr = x;
}

*/

// Cerebellum
/*
void
Delta::Tick()
{
    float US = sum(us, us_size);
//    float x= clip(dot(cs, w, cs_size), 0, 1);  // Amygdala
    float x= dot(cs, w, cs_size); // Cerebellum
    float R = alpha*(US-x);
    
    if(GetName()[0] == 'C')
        printf("%s: %f & %f + %f -> %f\t(%f %f)\n", GetName(), cs[5], cs[4], US, R, w[5], w[4]);
    
    if(R>0)
        for(int i=0; i<cs_size; i++)
            w[i] += cs[i]*R;
    
    if(x - US > 0)
        *cr = x - US;
    else
        *cr = 0;
}
*/



static InitClass init("Delta", &Delta::Create, "Source/UserModules/Delta/");


