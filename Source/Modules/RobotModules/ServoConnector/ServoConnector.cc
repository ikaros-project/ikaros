//
//	ServoConnector.cc		This file is a part of the IKAROS project
//
//
//    Copyright (C) 2016 Birger Johansson
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
//	Created: 2016
//

#include "ServoConnector.h"


using namespace ikaros;


void
ServoConnector::Init()
{
    connectorSize = 0;
    connector = GetIntArray("connector", connectorSize);
    
    invertedSize = 0;
    inverted = GetIntArray("inverted", invertedSize);

	offsetSize = 0;
	offset = GetIntArray("offset", offsetSize);
	
    input = GetInputArray("INPUT");
    inputSize = GetInputSize("INPUT");
    
    output = GetOutputArray("OUTPUT");
    outputSize = GetOutputSize("OUTPUT");

	// Check sizes here!
	
	if (!connector)
		Notify(msg_fatal_error, "Check connector parameters in ikc file.");

	if (!inverted)
		Notify(msg_fatal_error, "Check inverted parameters in ikc file.");

	if (!offset)
		Notify(msg_fatal_error, "Check offset parameters in ikc file.");
	
	if (outputSize != connectorSize)
		Notify(msg_fatal_error, "Output_size does not match connector size");

	if (outputSize != offsetSize)
		Notify(msg_fatal_error, "Output_size does not match offset size");

	if (outputSize != invertedSize)
		Notify(msg_fatal_error, "Output_size does not match inverted size");

}

ServoConnector::~ServoConnector()
{
}

void
ServoConnector::Tick()
{
    //printf("%s\n", GetName());
    //print_array("ServoConnector: input", input, inputSize);

	
    // Connector
    for (int i = 0; i < inputSize; i++)
        output[i] = input[connector[i]-1];
	
	//print_array("ServoConnector: output1", output, outputSize);

	// Offset
	for (int i = 0; i < inputSize; i++)
		output[i] = output[i] + offset[i];

	//print_array("ServoConnector: output2", output, outputSize);

    // Inverted
    for (int i = 0; i < inputSize; i++)
        if (inverted[i] == 1)
            output[i] = -output[i];
	
	//print_array("ServoConnector: output", output, outputSize);
}

static InitClass init("ServoConnector", &ServoConnector::Create, "Source/Modules/RobotModules/ServoConnector/");

