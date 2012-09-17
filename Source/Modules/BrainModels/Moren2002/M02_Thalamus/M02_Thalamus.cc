//
//	M02_Thalamus.cc		This file is a part of the IKAROS project
//						A module implementing a simplistic model of the Thalamus
//
//    Copyright (C) 2002  Jan Moren
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
//	Created: 2002-01-05 
//
//	2002-12-25 Converted for new simulator
//  2009-06-22 Moved to Ikaros 1.2 (CB)
//
//	A fake Thalamus module. All it does is add a new output of the maximum over
//	all inputs. Do not take this as a serious model of thalamic function!

#include "M02_Thalamus.h"

using namespace ikaros;

void M02_Thalamus::Init()
{
	size		 	= GetInputSize("INPUT");
	input			= GetInputArray("INPUT");
	Th				= GetOutputArray("TH");
	output			= GetOutputArray("OUTPUT");
	
}


void M02_Thalamus::Tick()
{
    copy_array(output, input, size);
	Th[0] = max(input, size);
}


