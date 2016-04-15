//
//    AVGrab.m		This file is a part of the IKAROS project
//					Frame grabber using AVKit
//
//    Copyright (C) 2011-2015  Christian Balkenius
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

#import "AVGrab.h"

#import <AVFoundation/AVFoundation.h>

#import <CoreMedia/CoreMedia.h>
#import <CoreMedia/CMBufferQueue.h>
#import <CoreMedia/CMSampleBuffer.h>

#include <unistd.h>

@interface AVGrab()

- (void)captureOutput:(AVCaptureOutput *)captureOutput
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
    fromConnection:(AVCaptureConnection *)connection;
@end



@implementation AVGrab

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
    [results addObjectsFromArray:[AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo]];
    [results addObjectsFromArray:[AVCaptureDevice devicesWithMediaType:AVMediaTypeMuxed]];
    return results;
}



// Returns the default video device or nil if none found.
+ (AVCaptureDevice *)defaultVideoDevice
{
	AVCaptureDevice *device = nil;
    
	device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
	if( device == nil ){
        device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeMuxed];
	}
    return device;
}



// Returns the named capture device or nil if not found.
+(AVCaptureDevice *)deviceNamed:(NSString *)name
{
    AVCaptureDevice *result = nil;
    
    NSArray *devices = [AVGrab videoDevices];
	for(AVCaptureDevice *device in devices)
    {
        if ([name isEqualToString:[device description]])
        {
            result = device;
        }
    }
    
    return result;
}



+(NSImage *)singleSnapshotFrom:(AVCaptureDevice *)device
{
    NSImage *image = nil;    
    AVGrab *snap = [[AVGrab alloc] init];
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
            NSLog(@"%s: Received frame", module_name);
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



-(BOOL)startSession:(AVCaptureDevice *)device withMode:(int)mode andRequestedWidth:(int)w andHeight:(int)h
{
    if(device == nil)
    {
        NSLog(@"ERROR: No device");
		return NO;
    }
    
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

    NSLog(@"Started session (%d x %d): status = %d", w, h, status);

    return (status == 1 ? YES : NO);
}



-(int)width
{
    return 800;
}



-(int)height
{
    return 600;
}



- (void)captureOutput:(AVCaptureOutput *)captureOutput
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
    fromConnection:(AVCaptureConnection *)connection
{
    if(captureOutput == decompressedVideoOutput)
    {
        CVImageBufferRef imageBufferToRelease;
        CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
        OSType format = CVPixelBufferGetPixelFormatType(pixelBuffer);

        if(format != kCVPixelFormatType_32ARGB) // kCMPixelFormat_422YpCbCr8
        {
            printf("AVGrab ERROR: Incorrect image format: %x\n", format);
            return;
        }

/*
        size_t w = CVPixelBufferGetWidth(pixelBuffer);
        size_t h = CVPixelBufferGetHeight(pixelBuffer);
*/

        @synchronized(self)
        {
            imageBufferToRelease = imageBuffer; // old buffer
            imageBuffer = pixelBuffer; // new buffer
            CFRetain(imageBuffer);
        }

        CVBufferRelease(imageBufferToRelease);
    }
}



- (void)captureOutput:(AVCaptureOutput *)captureOutput
  didDropSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection *)connection
{
    // Frame dropped - do nothing
    NSLog(@"FRAME DROPPED\n");
}



-(void)main
{
    pool  = [[NSAutoreleasePool alloc] init];
 
    session = [[AVCaptureSession alloc] init];
    if ([session canSetSessionPreset:AVCaptureSessionPreset640x480])    // AVCaptureSessionPreset1280x720
    {
        session.sessionPreset = AVCaptureSessionPreset640x480;
    }
    else
    {
        printf("Video format not supported - trying anyway.\n");
    }

    deviceInput =[AVCaptureDeviceInput deviceInputWithDevice:requestedDevice error:nil];
    [session addInput:deviceInput];

    
    decompressedVideoOutput = [[AVCaptureVideoDataOutput alloc] init];

    dispatch_queue_t captureQueue = dispatch_queue_create("AVCaptureQueue", DISPATCH_QUEUE_SERIAL);

	[decompressedVideoOutput setSampleBufferDelegate:(id <AVCaptureVideoDataOutputSampleBufferDelegate>)self queue: captureQueue];

    decompressedVideoOutput.alwaysDiscardsLateVideoFrames = YES;
    decompressedVideoOutput.videoSettings =  @{ (NSString *)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32ARGB) };
    [session addOutput:decompressedVideoOutput];

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

