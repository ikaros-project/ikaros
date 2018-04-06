//
//	FadeCandy.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2016 Christian Balkenius
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

#include "FadeCandy.h"
#include <unistd.h>
#include <signal.h>


using namespace ikaros;



FadeCandy::FadeCandy(Parameter * p):
    Module(p)
{
    // Starting fade candy server
	startServer = false;
    fcserver_pid = -1;
	const char * c = GetValue("command");
	startServer = GetBoolValue("start_server");
    if(strlen(c) != 0 and startServer)
    {
        char * cmd = create_formatted_string("%s%s", GetClassPath(), GetValue("command"));

        printf("Starting: %s\n", cmd);

        char * argv[3] = { cmd, NULL, NULL };

        if((fcserver_pid = fork()) != 0)
            printf("PID: %d\n", fcserver_pid);
        else
        {
            execvp(cmd, argv);
            _exit (0);
        }
    }

    no_of_channels = 0;

    // Count channels *** parnent_group->appended_ekements

    XMLNode * par = xml->parent;
    for (XMLElement * c = ((XMLElement *)(par))->GetContentElement("channel"); c != NULL; c = c->GetNextElement("channel"))
        no_of_channels++;

    // Allocate memory

    channel_name_red = new char * [no_of_channels];
    channel_name_green = new char * [no_of_channels];
    channel_name_blue = new char * [no_of_channels];

    channel_size = new int [no_of_channels];
    channel_LED_size = new int [no_of_channels];
    channel_index = new int [no_of_channels];

    for (int i=0; i<no_of_channels; i++)
    {
        channel_name_red[i] = NULL;
        channel_name_green[i] = NULL;
        channel_name_blue[i] = NULL;
        channel_size[i] = 0;
        channel_index[i] = i;
    }

    // Add input for each column in the parameter list

    int col = 0;
    for (XMLElement * c = ((XMLElement *)(par))->GetContentElement("channel"); c != NULL; c = c->GetNextElement("channel"))
    {
        const char * name = c->GetAttribute("name");
        if (name == NULL)
        {
            Notify(msg_warning, "Column name missing in module \"%s\". Ignored.\n", GetName());
            break;
        }

        channel_name_red[col] = create_formatted_string("%s_RED", name);
        channel_name_green[col] = create_formatted_string("%s_GREEN", name);
        channel_name_blue[col] = create_formatted_string("%s_BLUE", name);

        const char * s = c->GetAttribute("size");
        if(s) channel_LED_size[col] = string_to_int(s);

        const char * ix = c->GetAttribute("index");
        if(ix) channel_index[col] = string_to_int(ix);

        AddInput(channel_name_red[col]);
        AddInput(channel_name_green[col]);
        AddInput(channel_name_blue[col]);

        col++;
    }
}



void
FadeCandy::Init()
{
    socket = new Socket();

    // Check sizes of connected inputs here

    // Get inputs

    channel_red = new float * [no_of_channels];
    channel_green = new float * [no_of_channels];
    channel_blue = new float * [no_of_channels];

    for (int i=0; i<no_of_channels; i++)
    {
        channel_red[i] = GetInputArray(channel_name_red[i]);
        channel_green[i] = GetInputArray(channel_name_green[i]);
        channel_blue[i] = GetInputArray(channel_name_blue[i]);

        channel_size[i] = GetInputSize(channel_name_red[i]); // Should check all have the same size
    }
}



FadeCandy::~FadeCandy()
{
	if (startServer)
	{
		sleep(1); // Added a sleep here to make sure the fcserver started and its ready to be killed.
		if((kill(fcserver_pid,SIGKILL)) == 0)
		{
			//	printf("Killed fcserver (PID %i)\n",fcserver_pid);
		}
		else
			printf("Could not kill fcserver (PID %i).\n",fcserver_pid);
		}
    // delete ***
}

void
FadeCandy::Tick()
{
    int len = 4+no_of_channels*64*3;
    unsigned char request[len];
    request[0] = 0;
    request[1] = 0;
    request[2] = (no_of_channels*64*3) / 256;
    request[3] = (no_of_channels*64*3) % 256;

    for (int i=0; i<no_of_channels; i++)
    {
        int k = 4 + i*64*3;
        for(int j=0; j<channel_size[i]; j++)
        {
            request[k++] = int(255*channel_red[i][j]);
            request[k++] = int(255*channel_green[i][j]);
            request[k++] = int(255*channel_blue[i][j]);
        }
    }

    socket->SendRequest("127.0.0.1", 7890, (char *)request, len);
    socket->Close();
}

// Install the module. This code is executed during start-up.

static InitClass init("FadeCandy", &FadeCandy::Create, "Source/Modules/RobotModules/FadeCandy/");


