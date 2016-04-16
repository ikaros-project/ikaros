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

// use the ikaros namespace to access the math library
// this is preferred to using <cmath>

using namespace ikaros;



FadeCandy::FadeCandy(Parameter * p):
    Module(p)
{
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

    for (int i=0; i<no_of_channels; i++)
    {
        channel_name_red[i] = NULL;
        channel_name_green[i] = NULL;
        channel_name_blue[i] = NULL;
        channel_size[i] = 0;
    }

    // Add input for each column in the parameter list

    int col = 0;
    for (XMLElement * c = ((XMLElement *)(par))->GetContentElement("channel"); c != NULL; c = c->GetNextElement("channel"))
    {
//	printf("Column: %s\n", c->GetAttribute("name"));

        const char * name = c->GetAttribute("name");
        if (name == NULL)
        {
            Notify(msg_warning, "Column name missing in module \"%s\". Ignored.\n", GetName());
            break;
        }

        channel_name_red[col] = create_formatted_string("%s_RED", name);
        channel_name_green[col] = create_formatted_string("%s_GREEN", name);
        channel_name_blue[col] = create_formatted_string("%s_BLUE", name);

        AddInput(channel_name_red[col]);
        AddInput(channel_name_green[col]);
        AddInput(channel_name_blue[col]);

// EXTRA DATA; E.G: CHANNEL SIZE?; column_decimals[col] = string_to_int(c->GetAttribute("decimals"), no_of_decimals);

        col++;
    }
}



void
FadeCandy::Init()
{
    socket = new Socket();
}



FadeCandy::~FadeCandy()
{
}



void
FadeCandy::Tick()
{
    unsigned char request[1024] = { 0, 0, 0, 3, 255, 128, 0};
    
    socket-> SendRequest("127.0.0.1", 7890, (char *)request, 7);
}



// Install the module. This code is executed during start-up.

static InitClass init("FadeCandy", &FadeCandy::Create, "Source/UserModules/FadeCandy/");


