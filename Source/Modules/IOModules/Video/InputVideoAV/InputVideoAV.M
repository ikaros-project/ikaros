//
//	InputVideoAV.cc		This file is a part of the IKAROS project
//                      A module to grab image data from a AVKit-compatible camera
//
//    Copyright (C) 2002-2015  Christian Balkenius
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
//
// TODO:
//
//  scale image from camera
//  allow camera to be unconnected and reconnected during operation
//
//  detect camera size automatically; now hardcoded in AVGrab.m


#include "InputVideoAV.h"

#ifdef USE_QUICKTIME

#import <AVKit/AVKit.h>
#import <AVFoundation/AVFoundation.h>

#import <Cocoa/Cocoa.h>
//#import <QuickTime/QuickTime.h>

#import "AVGrab.h"



InputVideoAV::InputVideoAV(Parameter * p):
    Module(p)
{
    printf("--- InputVideoAV\n");

    size_x  = GetIntValue("size_x");
    size_y  = GetIntValue("size_y");
    flip    = GetBoolValue("flip");
    mode    = GetIntValueFromList("mode");
    device_id = create_string(GetValue("device_id"));
    list_devices = GetBoolValue("list_devices");

    if(list_devices)
    {
        NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
        NSArray * devices = [AVGrab videoDevices];
        Notify(msg_print, "\n");
        Notify(msg_print, "Video devices (name, model, device_id):\n");
        Notify(msg_print, "\n");
        for(AVCaptureDevice * d in devices)
        {
            Notify(msg_print, "\"%s\", \"%s\", \"%s\"\n", [[d localizedName] UTF8String], [[d uniqueID] UTF8String], [[d uniqueID] UTF8String]);
        }
        Notify(msg_print, "\n");
        [pool release];
    }

    grabber = [[AVGrab alloc] init];    // Instance of this AVGrab class

    ((AVGrab*)(grabber))->module_name = this->GetName();

    printf("--- grabber: %lx\n", (long)grabber);

    AVCaptureDevice * device = nil;
    if(!equal_strings(device_id, ""))
    {
        device = [AVCaptureDevice deviceWithUniqueID: [NSString stringWithFormat:@"%s", device_id]];
        if(!device)
            device = [AVCaptureDevice deviceWithUniqueID: [NSString stringWithFormat:@" %s", device_id]];    // Work around possible bug where device names starts with a space
    }
    else
    {
        device = [AVGrab defaultVideoDevice];
        device_id = create_string([[device uniqueID] cStringUsingEncoding: NSUTF8StringEncoding]);
        Notify(msg_print, "InputVideoAV.device_id = \"%s\"\n", device_id);
    }
    
    if(!device)
    {
        Notify(msg_warning, "InputVideoAV: video device not found\n");
        grabber = nil;
    }

    else if(device && ![(AVGrab *)grabber startSession: device withMode: mode andRequestedWidth:size_x andHeight:size_y])
    {
        [(AVGrab *)grabber release];
        grabber = nil;
        Notify(msg_warning, "InputVideoAV: video grabber could not be created\n");
    }

    printf("--- device: %lx\n", (long)device);

    if(grabber)
    {
        size_x = [(AVGrab *)grabber width];
        size_y = [(AVGrab *)grabber height];
    }
}



InputVideoAV::~InputVideoAV()
{
    Notify(msg_verbose, "Closing Grabber\n");
    
    [(AVGrab *)grabber stopSession];  
    [(AVGrab *)grabber release];
}



void
InputVideoAV::Init()
{
    intensity   = GetOutputArray("INTENSITY");
    red         = GetOutputArray("RED");
    green       = GetOutputArray("GREEN");
    blue		= GetOutputArray("BLUE");
    
}



void
InputVideoAV::Tick()
{
    printf("--- InputVideoAV::Tick()\n");

    // We need to create an autorelease pool every tick since each call may be run in a new thread

    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

    const float c13 = 1.0/3.0;
    const float c1255 = 1.0/255.0;

    if(!grabber)
    {
        [pool release];
        printf("--- no grabber\n");
        return;
    }

    CVImageBufferRef buffer = [(AVGrab *)grabber grabCVBuffer];
    if(!buffer)
    {
        printf("--- no buffer\n");
        [pool release];
        return;
    }

    size_t	w = CVPixelBufferGetWidth(buffer);
    size_t	h = CVPixelBufferGetHeight(buffer);

    printf("--- size %ld %ld\n", w, h);


    CFTypeID bufferType = CFGetTypeID(buffer);
    if (bufferType != CVPixelBufferGetTypeID())
    {
        [pool release];
        printf("ERROR WRONG VIDEO FORMAT\n");
        return;
    }

// TODO: Should check color space here
//    CGColorSpaceRef colorSpace = CVImageBufferGetColorSpace(buffer);
//    NSLog(@"%@", colorSpace);
    
 //   size_t	w = CVPixelBufferGetWidth(buffer);
//    size_t	h = CVPixelBufferGetHeight(buffer);
    size_t  rb = CVPixelBufferGetBytesPerRow(buffer);
    size_t  ix = 0;
    
    CVPixelBufferLockBaseAddress(buffer, 0);
    unsigned char * rawData = (unsigned char *)CVPixelBufferGetBaseAddress(buffer);
    
    // Correct conversion to grayscale
    // 0.2126 R + 0.7152 G + 0.0722 B
    // y = (r*77)+(g*151)+(b*28);

    if(!flip)
    {
        for (long y=0; y<h; y++)
        {
            for (int x=0; x<w; x++)
            {
                intensity[ix] 	=   red[ix]		= c1255*rawData[y*rb+4*x+1];
                intensity[ix] 	+=  green[ix]	= c1255*rawData[y*rb+4*x+2];
                intensity[ix] 	+=  blue[ix]	= c1255*rawData[y*rb+4*x+3];
                intensity[ix]*=c13;
                ix++;
            }
        }
    }
    
    else
    {
        for (long y=h-1; 0<=y; y--)
        {
            for (int x=0; x<w; x++)
            {
                intensity[ix] 	=   red[ix]		= c1255*rawData[y*rb+4*x+1];
                intensity[ix] 	+=  green[ix]	= c1255*rawData[y*rb+4*x+2];
                intensity[ix] 	+=  blue[ix]	= c1255*rawData[y*rb+4*x+3];
                intensity[ix]*=c13;
                ix++;
            }
        }
    }

    printf("--- image transferred\n");

    CVPixelBufferUnlockBaseAddress(buffer, 0);
    CVBufferRelease(buffer);
    [pool release];
}



static InitClass init("InputVideoAV", &InputVideoAV::Create, "Source/Modules/IOModules/Video/InputVideoAV/");

#endif

