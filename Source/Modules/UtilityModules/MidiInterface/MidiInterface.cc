//
//	MidiInterface.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2012 <Author Name>
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
//  This example is intended as a starting point for writing new Ikaros modules
//  The example includes most of the calls that you may want to use in a module.
//  If you prefer to start with a clean example, use he module MinimalModule instead.
//

#include "MidiInterface.h"
#include "RtMidi.h"
const int cMidiMsgSize = 3;
// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;

void
Callback( double deltatime, std::vector< unsigned char > *message, void *user /*userData*/ )
{
    unsigned int nBytes = message->size();
    MidiInterface *interface = reinterpret_cast<MidiInterface *>(user);
    if(interface->debugmode)
    {
        for (unsigned int i = 0; i < nBytes; i++)
            std::cout << "Byte " << i << " = " << (int)message->at(i) << ", ";
        if (nBytes > 0)
            std::cout << "stamp = " << deltatime << std::endl;
    }
    interface->SetMidiData(message);
}

void
MidiInterface::SetSizes()
{
    SetOutputSize("OUTPUT", cMidiMsgSize);
}
void
MidiInterface::Init()
{
    Bind(inport, "inport");
    Bind(outport, "outport");
    Bind(debugmode, "debug");    

    // TODO input
    io(input_array, input_array_size, "INPUT");
    //input_array = GetInputArray("INPUT");
    //input_array_size = GetInputSize("INPUT");
    io(output_array, output_array_size, "OUTPUT");
    //output_array = GetOutputArray("OUTPUT");
    //output_array_size = GetOutputSize("OUTPUT");

    internal_array = create_array(cMidiMsgSize);
    try{
        rtmidi_in = new RtMidiIn();
        rtmidi_in->openPort( inport );
        rtmidi_in->setCallback( &Callback , this);
        rtmidi_in->ignoreTypes( false, false, false );
    } catch ( RtMidiError &error ) {

        error.printMessage();

        unsigned int nPorts = rtmidi_in->getPortCount();
        
        std::cout << "\nThere are " << nPorts << " MIDI input sources available.\n";
        std::string portName;
        for ( unsigned int i=0; i<nPorts; i++ ) {
            try {
            portName = rtmidi_in->getPortName(i);
            }
            catch ( RtMidiError &error ) {
            error.printMessage();
            // goto cleanup;
            }
            std::cout << "  Input Port #" << i+1 << ": " << portName << '\n';
        }
    }
    if(input_array)
        try
        {
            rtmidi_out = new RtMidiOut();
            rtmidi_out->openPort(outport);
        }
        catch (RtMidiError &error)
        {
            error.printMessage();
        }
    // TODO print out all midi available inports

}



MidiInterface::~MidiInterface()
{
    // Destroy data structures that you allocated in Init.
    destroy_array(internal_array);
    delete rtmidi_in;
    if(rtmidi_out) delete rtmidi_out;
}



void
MidiInterface::Tick()
{
    // output
    copy_array(output_array, internal_array, output_array_size);
    // input, assumes input size is multiple of 3
    for (int i = 0; input_array && i < input_array_size; i+=cMidiMsgSize)
    {
        std::vector<unsigned char> msg;
        msg.push_back((unsigned char)input_array[i]);
        msg.push_back((unsigned char)input_array[i + 1]);
        msg.push_back((unsigned char)input_array[i + 2]);
        rtmidi_out->sendMessage(&msg);
    }
    
	if(debugmode)
	{
		// print out debug info
        printf("Instance name: %s\n", this->instance_name);

	}
}

void        
MidiInterface::SetMidiData(std::vector< unsigned char> *message)
{
    // TODO this could be done by copy array
    for (int i = 0; i < message->size() && i < cMidiMsgSize; i++)
        internal_array[i] = (float)message->at(i);
}


// Install the module. This code is executed during start-up.

static InitClass init("MidiInterface", &MidiInterface::Create, "Source/Modules/UtilityModules/MidiInterface/");


