//
//	  FFMpegGrab.h		This file is a part of the IKAROS project
//
//    Copyright (C) 2018 Birger Johansson
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

#ifndef FFMpegGrab_h
#define FFMpegGrab_h

//#define FFMPEGLOG
//#define FFMPEGTIMER

#include "ikaros.h"


#include <stdio.h>
#include <unistd.h>		// usleep
#include <thread>		// std::thread
#include <mutex>		// std::mutex

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class FFMpegGrab
{
	public:
	
	FFMpegGrab();
	~FFMpegGrab();
	bool Init();
	void loop();

	// FFmpeg related
	AVFormatContext *input_format_context;
	int             videoStreamId;
	const AVCodec   *input_codec;
	AVCodecContext  *avctx;
	AVFrame         *inputFrame;
	AVFrame         *outputFrame;
	AVPacket        packet;
	int             numBytes;
	uint8_t         *buffer;
	int 			inputSizeX;
	int 			inputSizeY;
	int 			outputSizeX;
	int 			outputSizeY;
    SwsContext      *img_convert_ctx;
	
	uint8_t			*ikarosFrame;
	int 			ikarosFrameSize;

	int decode(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt);
	
	bool 			uv4l;
	const char *	url;
	bool 			printInfo;
	bool 			syncronized; // Syncing with tick
	bool 			freshData = false;
	
	float 			FPS;
	Timer 			timer;
	
	
	private:
	bool 			shutdown = false;
	bool 			shutdownComplete = false;
};

#endif
