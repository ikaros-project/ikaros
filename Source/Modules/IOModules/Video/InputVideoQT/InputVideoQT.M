//
//	InputVideoQT.cc		This file is a part of the IKAROS project
//                      A module to grab image data from a QuickTime compatible camera
//
//    Copyright (C) 2002-2012  Christian Balkenius
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
//  remove dependence on TIFF represenattion to get rid of the stupid mutex-solution
//  allow camera to be unconnected and reconnected during operation
//


#include "InputVideoQT.h"

#ifdef USE_QUICKTIME


#import <QTKit/QTKit.h>
#import <Cocoa/Cocoa.h>
#import <QuickTime/QuickTime.h>

#import "QTGrab.h"

// Since TIFFRepresentation is not thread-safe
// we use the following object as a lock
// This work as long as no other modules use the
// same strategy.

static NSObject * lock = [NSObject new];



InputVideoQT::InputVideoQT(Parameter * p):
    Module(p)
{
    size_x  = GetIntValue("size_x");
    size_y  = GetIntValue("size_y");
    flip    = GetBoolValue("flip");
    mode    = GetIntValueFromList("mode");
    device_id = create_string(GetValue("device_id"));
    list_devices = GetBoolValue("list_devices");

    if(list_devices)
    {
        NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
        NSArray * devices = [QTGrab videoDevices];
        Notify(msg_print, "\n");
        Notify(msg_print, "Video devices (name, model, device_id):\n");
        Notify(msg_print, "\n");
        for(QTCaptureDevice * d in devices)
        {
            Notify(msg_print, "\"%s\", \"%s\", \"%s\"\n", [[d localizedDisplayName] UTF8String], [[d modelUniqueID] UTF8String], [[d uniqueID] UTF8String]);
        }
        Notify(msg_print, "\n");
        [pool release];
    }

    grabber = [[QTGrab alloc] init];    // Instance of this QTGrab class


    QTCaptureDevice * device = nil;
    if(!equal_strings(device_id, ""))
    {
        device = [QTCaptureDevice deviceWithUniqueID: [NSString stringWithFormat:@"%s", device_id]];
        if(!device)
            device = [QTCaptureDevice deviceWithUniqueID: [NSString stringWithFormat:@" %s", device_id]];    // Work around possible bug where device names starts with a space
    }
    else
    {
        device = [QTGrab defaultVideoDevice];
        device_id = create_string([[device uniqueID] cStringUsingEncoding: NSUTF8StringEncoding]);
        Notify(msg_print, "InputVideoQT.device_id = \"%s\"\n", device_id);
    }
    
    if(!device)
    {
        Notify(msg_warning, "InputVideoQT: video device not found\n");
        grabber = nil;
    }

    else if(device && ![(QTGrab *)grabber startSession: device withMode: mode andRequestedWidth:size_x andHeight:size_y])
    {
        [(QTGrab *)grabber release];
        grabber = nil;
        Notify(msg_warning, "InputVideoQT: video grabber could not be created\n");
    }

    if(grabber)
    {
        size_x = [(QTGrab *)grabber width];
        size_y = [(QTGrab *)grabber height];
    }

    AddOutput("INTENSITY", size_x, size_y);
    AddOutput("RED", size_x, size_y);
    AddOutput("GREEN", size_x, size_y);
    AddOutput("BLUE", size_x, size_y);
}



InputVideoQT::~InputVideoQT()
{
    Notify(msg_verbose, "Closing Grabber\n");
    
    [(QTGrab *)grabber stopSession];  
    [(QTGrab *)grabber release];
}



void
InputVideoQT::Init()
{
    intensity   = GetOutputArray("INTENSITY");
    red         = GetOutputArray("RED");
    green       = GetOutputArray("GREEN");
    blue		= GetOutputArray("BLUE");
    
}



void
InputVideoQT::Tick()
{
    // We need to create an autorelease pool every tick since each may be run in a new thread every time

    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

    const float c13 = 1.0/3.0;
    const float c1255 = 1.0/255.0;

    if(!grabber)
    {
        [pool release];
        return;
    }

    CVImageBufferRef buffer = [(QTGrab *)grabber grabCVBuffer];
    if(!buffer)
    {
        [pool release];
        return;
    }

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
    
    size_t	w = CVPixelBufferGetWidth(buffer);
    size_t	h = CVPixelBufferGetHeight(buffer);
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

    CVPixelBufferUnlockBaseAddress(buffer, 0);
    CVBufferRelease(buffer);
    [pool release];
}


#endif
