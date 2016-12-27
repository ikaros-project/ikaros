//
//	DataConverter.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2014 Birger Johansson
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
//  This module converts between Euler angles, matrixes and quaternion
//

#include "DataConverter.h"

using namespace ikaros;


void
DataConverter::Init()
{
    inputMatrix = GetInputMatrix("INPUT");
    outputMatrix = GetOutputMatrix("OUTPUT");
	
	inputMatrixSizeX = GetInputSizeX("INPUT");
	inputMatrixSizeY = GetInputSizeY("INPUT");
	
	outputMatrixSizeX = GetOutputSizeX("OUTPUT");
	outputMatrixSizeY = GetOutputSizeY("OUTPUT");

	// Check input and output size
	if (inputMatrixSizeX*inputMatrixSizeY != outputMatrixSizeX*outputMatrixSizeY)
		Notify(msg_fatal_error,"Input size does not match output size. %i %i = %i %i",inputMatrixSizeX,inputMatrixSizeY,outputMatrixSizeX,outputMatrixSizeY);
}

void
DataConverter::Tick()
{
	copy_array(*outputMatrix, *inputMatrix, inputMatrixSizeX*inputMatrixSizeY);
}

static InitClass init("DataConverter", &DataConverter::Create, "Source/Modules/UtilityModules/DataConverter/");


