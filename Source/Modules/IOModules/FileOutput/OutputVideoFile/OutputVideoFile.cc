//
//    OutputVideoFile.cc    This file is a part of the IKAROS project
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
//
//


#include "OutputVideoFile.h"

using namespace ikaros;

int OutputVideoFile::encode(AVCodecContext *avctx, AVPacket *pkt, int *got_packet, AVFrame *frame)
{
        int ret;
        *got_packet = 0;
        ret = avcodec_send_frame(avctx, frame);
        if (ret < 0)
                return ret;
        ret = avcodec_receive_packet(avctx, pkt);
        if (!ret)
                *got_packet = 1;
        if (ret == AVERROR(EAGAIN))
                return 0;
        return ret;
}

void
OutputVideoFile::Init()
{

        intensity = GetInputArray("INTENSITY");
        if(intensity)
        {
                //intensity  = GetInputArray("INTENSITY");
                size_x      = GetInputSizeX("INTENSITY");
                size_y      = GetInputSizeY("INTENSITY");
                color = false;
        }
        else
        {
                red         = GetInputArray("RED");
                green       = GetInputArray("GREEN");
                blue        = GetInputArray("BLUE");
                size_x      = GetInputSizeX("RED");
                size_y      = GetInputSizeY("RED");

                if (size_x != GetInputSizeX("GREEN") ||
                    size_x != GetInputSizeX("BLUE")  ||
                    size_y != GetInputSizeY("GREEN") ||
                    size_y != GetInputSizeY("BLUE"))
                {
                        Notify(msg_fatal_error, "Incorrect sizes for Color input.\n");
                        printf("RED %i,%i",GetInputSizeX("RED"),GetInputSizeY("RED"));
                        printf("GREEN %i,%i",GetInputSizeX("GREEN"),GetInputSizeY("GREEN"));
                        printf("BLUE %i,%i",GetInputSizeX("BLUE"),GetInputSizeY("BLUE"));
                        return;
                }
                color = true;
        }

        quality     =  GetIntValueFromList("quality");// Set the cfr paramter for the 264 encoder.
        frameRate  =  GetIntValue("frame_rate");

        // Register all formats and codecs
        av_log_set_level(AV_LOG_FATAL);

        filename = GetValue("filename");
        if (filename == NULL)
        {
                Notify(msg_fatal_error, "No output file parameter supplied.\n");
                return;
        }
        if (filename[strlen(filename)-4] != '.' ||
            filename[strlen(filename)-3] != 'm' ||
            filename[strlen(filename)-2] != 'p' ||
            filename[strlen(filename)-1] != '4')
        {
                Notify(msg_fatal_error, "Filename must end with .mp4\n");
                return;
        }

        // Find the h264 codec
        output_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (!output_codec) {
                Notify(msg_fatal_error, "Codec not found\n");
        }
        // Alocate context
        c = avcodec_alloc_context3(output_codec);
        if (!c) {
                Notify(msg_fatal_error, "Could not allocate video codec context\n");
                return;
        }

        // Alocate context for avformat
        avformat_alloc_output_context2(&output_format_context, NULL, NULL, filename);
        if (!output_format_context) {
                Notify(msg_fatal_error, "Could not allocate avformat context\n");
                return;
        }

        fmt = output_format_context->oformat;

        // Add streams
        video_st  = avformat_new_stream(output_format_context, NULL);
        if (!video_st) {
                Notify(msg_fatal_error, "Could not allocate stream\n");
                return;
        }
        //audio_st  = avformat_new_stream(output_format_context, NULL);
        video_st->id = output_format_context->nb_streams-1;


        switch ((output_codec)->type) {
        //        case AVMEDIA_TYPE_AUDIO:
        //            c->sample_fmt  = output_codec->sample_fmts ?
        //            output_codec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
        //            c->bit_rate    = 64000;
        //            c->sample_rate = 44100;
        //            if (output_codec->supported_samplerates) {
        //                c->sample_rate = output_codec->supported_samplerates[0];
        //                for (i = 0; output_codec->supported_samplerates[i]; i++) {
        //                    if (output_codec->supported_samplerates[i] == 44100)
        //                        c->sample_rate = 44100;
        //                }
        //            }
        //            c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
        //            c->channel_layout = AV_CH_LAYOUT_STEREO;
        //            if (output_codec->channel_layouts) {
        //                c->channel_layout = output_codec->channel_layouts[0];
        //                for (i = 0; output_codec->channel_layouts[i]; i++) {
        //                    if (output_codec->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
        //                        c->channel_layout = AV_CH_LAYOUT_STEREO;
        //                }
        //            }
        //            c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
        //            audio_st->time_base = (AVRational){ 1, c->sample_rate };
        //            break;

        case AVMEDIA_TYPE_VIDEO:
                c->codec_id     = output_codec->id;
                c->width        = size_x;
                c->height       = size_y;
                c->flags       |= AV_CODEC_FLAG_GLOBAL_HEADER;// Magic... Makes quicktime happy..
                c->time_base    = (AVRational){1,frameRate};
                c->gop_size     = 12;
                c->pix_fmt      = AV_PIX_FMT_YUV420P;
                break;

        default:
                break;
        }

        // Someone smart on the internet said "The range is a log scale of 0 to 51. 0 is lossless (files will likely be huge), 18 is often considered to be "visually lossless", 23 is default, and 51 is worst quality. Generally you use the highest value that still gives you an acceptable quality"
        switch (quality) {
        case 0:
                av_opt_set(c->priv_data, "crf", "45", 0); // verylow
                break;
        case 1:
                av_opt_set(c->priv_data, "crf", "35", 0); // low
                break;
        case 2:
                av_opt_set(c->priv_data, "crf", "23", 0); // normal
                break;
        case 3:
                av_opt_set(c->priv_data, "crf", "18", 0); // high
                break;
        default:
                break;
        }
        // More 264 tuning
        //    av_opt_set(c->priv_data, "preset", "veryslow", 0);
        //    av_opt_set(c->priv_data, "tune", "Film", 0);
        //    av_opt_set(c->priv_ data, "profile", "Main", 0);

        // Open codec
        ret = avcodec_open2(c, output_codec,  NULL);
        if (ret < 0) {
                Notify(msg_fatal_error, "Could not open video codec\n");
                return;
        }
        // Copy the stream parameters to the muxer
        ret = avcodec_parameters_from_context(video_st->codecpar, c);
        if (ret < 0) {
                Notify(msg_fatal_error, "Could not copy the stream parameters\n");
                return;
        }
        //av_dump_format(output_format_context, 0, filename, 1);

        // Open file
        if (!(fmt->flags & AVFMT_NOFILE)) {
                ret = avio_open(&output_format_context->pb, filename, AVIO_FLAG_WRITE);
                if (ret < 0) {
                        Notify(msg_fatal_error, "Could not open file\n");
                        return;
                }
        }

        // write stream header
        ret = avformat_write_header(output_format_context, NULL); // opt?
        if (ret < 0) {
                Notify(msg_fatal_error, "Error occurred when opening output file\n");
                return;
        }
}


void
OutputVideoFile::Tick()
{
        // Allocate the frame sent to the encoder (AV_PIX_FMT_YUV420P)
        outputFrame = av_frame_alloc();
        if (!outputFrame) {
                Notify(msg_fatal_error, "Could not allocate video frame\n");
                return;
        }
        outputFrame->format = AV_PIX_FMT_YUV420P;
        outputFrame->width  = c->width;
        outputFrame->height = c->height;
        ret = av_image_alloc(outputFrame->data, outputFrame->linesize, c->width, c->height, AV_PIX_FMT_YUV420P, 32);
        if (ret<0) {
                Notify(msg_fatal_error, "Could not allocate video frame data\n");
                return;
        }

        // Allocate the frame to fill with ikaros data (AV_PIX_FMT_RGB24)
        inputFrame = av_frame_alloc();
        if (!inputFrame) {
                Notify(msg_fatal_error, "Could not allocate video frame (RGB)\n");
                return;
        }
        inputFrame->format = AV_PIX_FMT_RGB24;
        inputFrame->width  = c->width;
        inputFrame->height = c->height;
        ret = av_image_alloc(inputFrame->data, inputFrame->linesize, c->width, c->height, AV_PIX_FMT_RGB24, 32);
        if (ret<0) {
                Notify(msg_fatal_error, "Could not allocate video frame data\n");
                return;
        }

        // Fill the frame with ikaros data
        i = 0;
        for(int y=0; y<c->height; y++)
                for(int x=0; x<c->width; x++)
                {
                        int y1 = y*size_x;
                        int xy = x + y1;
                        if (color)
                        {
                                inputFrame->data[0][i++] = red[xy]*255;
                                inputFrame->data[0][i++] = green[xy]*255;
                                inputFrame->data[0][i++] = blue[xy]*255;
                        }
                        else
                        {
                                inputFrame->data[0][i++] = intensity[xy]*255;
                                inputFrame->data[0][i++] = intensity[xy]*255;
                                inputFrame->data[0][i++] = intensity[xy]*255;
                        }
                }

        // Convert AV_PIX_FMT_RGB24 to AV_PIX_FMT_YUV420P
        img_convert_ctx = sws_getCachedContext(img_convert_ctx,c->width, c->height,
                                               AV_PIX_FMT_RGB24,
                                               c->width, c->height, AV_PIX_FMT_YUV420P,
                                               SWS_BILINEAR, NULL, NULL, NULL);
        sws_scale(img_convert_ctx, inputFrame->data, inputFrame->linesize, 0, c->height, outputFrame->data, outputFrame->linesize);


        // Timestamp
        outputFrame->pts = (int)GetTick()*90000*1/float(frameRate);

        // Send ikaros data to encoder
        av_init_packet(&pkt);
        pkt.data = NULL; // packet data will be allocated by the encoder
        pkt.size = 0;
        fflush(stdout);

        // Encode
        encode(c, &pkt, &got_output, outputFrame);

        if (got_output)
        {
                av_interleaved_write_frame(output_format_context, &pkt);
                av_packet_unref(&pkt);
        }

        // Free frames
        av_freep(&outputFrame->data[0]);
        av_frame_free(&outputFrame);
        av_freep(&inputFrame->data[0]);
        av_frame_free(&inputFrame);
}

OutputVideoFile::~OutputVideoFile()
{
        // Get delayed frames
        for (got_output = 1; got_output; i++) {
                fflush(stdout);

                ret = encode(c, &pkt, &got_output, NULL);
                if (ret != 0 and ret != AVERROR_EOF) {
                        Notify(msg_fatal_error, "Error encodeing frame\n");
                        return;
                }
                if (got_output) {
                        av_interleaved_write_frame(output_format_context, &pkt);
                        av_packet_unref(&pkt);
                }
        }
        av_write_trailer(output_format_context);
        avio_closep(&output_format_context->pb);
        avformat_free_context(output_format_context);
}
static InitClass init("OutputVideoFile", &OutputVideoFile::Create, "Source/Modules/IOModules/FileOutput/OutputVideoFile/");
