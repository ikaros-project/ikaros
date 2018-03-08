//
//	  OutputVideoFile.h		This file is a part of the IKAROS project
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


#ifndef OutputVideoFile_
#define OutputVideoFile_

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>

}

#include "IKAROS.h"

class OutputVideoFile: public Module
{
public:
    static Module * Create(Parameter * p) { return new OutputVideoFile(p); }
	OutputVideoFile(Parameter * p) : Module(p) {}
    virtual ~OutputVideoFile();
	
    void            Init();
    void            Tick();
    
    const char *	filename;
    
    int				size_x;
    int				size_y;
    
    float *			intensity;
    float *			red;
    float *			green;
    float *			blue;
    bool            printInfo;
    int             quality;
    bool            color;
    int             frameRate;
    
    // FFmpeg related
    AVCodec         *output_codec;
    AVCodecContext  *c;
    AVStream        *audio_st, *video_st;
    AVFrame         *inputFrame;
    AVFrame         *outputFrame;
    AVPacket        pkt;
    int             i, ret, got_output;
    AVFormatContext *output_format_context;
    AVOutputFormat  *fmt;
    
    int encode(AVCodecContext *avctx, AVPacket *pkt, int *got_packet, AVFrame *frame);
};

#endif
