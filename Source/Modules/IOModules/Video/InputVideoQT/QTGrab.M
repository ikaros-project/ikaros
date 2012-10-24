//
//    QTGrab.m		This file is a part of the IKAROS project
//					Frame grabber using QTKit
//
//    Copyright (C) 2011  Christian Balkenius
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
//    Based on code by Robert Harder on 9/10/09.
//    http://iharder.sourceforge.net/current/macosx/imagesnap/


#import "QTGrab.h"
#include <unistd.h>

@interface QTGrab()


- (void)captureOutput:(QTCaptureOutput *)captureOutput 
  didOutputVideoFrame:(CVImageBufferRef)videoFrame 
     withSampleBuffer:(QTSampleBuffer *)sampleBuffer 
       fromConnection:(QTCaptureConnection *)connection;

@end


@implementation QTGrab

- (id)init
{
	self = [super init];
    session = nil;
    deviceInput = nil;
    decompressedVideoOutput = nil;
	imageBuffer = nil;
    requestedMode = 0;
    requestedDevice = nil;
    requestedSize = NSMakeSize(0,0);
    status = 0;
	return self;
}



- (void)dealloc
{	
    [session release];
    [deviceInput release];
    [decompressedVideoOutput release];
    CVBufferRelease(imageBuffer);
    [super dealloc];
}



// Returns an array of video devices attached to this computer.
+ (NSArray *)videoDevices
{
    NSMutableArray *results = [NSMutableArray arrayWithCapacity:3];
    [results addObjectsFromArray:[QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeVideo]];
    [results addObjectsFromArray:[QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeMuxed]];
    return results;
}



// Returns the default video device or nil if none found.
+ (QTCaptureDevice *)defaultVideoDevice
{
	QTCaptureDevice *device = nil;
    
	device = [QTCaptureDevice defaultInputDeviceWithMediaType:QTMediaTypeVideo];
	if( device == nil ){
        device = [QTCaptureDevice defaultInputDeviceWithMediaType:QTMediaTypeMuxed];
	}
    return device;
}



// Returns the named capture device or nil if not found.
+(QTCaptureDevice *)deviceNamed:(NSString *)name
{
    QTCaptureDevice *result = nil;
    
    NSArray *devices = [QTGrab videoDevices];
	for(QTCaptureDevice *device in devices)
    {
        if ([name isEqualToString:[device description]])
        {
            result = device;
        }
    }
    
    return result;
}



+(NSImage *)singleSnapshotFrom:(QTCaptureDevice *)device
{
    NSImage *image = nil;    
    QTGrab *snap = [[QTGrab alloc] init];
    if([snap startSession:device withMode:0 andRequestedWidth:0 andHeight:0])
    {
        image = [snap grab];
        [snap stopSession];
     }
    [snap release];
    
    return image;
}



-(NSImage *)grab
{
    NSDate * startGrabTime = [NSDate date];
    CVImageBufferRef frame = nil;
    while(frame == nil && [startGrabTime timeIntervalSinceNow] > -1.0) // Time out after 1 second
    { 
        @synchronized(self)
        {
            frame = imageBuffer; 
            CVBufferRetain(frame);
        }
		
        if(frame == nil)
        {   
            [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow: 0.1]]; 
        }
    }
    
    NSCIImageRep *imageRep = [NSCIImageRep imageRepWithCIImage:[CIImage imageWithCVImageBuffer:frame]];
    NSImage *image = [[[NSImage alloc] initWithSize:[imageRep size]] autorelease];
    [image addRepresentation:imageRep];
    CVBufferRelease(frame);
    
    return image;
}



-(CVImageBufferRef)grabCVBuffer
{
    NSDate * startGrabTime = [NSDate date];
    CVImageBufferRef frame = nil;
    while(frame == nil && [startGrabTime timeIntervalSinceNow] > -1.0) // Time out after 1 second
    { 
        @synchronized(self)
        {
            frame = imageBuffer; 
            CVBufferRetain(frame);
        }
		
        if(frame == nil)
        {   
            [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow: 0.1]]; 
        }
    }
    
    return frame;
}




-(void)stopSession
{
    while(session != nil)
    {
        [session stopRunning];
        if( [session isRunning] ){
            [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow: 0.1]];
        }
        else
        {
            [session release];
            [deviceInput release];
            [decompressedVideoOutput release];
            
            session = nil;
            deviceInput = nil;
            decompressedVideoOutput = nil;
        }
    }
}



-(BOOL)startSession:(QTCaptureDevice *)device withMode:(int)mode andRequestedWidth:(int)w andHeight:(int)h
{
    if(device == nil)
		return NO;
    
    requestedMode = mode;
    requestedDevice = device;
    requestedSize =  NSMakeSize((float)(w), (float)(h));
    
    [self start];  // Start independent capture thread

    for(int i=0; i<500; i++) // Give the start up max 5 second
    {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, YES);
        if(status == 1)
            break;
        usleep((useconds_t)(1000*10));
    }
    
    return (status == 1 ? YES : NO);
}



-(int)width
{
    return [[[decompressedVideoOutput pixelBufferAttributes] objectForKey:(id)kCVPixelBufferWidthKey] intValue];
}



-(int)height
{
    return [[[decompressedVideoOutput pixelBufferAttributes] objectForKey:(id)kCVPixelBufferHeightKey] intValue];
}



- (void)captureOutput:(QTCaptureOutput *)captureOutput 
  didOutputVideoFrame:(CVImageBufferRef)videoFrame 
     withSampleBuffer:(QTSampleBuffer *)sampleBuffer 
       fromConnection:(QTCaptureConnection *)connection
{
    if (videoFrame == nil)
        return;
    
    CVImageBufferRef imageBufferToRelease;
    CVBufferRetain(videoFrame);
	
    @synchronized(self)
    {
        imageBufferToRelease = imageBuffer;
        imageBuffer = videoFrame;
    }
    
    CVBufferRelease(imageBufferToRelease);
}



-(void)main
{
    pool  = [[NSAutoreleasePool alloc] init];
 
    NSError *error = nil;

    session = [[QTCaptureSession alloc] init];
	if( ![requestedDevice open:&error] )
    {
        [session release];
        session = nil;
		return; //fail
	}
    
	deviceInput = [[QTCaptureDeviceInput alloc] initWithDevice:requestedDevice];
	if (![session addInput:deviceInput error:&error])
    {
        [session release];
        [deviceInput release];
        session = nil;
        deviceInput = nil;
		return; //fail
	}
    
    if(requestedMode == 0)
        decompressedVideoOutput = [[QTCaptureDecompressedVideoOutput alloc] init];
    else
        decompressedVideoOutput = (QTCaptureDecompressedVideoOutput *)[[QTCaptureVideoPreviewOutput alloc] init];
	[decompressedVideoOutput setDelegate:self];
    
    // Request image parameters
    [decompressedVideoOutput setPixelBufferAttributes:
       [NSDictionary dictionaryWithObjectsAndKeys:
         [NSNumber numberWithDouble: requestedSize.width], (id)kCVPixelBufferWidthKey,
         [NSNumber numberWithDouble: requestedSize.height], (id)kCVPixelBufferHeightKey,
         [NSNumber numberWithUnsignedInt:kCVPixelFormatType_32ARGB], (id)kCVPixelBufferPixelFormatTypeKey,
      nil]];
    
	if(![session addOutput:decompressedVideoOutput error:&error])
    {
        [session release];
        [deviceInput release];
        [decompressedVideoOutput release];
        session = nil;
        deviceInput = nil;
        decompressedVideoOutput = nil;
		return; // fail
	}
    
    @synchronized(self)
    {
        if(imageBuffer != nil)
        {
            CVBufferRelease(imageBuffer);
            imageBuffer = nil;
        } 
    }
	
    [session startRunning];

    status = 1;
}



@end


