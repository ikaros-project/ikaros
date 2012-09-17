//
//	M02_SensoryCortex.cc		This file is a part of the IKAROS project
//                              This module implemets a sensory cortical surface as a multi-peak SOM
//
//    Copyright (C) 2001-2002  Christian Balkenius
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
//	Created: 2001-10-22
//  2009-06-20 Moved to Ikaros 1.2 (CB)
//

#include "M02_SensoryCortex.h"


void
M02_SensoryCortex::Init()
{
	size    = GetInputSize("INPUT");
	input   = GetInputArray("INPUT");
	output  = GetOutputArray("OUTPUT");
}



void
M02_SensoryCortex::Tick()
{
    copy_array(output, input, size);
}


