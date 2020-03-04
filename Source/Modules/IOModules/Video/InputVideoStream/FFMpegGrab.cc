//
//	  FFMpegGrab.cc		This file is a part of the IKAROS project
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

#include "FFMpegGrab.h"
#include "IKAROS.h"
using namespace ikaros;

FFMpegGrab::FFMpegGrab()
{	
	kernel().Notify(msg_debug, "FFMpegGrab Create()\n");

	//av_register_all();
	avformat_network_init();
	av_log_set_level(AV_LOG_INFO);
}

bool FFMpegGrab::Init()
{
	kernel().Notify(msg_debug, "FFMpegGrab Init()\n");

	AVInputFormat *file_iformat = NULL;
	// uv4l uses raw h264
	if (uv4l)
		file_iformat = av_find_input_format("h264");
	
	AVDictionary *options = NULL;

	if (uv4l)
		av_dict_set(&options, "probesize", "192", 0);

	/// Open video file
	if(avformat_open_input(&input_format_context, url, file_iformat, &options) != 0) // Allocating input_format_context
	{
		kernel().Notify(msg_fatal_error, "FFMpegGrab: Could not open file %s\n",url);
		return false;
	}
	av_dict_free(&options);
	
	// Retrieve stream information
	//if(avformat_find_stream_info(input_format_context, NULL) < 0)
	if(av_find_best_stream(input_format_context, AVMEDIA_TYPE_VIDEO,0,0,NULL,0))
	{
		kernel().Notify(msg_fatal_error, "FFMpegGrab: Couldn't find stream information\n");
		return false;
	}
	
	if (printInfo)
		av_dump_format(input_format_context, 0, url, 0);
	
	av_log_set_level(AV_LOG_ERROR);

	/// Find the first video stream
	videoStreamId = -1;
	for(int i=0; i<input_format_context->nb_streams; i++)
	{
		if(input_format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStreamId=i;
			break;
		}
	}
	if(videoStreamId==-1)
	{
		kernel().Notify(msg_fatal_error, "FFMpegGrab: Didn't find a video stream\n");
		return false;
	}
	
	// Prepare decoder
	if (uv4l)
	{
		input_codec = avcodec_find_decoder_by_name ("h264_mmal");
		if (input_codec)
			kernel().Notify(msg_debug, "FFMpegGrab:Using HW decoding (mmal)\n");
		else
			input_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	}
	else // Other stream formats
		input_codec = avcodec_find_decoder(input_format_context->streams[videoStreamId]->codecpar->codec_id);
	
	if(input_codec==NULL)
	{
		kernel().Notify(msg_fatal_error, "FFMpegGrab: Unsupported codec!\n");
		return false;
	}
	
	// Creating decoding context
	avctx = avcodec_alloc_context3(input_codec);
	if (!avctx)
	{
		kernel().Notify(msg_fatal_error, "FFMpegGrab: Could not allocate a decoding context\n");
		avformat_close_input(&input_format_context);
		return false;
	}
	
	// Setting parmeters to context?
	int error = avcodec_parameters_to_context(avctx, (*input_format_context).streams[videoStreamId]->codecpar);
	if (error < 0)
	{
		avformat_close_input(&input_format_context);
		avcodec_free_context(&avctx);
		kernel().Notify(msg_fatal_error, "FFMpegGrab: Could not set paramters to context\n");
		return false;
		
	}
	/// Open codec
	if( avcodec_open2(avctx, input_codec, NULL) < 0 )
	{
		kernel().Notify(msg_fatal_error, "FFMpegGrab: Could not open codec\n");
		return false;
	}
	
	inputFrame = av_frame_alloc();    // Input
	if(inputFrame == NULL)
	{
		kernel().Notify(msg_fatal_error, "FFMpegGrab: Could not allocate AVFrame\n");
		return false;
	}
	
	// This is not always working.
	inputSizeX = int(avctx->width);
	inputSizeY = int(avctx->height);
	
	if(inputSizeX == 0 and inputSizeY == 0)
		kernel().Notify(msg_warning, "FFMpegGrab: Could not figure out resolution of stream at init. Will scale output to size_x size_y parameters. \n");

	
	// Outputframe
	outputFrame = av_frame_alloc();   		// Output (after resize and convertions)
	outputFrame->format = AV_PIX_FMT_RGB24;
	outputFrame->width  = outputSizeX;
	outputFrame->height = outputSizeY;
	
	av_image_alloc(outputFrame->data, outputFrame->linesize, outputFrame->width, outputFrame->height, AV_PIX_FMT_RGB24, 32);
	
	if(outputFrame==NULL)
	{
		kernel().Notify(msg_fatal_error, "FFMpegGrab: Could not allocate AVFrame\n");
		return false;
	}
	
	// Function pointer and thread
	typedef  void (FFMpegGrab::*FuncMemFn)();
	FuncMemFn p = &FFMpegGrab::loop;
	std::thread (p,this).detach();
	
	return true;
}


FFMpegGrab::~FFMpegGrab()
{
	// Stop ffmpeg loop
	shutdown = true;
	while (!shutdownComplete) {
		sleep(1);
	}
	kernel().Notify(msg_debug, "FFMpegGrab: Shuting down complete\n");
	delete ikarosFrame;
	av_freep(&inputFrame->data[0]);
	av_free(inputFrame);
	av_freep(&outputFrame->data[0]);
	av_free(outputFrame);
	av_free(buffer);
	avcodec_close(avctx);
	avformat_close_input(&input_format_context);
}

void FFMpegGrab::loop()
{
	
#ifdef FFMPEGTIMER
	Timer timerSub;
	Timer timer3;
#endif
	
	std::mutex mtx;           // mutex for critical section
	
	if (!ikarosFrame)
		ikarosFrame = new uint8_t[outputSizeX*outputSizeY*3*sizeof(uint8_t)];
	
	int gotFrame = 0;
	while (true)                                            	// Always loop
	{
		if(av_read_frame(input_format_context, &packet)>=0)     // Read packet from source
		{
			if(packet.stream_index==videoStreamId)              // Is this a packet from the video stream?
			{
#ifdef FFMPEGTIMER
				timerSub.Restart();
#endif
				decode(avctx, inputFrame, &gotFrame, &packet);
				if (gotFrame)                                   // Decoder gave us a video frame
				{
						

#ifdef FFMPEGTIMER
					printf("FFMpegGrab: Decoded Time %f\n",timerSub.GetTime());
					timerSub.Restart();
#endif

					//	Convert the frame to AV_PIX_FMT_RGB24 format
					static struct SwsContext *img_convert_ctx;
					img_convert_ctx = sws_getCachedContext(img_convert_ctx,avctx->width, avctx->height,
														   avctx->pix_fmt,
														   outputSizeX, outputSizeY, AV_PIX_FMT_RGB24,
														   SWS_BICUBIC, NULL, NULL, NULL);
#ifdef FFMPEGTIMER
					printf("FFMpegGrab: sws_getCachedContext Time %f\n",timerSub.GetTime());
					timerSub.Restart();
#endif
					// Scale the image
					sws_scale(img_convert_ctx, (const uint8_t * const *)inputFrame->data,
							  inputFrame->linesize, 0, avctx->height,
							  outputFrame->data, outputFrame->linesize);
					
					bool waitForCollected = true;
#ifdef FFMPEGTIMER
					printf("FFMpegGrab: sws_scale Time %f\n",timerSub.GetTime());
					timerSub.Restart();
#endif
					mtx.lock();
					freshData = true;
					memcpy(ikarosFrame, outputFrame->data[0], outputSizeX*outputSizeY*3*sizeof(uint8_t));
					//printf("FFMpegGrab: new framme\n");

					mtx.unlock();
#ifdef FFMPEGTIMER
					printf("FFMpegGrab: memcpy Time %f\n",timerSub.GetTime());
					timerSub.Restart();
#endif
					if (syncronized)
						while (waitForCollected) // Wait unitil gotFrame is false ==  tick has taken the frame
						{
							usleep(1000);
							mtx.lock();
							waitForCollected = freshData; // If freshdata is set to false then assume data collected
							mtx.unlock();
						}
#ifdef FFMPEGTIMER
					printf("FFMpegGrab: syncronized %f\n",timerSub.GetTime());
					timerSub.Restart();
#endif
					gotFrame = 0;
					if (printInfo)
					{
						FPS = FPS + (1.0/timer.GetTime()*1000.0-FPS)*0.02; // Moving avarage mean FPS
						timer.Restart();
						kernel().Notify(msg_print, "FFMpegGrab %s: FPS %.0f\n",url, FPS);

					}
					if (shutdown)
					{
						if (printInfo)
							kernel().Notify(msg_debug, "FFMpegGrab: Shuting down\n");
						break;
					}
				}
			}
		}
		av_packet_unref(&packet);
	}
	shutdownComplete = true;
}
int FFMpegGrab::decode(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt)
{
	int ret;
	*got_frame = 0;
	if (pkt) {
		ret = avcodec_send_packet(avctx, pkt);
		if (ret != 0)
			return ret == AVERROR_EOF ? 0 : ret;
	}
	ret = avcodec_receive_frame(avctx, frame);
	if (ret != 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
		return ret;
	if (ret == 0)
		*got_frame = 1;
	return 0;
}


