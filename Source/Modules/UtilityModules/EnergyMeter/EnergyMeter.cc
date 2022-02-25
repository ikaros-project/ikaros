//
//	EnergyMeter.cc		This file is a part of the IKAROS project
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


#include "EnergyMeter.h"

using namespace ikaros;

void
EnergyMeter::Init()
{
    io(measurered_power, "MEASURED_POWER");
    io(energy, "ENERGY");
}



void
EnergyMeter::Tick()
{
    try
    {
        auto data = socket.HTTPGet("192.168.50.59/status");
        *measurered_power = std::stof(split(data, "\"power\":", 1).at(1)); // Not exactly JSON parsing but it will do.
   }
    catch(const std::exception& e)
    {
        *energy = 0;
    }

    *energy += GetTickLength()*(*measurered_power) /(1000.0*1000.0*3600.0); // Convert time interval to hours and integrate to Wh
    //printf("ENERGY: %f, POWER: %f\n", *energy, *measurered_power);
}



static InitClass init("EnergyMeter", &EnergyMeter::Create, "Source/Modules/UtilityModules/EnergyMeter/");


