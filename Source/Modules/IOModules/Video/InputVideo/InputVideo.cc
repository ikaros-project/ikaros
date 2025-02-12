//
//	  InputVideo.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2016 Birger Johansson
//

#define __STDC_CONSTANT_MACROS
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>
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
    AVCodecContext *avctx;
    AVFrame *inputFrame;
    AVFrame *outputFrame;
    AVPacket packet;
    SwsContext *img_convert_ctx;
    AVDictionary *options = NULL;



    void 
    Init()
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

        // Setting options
        std::string sizeString = size_x.as_int_string() + "x" + size_y.as_int_string();
        std::string frameRateString = frameRate.as_int_string();
        std::string idString = id.as_int_string();

        input_format_context = avformat_alloc_context();
        avdevice_register_all();
        if(listDevices)
        {
            AVFormatContext *input_format_context = avformat_alloc_context();
            AVDictionary *options = NULL;
            av_dict_set(&options, "list_devices", "true", 0);
            const AVInputFormat *iformat = av_find_input_format("avfoundation");
            avformat_open_input(&input_format_context, "", iformat, &options); 
        }

        const AVInputFormat *ifmt = av_find_input_format("avfoundation");
        
        av_dict_set(&options, "video_size", sizeString.c_str(), 0);
        av_dict_set(&options, "pixel_format", "1", 0);
        av_dict_set(&options, "framerate", frameRateString.c_str(), 0);

        if(avformat_open_input(&input_format_context, idString.c_str(), ifmt, &options) != 0)
            throw fatal_error("Couldn't open input stream.");

        av_log_set_level(AV_LOG_FATAL);

        if(avformat_find_stream_info(input_format_context, NULL) < 0)
            throw fatal_error("Couldn't find stream information");

        // Find the first video stream
        videoStreamId = -1;
        for (int i = 0; i < input_format_context->nb_streams; i++)
            if(input_format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                videoStreamId = i;
                break;
            }

        if(videoStreamId == -1)
            throw fatal_error("Didn't find a video stream");

        const AVCodec *input_codec = avcodec_find_decoder(input_format_context->streams[videoStreamId]->codecpar->codec_id);
        if(input_codec == NULL)
            throw fatal_error("Unsupported codec!");

        avctx = avcodec_alloc_context3(input_codec);
        if(!avctx)
        {
            avformat_close_input(&input_format_context);
            throw fatal_error("Could not allocate a decoding context");
        }

        if(avcodec_parameters_to_context(avctx, (*input_format_context).streams[videoStreamId]->codecpar) < 0)
        {
            avformat_close_input(&input_format_context);
            avcodec_free_context(&avctx);
            // Is error?
            return;
        }

        if(avcodec_open2(avctx, input_codec, NULL) < 0)
            throw fatal_error("Could not open codec");

        inputFrame = av_frame_alloc(); // Input
        if(inputFrame == NULL)
            throw fatal_error("Could not allocate AVFrame");

        outputFrame = av_frame_alloc(); // Output (after resize and convertions)
        outputFrame->format = AV_PIX_FMT_RGB24;
        outputFrame->width = size_x;
        outputFrame->height = size_y;
        av_image_alloc(outputFrame->data, outputFrame->linesize, size_x, size_y, AV_PIX_FMT_RGB24, 32);

        if(outputFrame == NULL)
            throw fatal_error("Could not allocate AVFrame");
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



    void 
    Tick()
    {
        constexpr float c1_3 = 1.0 / 3.0;
        constexpr float c1_255 = 1.0 / 255.0;

        int sx = size_x;
        int sy = size_y;

        int gotFrame = 0;
        while (!gotFrame) // Keep reading from source until we get a video packet
        {
            int ret = 0;
            while (ret < 0)
            {
                ret = av_read_frame(input_format_context, &packet);
                av_packet_unref(&packet);
            }

            if(av_read_frame(input_format_context, &packet) >= 0 && packet.stream_index == videoStreamId) // Read packet from source. Waiting until data is there! And packet from the video stream?
                decode(avctx, inputFrame, &gotFrame, &packet);
            else
                decode(avctx, inputFrame, &gotFrame, NULL); // No more input but decoder may have more frames

            if(gotFrame) // Decode gave us a frame
            {
                img_convert_ctx = sws_getCachedContext(img_convert_ctx, avctx->width, avctx->height, avctx->pix_fmt, sx, sy, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL); // Convert the frame to AV_PIX_FMT_RGB24 format
                sws_scale(img_convert_ctx, (const uint8_t *const *)inputFrame->data, inputFrame->linesize, 0, avctx->height, outputFrame->data, outputFrame->linesize);  // Scale the image

                // Copy and convert data to ikaros outputs

                unsigned char *data = outputFrame->data[0];
                float * r = output[0].data();
                float * g = output[1].data();
                float * b = output[2].data();

                float * r_ = red.data();
                float * g_ = green.data();
                float * b_ = blue.data();

                float * intens = intensity.data();
                int p = 0;
                int row_start = 0;

                for (int row = 0; row < sy; row++)
                {
                    for (int col = 0; col < sx; col++)
                    {
                        int y1 = row * sx;                              // y1 = y * size_x;
                        int xy = col + y1;                              // xy = x + y1;
                        int x3 = col * 3;                               // x3 = x * 3;

                        intens[p] =  r[p] = r_[p] = c1_255 * data[row_start + x3 + 0];
                        intens[p] += g[p] = g_[p] = c1_255 * data[row_start + x3 + 1];
                        intens[p] += b[p] = b_[p] = c1_255 * data[row_start + x3 + 2];
                        intens[p] *= c1_3;

                        p++;
                    }
                    row_start += outputFrame->linesize[0]; 
                }
                av_packet_unref(&packet);
            }
        }
    }



    ~InputVideo()
    {
        if (inputFrame)
            av_frame_free(&inputFrame);

        if (outputFrame) {
            av_freep(&outputFrame->data[0]); // Free data allocated by av_image_alloc()
            av_frame_free(&outputFrame);
        }

        if (avctx) 
            avcodec_free_context(&avctx);

        if (input_format_context)
            avformat_close_input(&input_format_context);

        if (options)
            av_dict_free(&options);
    }
};

INSTALL_CLASS(InputVideo)
