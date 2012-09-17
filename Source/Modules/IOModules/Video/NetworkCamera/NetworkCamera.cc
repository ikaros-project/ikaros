//
//	NetworkCamera.cc    This file is a part of the IKAROS project
//						A module to grab images from a network camera
//
//    Copyright (C) 2002-2010  Christian Balkenius, Birger Johansson
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



#include "NetworkCamera.h"

#ifdef USE_LIBJPEG
#ifdef USE_SOCKET

#define MAX_STR_LEN 512



extern "C" {
#include "jpeglib.h"
}

#include <setjmp.h>

using namespace ikaros;

void
NetworkCamera::SkipBoundary(char * bound)
{
    int p = 0;
    //int rc;
    char c;

    while (bound[p] != 0)
    {
        //rc = 
        socket->ReadData(&c, 1);
        if (boundary[p] == c)
            p++;
    }

    socket->ReadData(&c, 1); // Skip line break
    socket->ReadData(&c, 1); // Skip line break
}



void
NetworkCamera::ReadLine(char * line)
{
    int p = 0;
    int rc;
    char c;
    do
    {
        rc = socket->ReadData(&c, 1);
        if (rc == 1 && c != '\n' && c!= '\r')
            line[p++] = c;
        else
            line[p] = 0;
    }
    while (c != 13);
    socket->ReadData(&c, 1);  // Skip line break
}



bool
NetworkCamera::ReadBlock()
{
    // Check for new image data on the socket

    if (!socket->Poll())
        return false;

    // Read Header

    char line[256];
    char attribute[256];
    int length = 0;
    do
    {
        ReadLine(line);
        int cnt = sscanf(line, "%[^:]", attribute);

        if (cnt == 1 && !strcmp(attribute, "Content-Length"))
        {
            sscanf(line, "%[^:]:%d", attribute, &length);
        }

        else if (cnt == 1 && !strcmp(attribute, "Delta-time"))
        {
            int delta_time;
            cnt = sscanf(line, "%[^:]:%d", attribute, &delta_time);
            if (cnt > 0)
                timestamp += delta_time;
        }
    }
    while (strlen(line) > 0);

    // Read Image Data (fill buffer)

    if(length > buffer_size)
    {
        buffer_size = length;
        buffer = (char *)realloc(buffer, buffer_size*sizeof(char));
        if(!buffer)
            Notify(msg_fatal_error, "NetworkCamera: memory allocation failed\n");
    }
    
    socket->ReadData(buffer, length, true);
    SkipBoundary(boundary);

    return true;
}



void
NetworkCamera::ReadHeader() // This part is specific for the AXIS camera and should be more general
{
    char header[1024];

    int rc = socket->ReadData(header, 100); // we just happen to know that the header size == 100
    header[100] = 0;

    // Read boundary marker

    int b = 0;
    char c;
    do
    {
        rc = socket->ReadData(&c, 1);
        if (rc == 0)
        {
            Notify(msg_warning, "NetworkCamera::ReadHeader: Error reading boundary marker\n");
            return;
        }
        if (c != '\r' && c != '\n')
            boundary[b++] = c;
    } while (c != '\n');
    boundary[b] = 0;

    // The boundary marker in the package from the camera should probably not contain
    // the leading "--" but since it does we just keep them in the string
    // This may have to be changed for other cameras

    SkipBoundary(boundary);
}



NetworkCamera::NetworkCamera(Parameter * p):
    Module(p)
{
    timestamp = 0;
    
    buffer_size =   INITIAL_BUFFER_SIZE;
    buffer      =   (char *)malloc(INITIAL_BUFFER_SIZE*sizeof(char));

    size_x  = GetIntValue("size_x", 352);	// 704x480, 176x144, 352x240
    size_y  = GetIntValue("size_y", 240);

    if (size_x*size_y < 1)
    {
        Notify(msg_fatal_error, "Image size not supplied.\n");
        return;
    }

    host_ip			=	GetValue("host_ip");
    host_port		=	GetIntValue("host_port", 80);
    image_request	=	GetValue("image_request");		// Not used
    fps				=	GetIntValue("fps", 10);
    compression		=	GetIntValue("compression", 50);

    AddOutput("INTENSITY", size_x, size_y);
    AddOutput("RED", size_x, size_y);
    AddOutput("GREEN", size_x, size_y);
    AddOutput("BLUE", size_x, size_y);

    // Create socket

    socket = new Socket();

    // Set up connection and tell camera to stream data

    char msg[1024];
    
    sprintf(msg, "GET /axis-cgi/mjpg/video.cgi?deltatime=1&showlength=1&resolution=%dx%d&req_fps=%d&compression=%d HTTP/1.1\r\n\r\n", size_x, size_y, fps, compression);
    
    if (!socket->SendRequest(host_ip, host_port, msg))
    {
        Notify(msg_fatal_error, "Could not connect to network camera at %s:%d.\n", host_ip, host_port);
        return;
    }

    ReadHeader();
}



void
NetworkCamera::Init()
{
    intensity	= GetOutputMatrix("INTENSITY");
    red			= GetOutputMatrix("RED");
    green		= GetOutputMatrix("GREEN");
    blue		= GetOutputMatrix("BLUE");
}



void
NetworkCamera::Tick()
{
    while (ReadBlock()) ;	// Read all available blocks
    jpeg_decode(red, green, blue, intensity, size_x, size_y, buffer, buffer_size);
}

#endif
#endif

