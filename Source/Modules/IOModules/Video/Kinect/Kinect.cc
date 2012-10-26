//
//	  Kinect.cc     This file is a part of the IKAROS project
//                  A module to grab images from a Kinect
//
//    Copyright (C) 2012  Christian Balkenius, Birger Johansson
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

#include "Kinect.h"

extern "C"
{
#include "libfreenect.h"
#include "libfreenect_sync.h"
}

using namespace ikaros;




Kinect::~Kinect()
{
    freenect_sync_stop();
}



void
Kinect::Init()
{
    tilt        = GetInputArray("TILT", false);
    led         = GetInputArray("LED", false);

    intensity   = GetOutputMatrix("INTENSITY");
    depth       = GetOutputMatrix("DEPTH");
    red			= GetOutputMatrix("RED");
    green		= GetOutputMatrix("GREEN");
    blue		= GetOutputMatrix("BLUE");
    
    mode        = GetIntValueFromList("mode");
    
    freenect_sync_set_led(LED_OFF, 0);
}



void
Kinect::Tick()
{
	short *depth_buf = 0;
    unsigned char *rgb_data;
    uint32_t timestamp;

    int ret = freenect_sync_get_depth((void**)(&depth_buf), &timestamp, 0, (mode== 0 ? FREENECT_DEPTH_11BIT : FREENECT_DEPTH_REGISTERED));

    if(ret < 0)
    {
        Notify(msg_warning, "Kinect device not found.");
        return;
    }
    
    ret = freenect_sync_get_video((void**)(&rgb_data), &timestamp, 0, FREENECT_VIDEO_RGB);
    
    if(ret < 0)
    {
        Notify(msg_warning, "Kinect device not found.");
        return;
    }

    const float c13 = 1.0/3.0;
    const float c1255 = 1.0/255.0;

    size_t  rb = 640;
    for (long y=0; y<480; y++)
    {
        for (int x=0; x<640; x++)
        {
            intensity[y][x] 	=   red[y][x]   = c1255*rgb_data[y*3*rb+3*x];
            intensity[y][x] 	+=  green[y][x]	= c1255*rgb_data[y*3*rb+3*x+1];
            intensity[y][x] 	+=  blue[y][x]	= c1255*rgb_data[y*3*rb+3*x+2];
            intensity[y][x]*=c13;
        }
    }

    if(mode == 0) // raw
    {
        int s = 0;
        for(int j=0; j<480; j++)
            for(int i=0; i<640; i++)
                depth[j][i] = float(depth_buf[s++])/2047.0;
    }

    else // mm, registered
    {
        int s = 0;
        for(int j=0; j<480; j++)
            for(int i=0; i<640; i++)
                depth[j][i] = float(depth_buf[s++]);
    }

    // Set LED color

    if(led)
    {
        if(*led > 0.75)
            freenect_sync_set_led(LED_GREEN, 0);
        else if(*led > 0.5)
            freenect_sync_set_led(LED_YELLOW, 0);
        else if(*led > 0.25)
            freenect_sync_set_led(LED_RED, 0);
        else
            freenect_sync_set_led(LED_OFF, 0);
    }
    
    // Set tilt

    if(tilt)
        freenect_sync_set_tilt_degs(int(30.0*(*tilt)-15.0), 0);
}


static InitClass init("Kinect", &Kinect::Create, "Source/Modules/IOModules/Video/Kinect/");

