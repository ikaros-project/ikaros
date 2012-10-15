//
//	InputVideoFile.h		This file is a part of the IKAROS project
// 						
//    Copyright (C) 2012 <Author Name>
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


#ifndef InputVideoFile_
#define InputVideoFile_

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <stdio.h>

#include "IKAROS.h"

class InputVideoFile: public Module
{
public:
    static Module * Create(Parameter * p) { return new InputVideoFile(p); }

    InputVideoFile(Parameter * p);
    virtual ~InputVideoFile();

    void 		Init();
    void 		Tick();
    
    const char *	filename;
    
    int				size_x;
    int				size_y;
    
    int				native_size_x;
    int				native_size_y;
    
    float *			intensity;
    float *			red;
    float *			green;
    float *			blue;
    float *         restart;
    bool            loop;
    
    AVFormatContext *pFormatCtx;
    int             i, videoStreamIdx;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;
    AVFrame         *pFrame;
    AVFrame         *pFrameRGB;
    AVPacket        packet;
    int             frameFinished;
    int             numBytes;
    uint8_t         *buffer;
    
};

#endif

