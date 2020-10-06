//
//	OscInterface.cc		This file is a part of the IKAROS project
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
//  Uses oscpack: http://www.rossbencina.com/code/oscpack?q=~rossb/code/oscpack/

#include "OscInterface.h"
#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"
#include "ip/IpEndpointName.h"
// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;

const char cDelimiter = ';';
const int OUTPUT_BUFFER_SIZE = 512;
OscInterface::OscInterface(Parameter *P):
Module(P)
{
    std::vector<std::string> inadrvec;
    std::vector<std::string>  outadrvec;
    // printf("constr 1\n");
    const char* tmp = GetValue( "inadresses");
    // printf("constr 1, %s\n", tmp);
    if(tmp)
        inadrvec = split_string(tmp, cDelimiter);
    // printf("constr 2\n");
    tmp = GetValue("outadresses");
    if(tmp)
        outadrvec = split_string(tmp, cDelimiter);
    // inputs are sent to out adresses and vice versa
    ins = outadrvec.size();
    outs = inadrvec.size();
    // TODO let io names reflect adresses
    input_name = new char *[ins];
    output_name = new char *[outs];

    input = new float *[ins];
    output = new float *[outs];
    // printf("constr 3, ins=%i, outs=%i\n", ins, outs);
    for (int i = 0; i < ins; i++)
        AddInput(input_name[i] = create_formatted_string("INPUT_%d", i + 1));
    for (int i = 0; i < outs; i++)
        AddOutput(output_name[i] = create_formatted_string("OUTPUT_%d", i + 1));
    // add other IO channels here
    // AddOutput("OUTPUT");
}

OscInterface::~OscInterface()
{
    for (int i = 0; i < ins; i++)
        destroy_string(input_name[i]);
    for (int i = 0; i < outs; i++)
        destroy_string(output_name[i]);
    delete[] input_name;
    delete[] output_name;
    delete[] input;
    delete[] output;
    delete[] input_size_x;
    delete[] input_size_y;
    //destroy_array(internal_array);
    delete transmitSocket;
}

void 
OscInterface::SetSizes()
{
    // TODO support variable size specified in param
    int sx = 1;
    int sy = 1;

    // assumes all inputs same size, change if otherwise
    /*
    for (int i = 0; i < ins; i++)
    {
        int sxi = GetInputSizeX(input_name[i]);
        int syi = GetInputSizeY(input_name[i]);

        if (sxi == unknown_size)
            continue; // Not ready yet

        if (syi == unknown_size)
            continue; // Not ready yet

        if (sx != 0 && sxi != 0 && sx != sxi)
            Notify(msg_fatal_error, "Inputs have different sizes");

        if (sy != 0 && syi != 0 && sy != syi)
            Notify(msg_fatal_error, "Inputs have different sizes");

        sx = sxi;
        sy = syi;
    }
    */
    // printf("Setsizes 1\n");
    if (sx == unknown_size || sy == unknown_size)
        return; // Not ready yet
    for (int i = 0; i < outs; i++)
        SetOutputSize(output_name[i], sx, sy);
    // other sizes here
    size_x=sx;
    size_y=sy;
}

void
OscInterface::Init()
{
    // Bind(parameter, "parameter1");
    // printf("init 1\n");
    // inadresses are sent to outs and vice versa
    if(outs > 0)
        inadresses = split_string(GetValue("inadresses"), cDelimiter);
    //printf("init 2\n");
    if(ins > 0)
        outadresses = split_string(GetValue("outadresses"), cDelimiter);
    Bind(debugmode, "debug");

    outhost = GetValue("outhost");
    input_size_x = new int[ins];
    input_size_y = new int[ins];
    for (int i = 0; i < ins; i++)
    {
        input[i] = GetInputArray(input_name[i]);
        input_size_x[i] = GetInputSizeX(input_name[i]);
        input_size_y[i] = GetInputSizeY(input_name[i]);
    }
    //printf("init 3\n");
    for (int i = 0; i < outs; i++)
        output[i] = GetOutputArray(output_name[i]);
    //printf("init 4\n");
    // other outputs etc
    // io(output_array, output_array_size, "OUTPUT");
    Bind(inport, "inport");
    Bind(outport, "outport");
    Bind(show_unhandled, "show_unhandled");
    //internal_array = create_array(10);
    //insock.connectTo(host, port);
    if(outs>0)
    {
        insock.bindTo(inport);
        if (!insock.isOk()) {
            printf("OscInterface: Error opening port %i : %s \n", inport, insock.errorMessage().c_str());
        } else {
            printf("OscInterface: Listener started, will listen to packets on port %i \n", outport);
        }
    }

    if(ins>0)
    {
        // TODO open outhost
        // outsock.bindTo(outport);
            
        
        transmitSocket = new UdpTransmitSocket ( IpEndpointName( outhost.c_str(), outport ) );
        //if(transmitSocket->IsBound())
        printf("OscInterface: Transmitter started, will transmit packets to host %s, port %i \n", outhost.c_str(), outport);
        //else
        //    printf("OscInterface: Transmitter socket not bound to host %s, port %i \n", outhost.c_str(), outport);
        
    }
}



void
OscInterface::Tick()
{
    // iterate over inputs and outputs as required
    Send();
    Receive();
    if (debugmode)
    {
        printf("Instance name: %s", this->instance_name);
        // print out debug info
        // for (int i = 0; i < ins; i++)
        //     print_matrix(input_name[i], input[i], size_x, size_y);
    }
}

void
OscInterface::Send()
{
    //if (outadresses.size() == 0)
    //    return;
    //if(transmitSocket->.isOk())
    //{
        // TODO
        for (int i = 0; i < outadresses.size(); i++)
        {
            std::string address_string = outadresses.at(i);
            // formatting messages into a packet for sending:
            

            char buffer[OUTPUT_BUFFER_SIZE];
            osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );

            p << osc::BeginBundleImmediate
                << osc::BeginMessage( address_string.c_str() );
                //<< osc::Blob(input[i], input_size_x[i])
                
            //for(int jj=0; jj<input_size_y; jj++)
            for(int ii=0; ii<input_size_x[i]; ii++)
                p << input[i][ii];
            p << osc::EndMessage
                << osc::EndBundle;

            
            transmitSocket->Send( p.Data(), p.Size() );

            // processing incoming messages
        }
    //}
    //else
    //    printf("OscInterface: outsock is not ok\n");
}

void
OscInterface::Receive()
{
    if (inadresses.size() == 0)
        return;
    if(insock.isOk())
    {
       //printf("OscInterface: insock is ok\n");  
      if (insock.receiveNextPacket(30 /* timeout, in ms */)) {
        // printf("OscInterface: packet received\n");  
        pr.init(insock.packetData(), insock.packetSize());
        oscpkt::Message *msg;
        while (pr.isOk() && (msg = pr.popMessage()) != 0) {
          //printf("OscInterface: going through message\n");    
          float iarg;

          // TODO iterate over addresses
          for (int i = 0; i < inadresses.size(); i++)
          {
            std::string address_string = inadresses.at(i);
            if (msg->match(address_string).popFloat(iarg).isOkNoMoreArgs()) {
                output[i][0] = iarg;
                //printf("OscInterface: received /lfo %f from %s\n", iarg, insock.packetOrigin().asString().c_str());
                //oscpkt::Message repl; repl.init("/pong").pushInt32(iarg+1);
                //pw.init().addMessage(repl);
                //insock.sendPacketTo(pw.packetData(), pw.packetSize(), insock.packetOrigin());
            } else if (show_unhandled){
                printf("OscInterface: unhandled message: %s\n", msg->asString().c_str() );
                //cout << "Server: unhandled message: " << *msg << "\n";
            }
          }
        }
      }  
    }
    else
        printf("OscInterface: insock is not ok\n"); 
}


// Install the module. This code is executed during start-up.

static InitClass init("OscInterface", &OscInterface::Create, "Source/Modules/UtilityModules/OscInterface/");
