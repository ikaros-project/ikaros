//
//	  InputVideoStream.cc		This file is a part of the IKAROS project
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
//
// Code inspired from:
// https://www.ffmpeg.org/doxygen/2.8/examples.html
// http://dranger.com/ffmpeg/
// https://sourceforge.net/u/leixiaohua1020/profile/
// https://blogs.gentoo.org/lu_zero/2016/03/29/new-avcodec-api/
// And enormous amount of googleing...

//#define DEBUGTIMER


#define __STDC_CONSTANT_MACROS
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>
}

#include "FFMpegGrab.h"

#include "ikaros.h"

using namespace ikaros;

class InputVideoStream : public Module
{
	//parameter frameRate;
	parameter id;
	parameter size_x;
	parameter size_y;
	parameter listDevices;
	parameter printInfo;
	parameter url;
	parameter uv4l;
	parameter syncronized_framegrabber;
	parameter syncronized_tick;

	matrix intensity;
	matrix red;
	matrix green;
	matrix blue;
	matrix output;

	// FFmpeg related
	AVFormatContext *input_format_context;
	int videoStreamId;
	AVCodecContext *avctx;
	AVFrame *inputFrame;
	AVFrame *outputFrame;
	AVPacket packet;
	SwsContext *img_convert_ctx;
	AVDictionary *options = NULL;

	FFMpegGrab *framegrabber;
	uint8_t *newFrame;
	float convertIntToFloat[256];

	void
	Init()
	{
		Bind(intensity, "INTENSITY");
		Bind(red, "RED");
		Bind(green, "GREEN");
		Bind(blue, "BLUE");
		Bind(output, "OUTPUT");

		Bind(size_x, "size_x");
		Bind(size_y, "size_y");
		Bind(url, "url");
		Bind(printInfo, "info");
		Bind(uv4l, "uv4l");
		Bind(syncronized_framegrabber, "syncronized_framegrabber");
		Bind(syncronized_tick, "syncronized_tick");

		// Create a int to float table to speed up int to float cast.
		for (int i = 0; i <= 255; i++)
			convertIntToFloat[i] = 1.0 / 255.0 * i;

		// Create a grabber
		try
		{
			framegrabber = new FFMpegGrab();
		}
		catch(const std::exception& e)
		{
			Notify(msg_fatal_error, "Can not create frame grabber");
		}
		

		// Set paramters
		framegrabber->uv4l = uv4l;
		framegrabber->url = url.c_str(); // copy string?
		framegrabber->printInfo = printInfo;
		framegrabber->outputSizeX = size_x;
		framegrabber->outputSizeY = size_y;
		framegrabber->syncronized = syncronized_framegrabber;

		// Init grabber
		if (!framegrabber->Init())
			Notify(msg_fatal_error, "Can not start frame grabber");

		// Create memory
		newFrame = new uint8_t[size_x * size_y * 3 * sizeof(uint8_t)];
	}

	void
	Tick()
	{
		std::mutex mtx; // mutex for critical section
		Timer timer;

		// Modes
		// Get all frames (waitForNewData + syncronized)
		// Get latest frame	(waitForNewData + !syncronized)
		// Get a frame every tick (could be the same as last tick) 	(!waitForNewData + !syncronized)

		bool waitForNewData = syncronized_tick;
		bool gotNewData = false;

		mtx.lock();
		gotNewData = framegrabber->freshData;
		mtx.unlock();

		if (!gotNewData and waitForNewData) // No new data
			while (!gotNewData)				// wait until new data
			{
				mtx.lock();
				gotNewData = framegrabber->freshData;
				mtx.unlock();
				usleep(1000);
			}

#ifdef DEBUGTIMER
		if (gotNewData)
		{
			printf("InputVideoStream:We got an new image Time %f sec\n", timer.GetTime());
			timer.Restart();
		}
#endif

		if (!gotNewData) // No need to int->float convert again. Use last output.
			return;

		mtx.lock();
		memcpy(newFrame, framegrabber->frame, size_x * size_y * 3 * sizeof(uint8_t));
		framegrabber->freshData = 0;
		mtx.unlock();
#ifdef DEBUGTIMER
		printf("InputVideoStream: memcpy %f\n", timer.GetTime());
		timer.Restart();
#endif
		unsigned char *data = newFrame;

		float *r = red;
		float *g = green;
		float *b = blue;
		float *inte = intensity;
		int t = 0;

		const float c13 = 1.0 / 3.0;

		// Singel core
		while (t++ < size_x * size_y)
		{
			*r = convertIntToFloat[*data++];
			*g = convertIntToFloat[*data++];
			*b = convertIntToFloat[*data++];
			*inte++ = (*r++ + *g++ + *b++) * c13;
		}

		// Fill output
		
		output[0].copy(red);
		output[1].copy(green);
		output[2].copy(blue);

#ifdef DEBUGTIMER
		printf("InputVideoStream: float convert %f\n", timer.GetTime());
		timer.Restart();
#endif
	}

	~InputVideoStream()
	{
		delete newFrame;
		delete framegrabber;
	}
};

INSTALL_CLASS(InputVideoStream)