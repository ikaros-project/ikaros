//
//	  InputVideo.cc		This file is a part of the IKAROS project
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
// Code inspired from:
// https://www.ffmpeg.org/doxygen/2.8/examples.html
// http://dranger.com/ffmpeg/
// https://sourceforge.net/u/leixiaohua1020/profile/
// https://blogs.gentoo.org/lu_zero/2016/03/29/new-avcodec-api/

// Todo: Add rotation filter

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
// Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include <libavutil/imgutils.h>
};
#else
// Linux...
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

// Show Dshow Device
void show_dshow_device()
{
    AVFormatContext *input_format_context = avformat_alloc_context();
    AVDictionary *options = NULL;
    av_dict_set(&options, "list_devices", "true", 0);
    const AVInputFormat *iformat = av_find_input_format("dshow");
    printf("========Device Info=============\n");
    avformat_open_input(&input_format_context, "video=dummy", iformat, &options);
    printf("================================\n");
}

// Show Dshow Device Option
void show_dshow_device_option()
{
    AVFormatContext *input_format_context = avformat_alloc_context();
    AVDictionary *options = NULL;
    av_dict_set(&options, "list_options", "true", 0);
    const AVInputFormat *iformat = av_find_input_format("dshow");
    printf("========Device Option Info======\n");
    avformat_open_input(&input_format_context, "video=Integrated Camera", iformat, &options);
    printf("================================\n");
}

// Show VFW Device
void show_vfw_device()
{
    AVFormatContext *input_format_context = avformat_alloc_context();
    const AVInputFormat *iformat = av_find_input_format("vfwcap");
    printf("========VFW Device Info======\n");
    avformat_open_input(&input_format_context, "list", iformat, NULL);
    printf("=============================\n");
}

// Show AVFoundation Device
void show_avfoundation_device()
{
    AVFormatContext *input_format_context = avformat_alloc_context();
    AVDictionary *options = NULL;
    av_dict_set(&options, "list_devices", "true", 0);
    const AVInputFormat *iformat = av_find_input_format("avfoundation");
    printf("==AVFoundation Device Info===\n");
    avformat_open_input(&input_format_context, "", iformat, &options);
    printf("=============================\n");
}

#include "ikaros.h"

using namespace ikaros;

class InputVideo : public Module
{
    parameter frameRate;
    parameter id;
    parameter size_x;
    parameter size_y;
    parameter listDevices;

    matrix intensity;
    matrix red;
    matrix green;
    matrix blue;
    matrix output;

    // FFmpeg related
    AVFormatContext *input_format_context;
    int videoStreamId;
    // AVCodec         *input_codec;
    AVCodecContext *avctx;
    AVFrame *inputFrame;
    AVFrame *outputFrame;
    AVPacket packet;
    SwsContext *img_convert_ctx;

    void Init()
    {
        Bind(intensity, "INTENSITY");
        Bind(red, "RED");
        Bind(green, "GREEN");
        Bind(blue, "BLUE");
        Bind(output, "OUTPUT");

        Bind(frameRate, "frame_rate");
        Bind(id, "id");
        Bind(size_x, "size_x");
        Bind(size_y, "size_y");
        Bind(listDevices, "list_devices");

        // av_register_all(); // Register all formats and codecs
        input_format_context = avformat_alloc_context();
        avdevice_register_all(); // libavdevice

#ifdef _WIN32
        if(listDevices)
        {
            listDevices
            // Show Dshow Device
            show_dshow_device();
            // Show Device Options
            show_dshow_device_option();
            // Show VFW Options
            show_vfw_device();
        }
#if USE_DSHOW
        const AVInputFormat *ifmt = av_find_input_format("dshow");
        // Set own video device's name
        if(avformat_open_input(&input_format_context, "video=Integrated Camera", ifmt, NULL) != 0)
        {
            Notify(msg_fatal_error, "Couldn't open input stream.\n");
            return;
        }
#else
        const AVInputFormat *ifmt = av_find_input_format("vfwcap");
        if(avformat_open_input(&input_format_context, "0", ifmt, NULL) != 0)
        {
            Notify(msg_fatal_error, "Couldn't open input stream.\n");
            return;
        }
#endif
#elif defined LINUX
        const AVInputFormat *ifmt = av_find_input_format("video4linux2");
        if(avformat_open_input(&input_format_context, "/dev/video0", ifmt, NULL) != 0)
        {
            Notify(msg_fatal_error, "Couldn't open input stream.\n");
            return;
        }
#else
        if(listDevices)
        show_avfoundation_device();

        const AVInputFormat *ifmt = av_find_input_format("avfoundation");
        AVDictionary *options = NULL;

        // Setting options.
        std::string sizeString = std::to_string(size_x.as_int()) + std::string("x") + std::to_string(size_y.as_int());
        std::string frameRateString = std::to_string(frameRate.as_int());
        std::string idString = std::to_string(id.as_int());
        av_dict_set(&options, "video_size", sizeString.c_str(), 0);
        
        av_dict_set(&options, "pixel_format", "1", 0);


        av_dict_set(&options, "framerate", frameRateString.c_str(), 0);

        if(avformat_open_input(&input_format_context, idString.c_str(), ifmt, &options) != 0)
        {
            printf("Couldn't open input stream.\n");
            Notify(msg_fatal_error, "Couldn't open input stream.\n");
            return;
        }
#endif

        av_log_set_level(AV_LOG_FATAL);

        /// Retrieve stream information
        if(avformat_find_stream_info(input_format_context, NULL) < 0)
        {
            Notify(msg_fatal_error, "Couldn't find stream information\n");
            return;
        }

        /// Find the first video stream
        videoStreamId = -1;
        for (int i = 0; i < input_format_context->nb_streams; i++)
            if(input_format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                videoStreamId = i;
                break;
            }
        if(videoStreamId == -1)
        {
            Notify(msg_fatal_error, "Didn't find a video stream\n");
            return;
        }

        // Decoding video

        /// Find the decoder for the video stream
        const AVCodec *input_codec = avcodec_find_decoder(input_format_context->streams[videoStreamId]->codecpar->codec_id);
        if(input_codec == NULL)
        {
            Notify(msg_fatal_error, "Unsupported codec!\n");
            return; // Codec not found
        }

        // Creating decoding context
        avctx = avcodec_alloc_context3(input_codec);
        if(!avctx)
        {
            Notify(msg_fatal_error, "Could not allocate a decoding context\n");
            avformat_close_input(&input_format_context);
            return;
        }

        // Setting parmeters to context?
        int error = avcodec_parameters_to_context(avctx, (*input_format_context).streams[videoStreamId]->codecpar);
        if(error < 0)
        {
            avformat_close_input(&input_format_context);
            avcodec_free_context(&avctx);
            return;
        }

        /// Open codec
        if(avcodec_open2(avctx, input_codec, NULL) < 0)
        {
            Notify(msg_fatal_error, "Could not open codec\n");
            return;
        }

        inputFrame = av_frame_alloc(); // Input
        if(inputFrame == NULL)
        {
            Notify(msg_fatal_error, "Could not allocate AVFrame\n");
            return;
        }

        outputFrame = av_frame_alloc(); // Output (after resize and convertions)
        outputFrame->format = AV_PIX_FMT_RGB24;
        outputFrame->width = size_x.as_int();
        outputFrame->height = size_y.as_int();
        av_image_alloc(outputFrame->data, outputFrame->linesize, size_x.as_int(), size_y.as_int(), AV_PIX_FMT_RGB24, 32);

        if(outputFrame == NULL)
        {
            Notify(msg_fatal_error, "Could not allocate AVFrame\n");
            return;
        }
    }

    void Tick()
    {
        constexpr float c13 = 1.0 / 3.0;
        constexpr float c1255 = 1.0 / 255.0;

        int gotFrame = 0;
        while (!gotFrame) // Keep reading from source until we get a video packet
        {
            int ret = 0;
            while (ret < 0)
            {
                ret = av_read_frame(input_format_context, &packet);
                av_packet_unref(&packet);
            }

            if(av_read_frame(input_format_context, &packet) >= 0) // Read packet from source. Waiting until data is there!
            {
                if(packet.stream_index == videoStreamId) // Is this a packet from the video stream?
                    decode(avctx, inputFrame, &gotFrame, &packet);
            }
            else
            {
                decode(avctx, inputFrame, &gotFrame, NULL); // No more input but decoder may have more frames
            }

            if(gotFrame) // Decode gave us a frame
            {
                // Convert the frame to AV_PIX_FMT_RGB24 format
                img_convert_ctx = sws_getCachedContext(img_convert_ctx, avctx->width, avctx->height,
                                                       avctx->pix_fmt,
                                                       size_x.as_int(), size_y.as_int(), AV_PIX_FMT_RGB24,
                                                       SWS_BICUBIC, NULL, NULL, NULL);
                // Scale the image
                sws_scale(img_convert_ctx, (const uint8_t *const *)inputFrame->data,
                          inputFrame->linesize, 0, avctx->height,
                          outputFrame->data, outputFrame->linesize);

                // Put data in ikaros output
                unsigned char *data = outputFrame->data[0];

                float * r = output[0].data();
                float * g = output[1].data();
                float * b = output[2].data();
                float * intens = intensity.data();

                int sx = size_x.as_int();
                int sy = size_y.as_int();
                int p = 0;

                for (int row = 0; row < sy; row++)
                    for (int col = 0; col < sx; col++)
                    {
                        int yLineSize = row * outputFrame->linesize[0]; // y
                        int y1 = row * sx;                              // y1 = y * size_x;
                        int xy = col + y1;                              // xy = x + y1;
                        int x3 = col * 3;                               // x3 = x * 3;

                        intens[p] = r[p] = c1255 * data[yLineSize + x3 + 0];
                        intens[p] += g[p] = c1255 * data[yLineSize + x3 + 1];
                        intens[p] += b[p] = c1255 * data[yLineSize + x3 + 2];
                        intens[p] *= c13;

                        p++;
                    }

                av_packet_unref(&packet);
            }
        }
    }

    int
    decode(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt)
    {
        int ret;
        *got_frame = 0;
        if(pkt)
        {
            ret = avcodec_send_packet(avctx, pkt);
            if(ret < 0)
                return ret == AVERROR_EOF ? 0 : ret;
        }
        ret = avcodec_receive_frame(avctx, frame);
        if(ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
            return ret;
        if(ret >= 0)
            *got_frame = 1;
        return 0;
    }
};

INSTALL_CLASS(InputVideo)
