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

#include "FFMpegGrab.h"

using namespace ikaros;

FFMpegGrab::FFMpegGrab()
{
	if (printInfo)
		std::cout << "FFMpegGrab()" << std::endl;

	avformat_network_init();
	av_log_set_level(AV_LOG_INFO); // This must be set to be able to use av_dump_format. Later this wil be set to AV_LOG_ERROR
}

bool FFMpegGrab::Init()
{
	if (printInfo)
		std::cout << "FFMpegGrab Init()" << std::endl;

	const AVInputFormat *file_iformat = NULL;
	// uv4l uses raw h264
	if (uv4l)
		file_iformat = av_find_input_format("h264");

	AVDictionary *options = NULL;

	if (uv4l)
		av_dict_set(&options, "probesize", "192", 0);

	/// Open video file
	if (avformat_open_input(&input_format_context, url, file_iformat, &options) != 0) // Allocating input_format_context
	{
		std::cout << "FFMpegGrab: Could not open file " << url << std::endl;
		return false;
	}
	av_dict_free(&options);

	// Retrieve stream information
	// if(avformat_find_stream_info(input_format_context, NULL) < 0)
	if (av_find_best_stream(input_format_context, AVMEDIA_TYPE_VIDEO, 0, 0, NULL, 0))
	{
		std::cout << "FFMpegGrab: Couldn't find stream information" << std::endl;
		return false;
	}

	if (printInfo)
		av_dump_format(input_format_context, 0, url, 0);

	av_log_set_level(AV_LOG_ERROR);

	/// Find the first video stream
	videoStreamId = -1;
	for (int i = 0; i < input_format_context->nb_streams; i++)
	{
		if (input_format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoStreamId = i;
			break;
		}
	}
	if (videoStreamId == -1)
	{
		std::cout << "FFMpegGrab: Didn't find a video stream" << std::endl;
		return false;
	}

	// Prepare decoder
	if (uv4l)
	{
		input_codec = avcodec_find_decoder_by_name("h264_mmal"); // Using mmal for uv4l for raspberry pi
		if (input_codec)
		{
			std::cout << "FFMpegGrab: Using HW decoding (mmal)" << std::endl;
		}
		else
			input_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	}
	else // Other stream formats
		input_codec = avcodec_find_decoder(input_format_context->streams[videoStreamId]->codecpar->codec_id);

	if (input_codec == NULL)
	{
		std::cout << "FFMpegGrab: Unsupported codec!" << std::endl;
		return false;
	}

	// Creating decoding context
	avctx = avcodec_alloc_context3(input_codec);
	if (!avctx)
	{
		std::cout << "FFMpegGrab: Could not allocate a decoding context" << std::endl;
		avformat_close_input(&input_format_context);
		return false;
	}

	// Setting parmeters to context?
	int error = avcodec_parameters_to_context(avctx, (*input_format_context).streams[videoStreamId]->codecpar);
	if (error < 0)
	{
		avformat_close_input(&input_format_context);
		avcodec_free_context(&avctx);
		std::cout << "FFMpegGrab: Could not set parameters to context" << std::endl;
		return false;
	}
	/// Open codec
	if (avcodec_open2(avctx, input_codec, NULL) < 0)
	{
		std::cout << "FFMpegGrab: Could not open codec" << std::endl;
		return false;
	}

	inputFrame = av_frame_alloc(); // Input
	if (inputFrame == NULL)
	{
		std::cout << "FFMpegGrab: Could not allocate AVFrame" << std::endl;
		return false;
	}

	// Outputframe
	outputFrame = av_frame_alloc(); // Output (after resize and convertions)
	outputFrame->format = AV_PIX_FMT_RGB24;
	outputFrame->width = outputSizeX;
	outputFrame->height = outputSizeY;

	av_image_alloc(outputFrame->data, outputFrame->linesize, outputFrame->width, outputFrame->height, AV_PIX_FMT_RGB24, 32);

	if (outputFrame == NULL)
	{
		std::cout << "FFMpegGrab: Could not allocate AVFrame" << std::endl;
		return false;
	}

	// Function pointer and thread
	typedef void (FFMpegGrab::*FuncMemFn)();
	FuncMemFn p = &FFMpegGrab::loop;
	std::thread(p, this).detach();

	return true;
}

FFMpegGrab::~FFMpegGrab()
{
	// Stop ffmpeg loop
	shutdown = true;
	while (!shutdownComplete)
	{
		sleep(1);
	}
	delete frame;
	av_freep(&inputFrame->data[0]);
	av_free(inputFrame);
	av_freep(&outputFrame->data[0]);
	av_free(outputFrame);
	avcodec_free_context(&avctx);
	avformat_close_input(&input_format_context);
}

void FFMpegGrab::loop()
{

#ifdef FFMPEG_TIMER
	Timer timerSub;
#endif

	std::mutex mtx; // mutex for critical section

	if (!frame)
		frame = new uint8_t[outputSizeX * outputSizeY * 3 * sizeof(uint8_t)];

	int gotFrame = 0;
	while (true) // Always loop
	{
		if (av_read_frame(input_format_context, &packet) >= 0) // Read packet from source
		{
			if (packet.stream_index == videoStreamId) // Is this a packet from the video stream?
			{
#ifdef FFMPEG_TIMER
				timerSub.Restart();
#endif
				decode(avctx, inputFrame, &gotFrame, &packet);
				if (gotFrame) // Decoder gave us a video frame
				{

#ifdef FFMPEG_TIMER
					std::cout << "FFMpegGrab (timing): Decoded Time\t" << round(timerSub.GetTime() / 1000) << " ms" << std::endl;
					timerSub.Restart();
#endif

					//	Convert the frame to AV_PIX_FMT_RGB24 format
					img_convert_ctx = sws_getCachedContext(img_convert_ctx, avctx->width, avctx->height,
														   avctx->pix_fmt,
														   outputSizeX, outputSizeY, AV_PIX_FMT_RGB24,
														   SWS_BICUBIC, NULL, NULL, NULL);
#ifdef FFMPEG_TIMER
					std::cout << "FFMpegGrab (timing): sws_getCachedC.. \t" << round(timerSub.GetTime() / 1000) << " ms" << std::endl;
					timerSub.Restart();
#endif
					// Scale the image
					sws_scale(img_convert_ctx, (const uint8_t *const *)inputFrame->data,
							  inputFrame->linesize, 0, avctx->height,
							  outputFrame->data, outputFrame->linesize);

					bool waitForCollected = true;
#ifdef FFMPEG_TIMER
					std::cout << "FFMpegGrab (timing): sws_scale Time\t" << round(timerSub.GetTime() / 1000) << " ms" << std::endl;
					timerSub.Restart();
#endif
					mtx.lock();
					freshData = true;
					// Copy each row to have only the data sent to ikaros. This is because the linesize is not the same as the size of the image.
					for (int y = 0; y < outputSizeY; y++)
						memcpy(frame + y * outputSizeX * 3,
							   outputFrame->data[0] + y * outputFrame->linesize[0],
							   outputSizeX * 3);
					mtx.unlock();
#ifdef FFMPEG_TIMER
					std::cout << "FFMpegGrab (timing): memcpy Time\t" << round(timerSub.GetTime() / 1000) << " ms" << std::endl;
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
#ifdef FFMPEG_TIMER
					std::cout << "FFMpegGrab (timing): synchronized\t" << round(timerSub.GetTime() / 1000) << " ms\n" << std::endl;
					timerSub.Restart();
#endif
					gotFrame = 0;
#ifdef FFMPEG_FPS

					FPS = FPS + (1.0 / timer.GetTime() - FPS) * 0.02; // Moving avarage mean FPS
					timer.Restart();
					std::cout << "FFMpegGrab " << url << ": FPS " << FPS << std::endl;
#endif
					if (shutdown)
					{
						if (printInfo)
							std::cout << "FFMpegGrab: Shutting down" << std::endl;
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
	if (pkt)
	{
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
