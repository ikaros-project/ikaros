//
//	  InputVideo.h		This file is a part of the IKAROS project
// 						
//    Copyright (C) 2016 Birger Johansson
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


#ifndef InputVideo_
#define InputVideo_


#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include <libavutil/imgutils.h>
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
};
#endif
#endif

#define USE_DSHOW 0

#include "IKAROS.h"

class InputVideo: public Module
{
public:
    static Module * Create(Parameter * p) { return new InputVideo(p); }

    InputVideo(Parameter * p);
    virtual ~InputVideo();
    
    void            Init();
    void            Tick();
    
    int				size_x;
    int				size_y;
    
    float           frameRate;
    int             id;
    
    float *			intensity;
    float *			red;
    float *			green;
    float *			blue;
    bool            listDevices;
    
    // FFmpeg related
    AVFormatContext *input_format_context;
    int             videoStreamId;
    AVCodec         *input_codec;
    AVCodecContext  *avctx;
    AVFrame         *inputFrame;
    AVFrame         *outputFrame;
    AVPacket        packet;
    
    int decode(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt);
};
#endif
