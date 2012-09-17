//
//	InputQTMovie.M		This file is a part of the IKAROS project
//						A module for reading from a QuickTime movie
//
//    Copyright (C) 2001-2011  Christian Balkenius
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
//    This module is written in Objective-C++ to allow easy interface with QTKit
//

#include "InputQTMovie.h"

#ifdef USE_QUICKTIME


#import <QTKit/QTKit.h>
#import <Cocoa/Cocoa.h>
#import <QuickTime/QuickTime.h>



class InputQTMovieData
{
public:
    NSAutoreleasePool * pool;
    QTMovie *   movie;
};



InputQTMovie::InputQTMovie(Parameter * p):
Module(p)
{
    
    data = new InputQTMovieData();
    
    data->pool = [[NSAutoreleasePool alloc] init];
    [QTMovie enterQTKitOnThread];

    filename = GetValue("filename");
    loop = GetBoolValue("loop", false);
    
    size_x = GetIntValue("size_x");
    size_y = GetIntValue("size_y");

    if (!filename)
    {
        Notify(msg_fatal_error, "No filename for InputQTMovie.\n");
        return;
    }

    data->movie = nil;

    NSString * fileName = [NSString stringWithCString:filename encoding: NSUTF8StringEncoding];
    NSURL * url = [NSURL fileURLWithPath: fileName];
    if([QTMovie canInitWithURL: url])
    {
        data->movie = [QTMovie movieWithURL:url error:nil];

        NSSize size = [[data->movie attributeForKey:QTMovieNaturalSizeAttribute] sizeValue];        
        native_size_x = int(size.width);
        native_size_y = int(size.height);

        if(size_x == 0 || size_y == 0)
        {
            size_x = native_size_x;
            size_y = native_size_y;
        }
        
        AddOutput("INTENSITY", size_x, size_y);
        AddOutput("RED", size_x, size_y);
        AddOutput("GREEN", size_x, size_y);
        AddOutput("BLUE", size_x, size_y);
        
        AddOutput("RESTART", 1);

        [data->movie gotoBeginning];
    }
    
    else
    {
        Notify(msg_fatal_error, "Could not find movie file \"%s\".\n", filename);
    }
}



InputQTMovie::~InputQTMovie()
{
    [data->movie release];
//    [data->pool drain];
    
    delete data;
    [QTMovie exitQTKitOnThread];
}



void
InputQTMovie::Init()
{
    intensity	= GetOutputArray("INTENSITY");
    red			= GetOutputArray("RED");
    green		= GetOutputArray("GREEN");
    blue		= GetOutputArray("BLUE");
    
    restart     = GetOutputArray("RESTART");
    restart[0]  = -1; // indicates first tick
}



void
InputQTMovie::Tick()
{
    const float c13 = 1.0/3.0;
    const float c1255 = 1.0/255.0;
    
    restart[0] = (restart[0] == -1 ? 1 : 0);
  
    NSImage * image = [data->movie currentFrameImage];
    
    if(image == nil || [data->movie duration].timeValue == [data->movie currentTime].timeValue)
    {
        if(loop)
        {
            [data->movie gotoBeginning];
            image = [data->movie currentFrameImage];
            restart[0] = 1;
        }
        else
        {
            Notify(msg_end_of_file, "End of movie");  
            return;
        }
    }
    
    if(size_x != native_size_x || size_y != native_size_y)
    {
        NSImage * scaledImage = [[NSImage alloc] initWithSize:NSMakeSize(float(size_x), float(size_y))];
        
        [scaledImage lockFocus];
        
        [image drawInRect: NSMakeRect(0, 0, float(size_x), float(size_y))
                fromRect: NSZeroRect
                operation: NSCompositeSourceOver
                fraction: 1.0];
        
        [scaledImage unlockFocus];
        image = scaledImage;
    }

    NSData  * tiffData = [image TIFFRepresentation];
    NSBitmapImageRep * bitmap = [NSBitmapImageRep imageRepWithData:tiffData];
    unsigned char * rawData = [bitmap bitmapData];
/*
    NSLog(@"bytes per row %d", [bitmap bytesPerRow]);
    NSLog(@"bytes per plane %d", [bitmap bytesPerPlane]);
    NSLog(@"samples per pixel %d", [bitmap samplesPerPixel]);
    NSLog(@"number of planes %d", [bitmap numberOfPlanes]);
*/
    // Copy to IKAROS output arrays
    
    int		w = size_x;
    int		h = size_y;
    long    rb = [bitmap bytesPerRow];
    long    ix = 0;

    for (int y=0; y<h; y++)
    {
        for (int x=0; x<w; x++)
        {
            intensity[ix] 	=   red[ix]		= c1255*rawData[y*rb+4*x];
            intensity[ix] 	+=  green[ix]	= c1255*rawData[y*rb+4*x+1];
            intensity[ix] 	+=  blue[ix]	= c1255*rawData[y*rb+4*x+2];
            intensity[ix]*=c13;
            ix++;
        }
    }

    [data->movie stepForward];
}

#endif