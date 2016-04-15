//
//    AVGrab.h		This file is a part of the IKAROS project
//					Frame grabber using AVKit
//
//    Copyright (C) 2011-2014  Christian Balkenius
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


#import <Cocoa/Cocoa.h>
#import <AVKit/AVKit.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>



@interface AVGrab : NSThread <AVCaptureVideoDataOutputSampleBufferDelegate>
{    
@public
    NSAutoreleasePool* pool;

    AVCaptureSession                    *session;
    AVCaptureDeviceInput                *deviceInput;
    AVCaptureVideoDataOutput            *decompressedVideoOutput;
    CVImageBufferRef                    imageBuffer;

    int                                 requestedMode;
    NSSize                              requestedSize;
    AVCaptureDevice *                   requestedDevice;
    
    int                                 status; // 0 = error, 1 = ok

    const char *                        module_name;
}


+(NSArray *)videoDevices;
+(AVCaptureDevice *)defaultVideoDevice;
+(AVCaptureDevice *)deviceNamed:(NSString *)name;

+(NSImage *)singleSnapshotFrom:(AVCaptureDevice *)device;

-(id)init;
-(void)dealloc;

-(BOOL)startSession:(AVCaptureDevice *)device withMode:(int)mode andRequestedWidth:(int)w andHeight:(int)h;
-(void)stopSession;

-(int)width;
-(int)height;

-(NSImage *)grab;
-(CVImageBufferRef)grabCVBuffer;

-(void)main;

@end
