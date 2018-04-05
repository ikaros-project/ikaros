//
//	IPCServer.cc		This file is a part of the IKAROS project
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


#include "IPCServer.h"
//#define COMDEBUG
using namespace ikaros;

void IPCServer::Init()
{
    port = GetIntValue("port");
    s = new ServerSocket(port);
	input = GetInputMatrix("INPUT");
	size_x = GetInputSizeX("INPUT");
	size_y = GetInputSizeY("INPUT");
}

IPCServer::~IPCServer()
{
	delete s;
}


void IPCServer::Tick()
{
    // Send data
#ifdef COMDEBUG
	printf("IPCServer::Tick()\n");
#endif
   bool r = false;


   while(!r)
    {
        Timer::Sleep(10);
#ifdef COMDEBUG
        printf("IPCServer waiting %i\n",port);
#endif

        fflush(stdout);
        r = s->GetRequest();
#ifdef COMDEBUG
        printf("Get Request = %i\n",r);
#endif
        //r = s->TestArgument(0, "GET\r\n\r\n");
        //printf("Test Argument = %i\n",r);

        if(r)
        {
            s->Send(input[0], size_x, size_y);
            s->Close();
        }
    }
#ifdef COMDEBUG
    printf("IPCServer::Tick() END\n");
#endif
}


static InitClass init("IPCServer", &IPCServer::Create, "Source/Modules/IOModules/Network/IPCServer/");
