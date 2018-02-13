//
//	YARPPort.cc		This file is a part of the IKAROS project
//                      Ikaros to Yarp network communication
//
//    Copyright (C) 2011 Birger Johansson
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
//	Created: 20111031
//
//  Module that can read and write to an yarp network.


#include "YARPPort.h"

#include <iostream>
//#define SYNC_DEBUG

using namespace ikaros;
using namespace std;
using namespace yarp::os;
using namespace yarp::os::impl;
using namespace yarp::sig;


// showBottle function from YARP exemple files.
void
YARPPort::showBottle(Bottle& anUnknownBottle, int indentation = 0) {
    for (int i=0; i<anUnknownBottle.size(); i++) {
        for (int j=0; j<indentation; j++) { printf(" "); }
        printf("[%d]: ", i);
        Value& element = anUnknownBottle.get(i);
        switch (element.getCode()) {
            case BOTTLE_TAG_INT:
                printf("int %d\n", element.asInt());
                break;
            case BOTTLE_TAG_DOUBLE:
                printf("float %g\n", element.asDouble());
                break;
            case BOTTLE_TAG_STRING:
                printf("string \"%s\"\n", element.asString().c_str());
                break;
            case BOTTLE_TAG_BLOB:
                printf("binary blob of length %lu\n", element.asBlobLength());
                break;
            //case BOTTLE_TAG_VOCAB:
            //    printf("vocab [%s]\n", Vocab::decode(element.asVocab()).c_str());
            //    break;
            default:
                if (element.isList()) {
                    Bottle *lst = element.asList();
                    printf("list of %d elements\n", lst->size());
                    showBottle(*lst,indentation+2);
                } else {
                    printf("unrecognized type\n");
                }
                break;
        }
    }
}


void
YARPPort::Init()
{
    // Get name of Yarp port from IKC file
    yarpName = GetValue("yarp_name");
    type    =   GetIntValueFromList("type");
    SenOnlyNewValues    =   GetBoolValue("send_only_new_values");
    sendStrict    =   GetBoolValue("send_strict");
    receiveStrict    =   GetBoolValue("receive_strict");
    
    size_x  = GetIntValue("outputsize_x", 0);
    size_y  = GetIntValue("outputsize_y", 0);
    
    // Creating IO
    input	=	GetInputArray("INPUT",false);
    output  =	GetOutputArray("OUTPUT",false);

    trigger =	GetOutputArray("YARP_ACTIVITY",false);

    // Init yarp network
    Network yarp;
    port.open(yarpName);
    
    // Create a internal array
    previousData = create_array(size_x*size_y);
    
    set_array(previousData, -1 ,size_x*size_y);
              
}

YARPPort::~YARPPort()
{
    // Yarp is handleing memory for communication.
    // When running this module through instruments it indicate a small memory leak in the ACE lib. 
    destroy_array(previousData);
}

void
YARPPort::YarpWrite()
{
    
    // Creating package to send.
    PortablePair<Bottle,Vector>  &pp = port.prepare();
    
    // We might get a PortablePair that is reused. This is way we need to clear it.
    
    /*
     * Access the object which will be transmitted by the next call to 
     * yarp::os::BufferedPort::write.
     * The object can safely be modified by the user of this class, to
     * prepare it.  Extra objects will be created or reused as 
     * necessary depending on the state of communication with the
     * output(s) of the port.  Be careful!  If prepare() gives you
     * a reused object, it is up to the user to clear the object if that is
     * appropriate.  
     * If you are sending yarp::os::Bottle objects, you may want to call
     * yarp::os::Bottle::clear(), for example.
     * YARP doesn't clear objects for you, since there are many
     * cases in which overwriting old data is suffient and reallocation
     * of memory would be unnecessary and inefficient.
     * @return the next object that will be written
     */
    
    pp.head.clear();
    pp.body.clear();
    
    Bottle desc;
    desc.addString("Ikaros");

#ifdef SYNC_DEBUG
    // DEBUGGING
    Bottle tick; //length of data
    tick.addString("tick");
    tick.addInt(int(GetTick()));
        pp.head.addList() = tick;
#endif
    
    Bottle len; //length of data
    len.addString("l");
    len.addInt(size_x*size_y);
    
    Bottle cols;
    cols.addString("c");
    cols.addInt(size_x);
    
    Bottle rows;
    rows.addString("r");
    rows.addInt(size_y);
    
    
    pp.head.addList() = desc;
    pp.head.addList() = len;
    pp.head.addList() = cols;
    pp.head.addList() = rows;
    
    // Clear memory
    desc.clear();
    len.clear();
    cols.clear();
    rows.clear();
    
    Vector v(size_x*size_y);
    for(int i = 0; i < size_x*size_y; i++) 
        v[i] = input[i]; // Use memcopy instead?
    
    pp.body = v;
    v.clear();
    
    if (sendStrict)
        port.writeStrict(); // Send every tick to YARP network
    else
        port.write(); // Send a tick only if yarp still not working with previous send.
    
    //printf("Sent output to %s...\n", port.getName().c_str());
}

void
YARPPort::YarpRead()
{
    
    if (receiveStrict)
        port.setStrict();     // Read every received message.
    
    PortablePair<Bottle,Vector> *pp = port.read(false);
    
    if (pp!=NULL)
    {
        // We got some data
        trigger[0] = 1;

        //printf("**** HEAD **** ");
        //showBottle(pp->head,0);
        //printf("**** BODY **** ");
        //showBottle(pp->body,0);
        
        // Check tick
        //printf("%i (%i)\n", pp->head.find("tick").asInt(), GetTick());
#ifdef SYNC_DEBUG
        if ( pp->head.find("tick").asInt() != GetTick()-1)
        {
            printf("%i (%i)\n", pp->head.find("tick").asInt(), int(GetTick()));
            printf("Yarp doesn't cope with this ikaros speed. Ticks are ignored.\n");
        }
#endif
        Vector &v =  pp->body;
        //printf("Data:");
        for (int i = 0; i < size_x*size_y; i++)
            output[i] = v[i];
        
        // Store new data 
        copy_array(previousData, output, size_x*size_y);

    }
    else
        trigger[0] = 0;
}

void
YARPPort::Tick()
{
    
    // Some logic to not send to network each tick.
    if (type == 0)
        for (int i = 0; i < size_x*size_y; i++) 
            if (input[i] != previousData[i])
                SendYARP = true;
    
    if (!SenOnlyNewValues)
        SendYARP = true;
        
    //printf("%s\n",yarpName);
    //if (type == 0)
    //    YarpWrite();
    //else if (type == 1)
    //    YarpRead();
    //printf("%s end\n",yarpName);

    if (type == 0)
    {
        if (SendYARP)
        {
            //printf("%s sending (%li) %i \n",yarpName, GetTick(), SenOnlyNewValues);
            YarpWrite();
            SendYARP = false;
            copy_array(previousData, input, size_x*size_y);
            trigger[0] = 1;
        }
    }
    else if (type == 1)
    {
        YarpRead();
        ReceiveYARP = false;
    }
}

static InitClass init("YARPPort", &YARPPort::Create, "Source/Modules/IOModules/Network/YARPPort/");


