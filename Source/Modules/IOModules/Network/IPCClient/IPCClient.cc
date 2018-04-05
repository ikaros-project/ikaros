//
//	IPCClient.cc		This file is a part of the IKAROS project
// 					<Short description of the module>
//
//    Copyright (C) 2018 Birger Johansson
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
//	Created: 2003
//
//	<Additional description of the module>

//#define COMDEBUG

#include "IPCClient.h"

using namespace ikaros;


void
IPCClient::Init()
{
    host = GetValue("host");
    port = GetIntValue("port");
    
    s  = new Socket();
    
	output = GetOutputMatrix("OUTPUT");
	size_x = GetOutputSizeX("OUTPUT");
	size_y = GetOutputSizeY("OUTPUT");

    timeOut = GetOutputArray("TIME_OUT");

    //Notify(msg_print, "creating buffer, size %i\n", (2+size_x*size_y*sizeof(float)));
	buffer = new char[(2+size_x*size_y)*sizeof(float)];   // 16 bytes per value just in case sizeof(float)!
    timedOut = false;

    timer = new Timer();
    
    Bind(timeOutms, "time_out_in_ms");

    
}


IPCClient::~IPCClient()
{
	delete s;
}

void
IPCClient::Tick()
{

#ifdef COMDEBUG
	printf("IPCClient::Tick()\n");
#endif
    
    fflush(stdout);
	int r = 0;

    // Start a timer
    timer->Restart();
    timedOut = false;
    
	while(r == 0 && !timedOut) // block until connection is made and data is received
	{
	    Timer::Sleep(1);
        #ifdef COMDEBUG
            printf("IPCClient waiting %i %f %i\n", port, timer->GetTime(), time_out_in_ms);
        #endif
	    
        fflush(stdout);
		r = s->Get(host, port, "GET /test/ HTTP/1.1 \r\n\r\n", buffer, (2+size_x*size_y)*sizeof(float));

        // Timeout?
        if (timer->GetTime() > timeOutms && timeOutms > 0)
        {
            #ifdef COMDEBUG
                printf("We have a timeout\n");
            #endif
            timedOut = true;
            break;
        }
	}

	float * p = (float *)buffer;
	p++; p++; // skip header
	
    for(int i=0; i<size_x*size_y; i++)
		output[0][i] = *p++;
    
    if (timedOut)
        *timeOut = 1;
    else
        *timeOut = 0;

    fflush(stdout);
}

static InitClass init("IPCClient", &IPCClient::Create, "Source/Modules/IOModules/Network/IPCClient/");

