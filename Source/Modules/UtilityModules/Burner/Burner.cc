//
//	Burner.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2022 Christian Balkenius
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


#include "Burner.h"

using namespace ikaros;

void
Burner::Init()
{
    Bind(percent, "percent");
    throttling = GetOutputArray("THROTTLING");
}



void
Burner::Tick()
{
    long long counter = 0;
    Timer timer;
    float tl = GetTickLength();
    while(timer.GetTime() < 0.01*percent*GetTickLength())
        counter++; // Burn!

    if(GetTick() < 10)
        cycle_estimation += counter;

    else if (cycle_estimation > 0)
    {
        float th = 1.0 - float(counter) / (0.1*float(cycle_estimation));
        if(th < 0)
            th = 0;
        if(th > 1)
            th = 1;
        *throttling = th;
    }
}


static InitClass init("Burner", &Burner::Create, "Source/Modules/UtilityModules/Burner/");


