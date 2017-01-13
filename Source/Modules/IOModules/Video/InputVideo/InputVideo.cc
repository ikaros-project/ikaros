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

#include "InputVideo.h"
#include <stdio.h>
#include <string>

using namespace ikaros;

//Show Dshow Device
void show_dshow_device(){
    AVFormatContext *input_format_context = avformat_alloc_context();
    AVDictionary* options = NULL;
    av_dict_set(&options,"list_devices","true",0);
    AVInputFormat *iformat = av_find_input_format("dshow");
    printf("========Device Info=============\n");
    avformat_open_input(&input_format_context,"video=dummy",iformat,&options);
    printf("================================\n");
}

//Show Dshow Device Option
void show_dshow_device_option(){
    AVFormatContext *input_format_context = avformat_alloc_context();
    AVDictionary* options = NULL;
    av_dict_set(&options,"list_options","true",0);
    AVInputFormat *iformat = av_find_input_format("dshow");
    printf("========Device Option Info======\n");
    avformat_open_input(&input_format_context,"video=Integrated Camera",iformat,&options);
    printf("================================\n");
}

//Show VFW Device
void show_vfw_device(){
    AVFormatContext *input_format_context = avformat_alloc_context();
    AVInputFormat *iformat = av_find_input_format("vfwcap");
    printf("========VFW Device Info======\n");
    avformat_open_input(&input_format_context,"list",iformat,NULL);
    printf("=============================\n");
}

//Show AVFoundation Device
void show_avfoundation_device(){
    AVFormatContext *input_format_context = avformat_alloc_context();
    AVDictionary* options = NULL;
    av_dict_set(&options,"list_devices","true",0);
    AVInputFormat *iformat = av_find_input_format("avfoundation");
    printf("==AVFoundation Device Info===\n");
    avformat_open_input(&input_format_context,"",iformat,&options);
    printf("=============================\n");
}


InputVideo::InputVideo(Parameter * p):
Module(p)
{
    
    frameRate   = GetFloatValue("frame_rate");
    id          = GetIntValue("id");
    
    size_x      = GetIntValue("size_x");
    size_y      = GetIntValue("size_y");
    
    listDevices = GetBoolValue("list_devices");
    
    av_register_all();              // Register all formats and codecs
    input_format_context = avformat_alloc_context();
    avdevice_register_all();        // libavdevice
    
    
#ifdef _WIN32
    if (listDevices)
    {
        listDevices
        //Show Dshow Device
        show_dshow_device();
        //Show Device Options
        show_dshow_device_option();
        //Show VFW Options
        show_vfw_device();
    }
#if USE_DSHOW
    AVInputFormat *ifmt=av_find_input_format("dshow");
    //Set own video device's name
    if(avformat_open_input(&input_format_context,"video=Integrated Camera",ifmt,NULL)!=0){
		Notify(msg_fatal_error,"Couldn't open input stream.\n");
        return;
    }
#else
    AVInputFormat *ifmt=av_find_input_format("vfwcap");
    if(avformat_open_input(&input_format_context,"0",ifmt,NULL)!=0){
		Notify(msg_fatal_error,"Couldn't open input stream.\n");
        return;
    }
#endif
#elif defined LINUX
    AVInputFormat *ifmt=av_find_input_format("video4linux2");
    if(avformat_open_input(&input_format_context,"/dev/video0",ifmt,NULL)!=0){
		Notify(msg_fatal_error,"Couldn't open input stream.\n");
        return;
    }
#else
    
    if (listDevices)
        show_avfoundation_device();
    
    AVInputFormat *ifmt=av_find_input_format("avfoundation");
    AVDictionary* options = NULL;
    
    // Setting options
    std::string sizeString = std::to_string(size_x)+std::string("x")+std::to_string(size_y);
    std::string frameRateString = std::to_string(frameRate);
    std::string idString = std::to_string(id);
    av_dict_set(&options,"video_size",sizeString.c_str(),0);
    av_dict_set(&options,"pixel_format","1",0); // AV_PIX_FMT_YUYV422
    av_dict_set(&options, "framerate", frameRateString.c_str(), 0);
    
    if(avformat_open_input(&input_format_context,idString.c_str(),ifmt,&options)!=0){
		Notify(msg_fatal_error,"Couldn't open input stream.\n");
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
    for(int i=0; i<input_format_context->nb_streams; i++)
        if(input_format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamId=i;
            break;
        }
    if(videoStreamId==-1)
    {
        Notify(msg_fatal_error, "Didn't find a video stream\n");
        return;
    }
    
    // Decoding video
    
    /// Find the decoder for the video stream
    input_codec = avcodec_find_decoder(input_format_context->streams[videoStreamId]->codecpar->codec_id);
    if(input_codec==NULL) {
		Notify(msg_fatal_error,"Unsupported codec!\n");
        return; // Codec not found
    }
    
    // Creating decoding context
    avctx = avcodec_alloc_context3(input_codec);
    if (!avctx) {
		Notify(msg_fatal_error,"Could not allocate a decoding context\n");
        avformat_close_input(&input_format_context);
        return;
    }
    
    // Setting parmeters to context?
    int error = avcodec_parameters_to_context(avctx, (*input_format_context).streams[videoStreamId]->codecpar);
    if (error < 0) {
        avformat_close_input(&input_format_context);
        avcodec_free_context(&avctx);
        return;
    }
    
    /// Open codec
    if( avcodec_open2(avctx, input_codec, NULL) < 0 )
    {
        Notify(msg_fatal_error, "Could not open codec\n");
        return;
    }
    
    inputFrame = av_frame_alloc();    // Input
    if(inputFrame == NULL)
    {
        Notify(msg_fatal_error, "Could not allocate AVFrame\n");
        return;
    }
    
    outputFrame = av_frame_alloc();   // Output (after resize and convertions)
    outputFrame->format = AV_PIX_FMT_RGB24;
    outputFrame->width  = size_x;
    outputFrame->height = size_y;
    av_image_alloc(outputFrame->data, outputFrame->linesize, size_x, size_y, AV_PIX_FMT_RGB24, 32);
    
    if(outputFrame==NULL)
    {
        Notify(msg_fatal_error, "Could not allocate AVFrame\n");
        return;
    }
    AddOutput("INTENSITY", size_x, size_y);
    AddOutput("RED", size_x, size_y);
    AddOutput("GREEN", size_x, size_y);
    AddOutput("BLUE", size_x, size_y);
}
void
InputVideo::Init()
{
    intensity	= GetOutputArray("INTENSITY");
    red			= GetOutputArray("RED");
    green		= GetOutputArray("GREEN");
    blue		= GetOutputArray("BLUE");
}

int
InputVideo::decode(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt)
{
    int ret;
    *got_frame = 0;
    if (pkt) {
        ret = avcodec_send_packet(avctx, pkt);
        if (ret < 0)
            return ret == AVERROR_EOF ? 0 : ret;
    }
    ret = avcodec_receive_frame(avctx, frame);
    if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
        return ret;
    if (ret >= 0)
        *got_frame = 1;
    return 0;
}

void
InputVideo::Tick()
{
    const float c13 = 1.0/3.0;
    const float c1255 = 1.0/255.0;
    
    int gotFrame = 0;
    while (!gotFrame)                                           // Keep reading from source until we get a video packet
    {
        int ret = 0;
        while (ret<0)
        {
            ret = av_read_frame(input_format_context, &packet);
            av_packet_unref(&packet);
        }
        
        if(av_read_frame(input_format_context, &packet)>=0)     // Read packet from source. Waiting until data is there!
        {
            
            if(packet.stream_index==videoStreamId)              // Is this a packet from the video stream?
                decode(avctx, inputFrame, &gotFrame, &packet);
        }
        else
        {
            decode(avctx, inputFrame, &gotFrame, NULL);         // No more input but decoder may have more frames
        }
        
        if (gotFrame)                                           // Decode gave us a frame
        {
            // Convert the frame to AV_PIX_FMT_RGB24 format
            static struct SwsContext *img_convert_ctx;
            img_convert_ctx = sws_getCachedContext(img_convert_ctx,avctx->width, avctx->height,
                                                   avctx->pix_fmt,
                                                   size_x, size_y, AV_PIX_FMT_RGB24,
                                                   SWS_BICUBIC, NULL, NULL, NULL);
            // Scale the image
            sws_scale(img_convert_ctx, (const uint8_t * const *)inputFrame->data,
                      inputFrame->linesize, 0, avctx->height,
                      outputFrame->data, outputFrame->linesize);
            
            // Put data in ikaros output
            unsigned char * data = outputFrame->data[0];
            for(int y=0; y<size_y; y++)
                for(int x=0; x<size_x; x++)
                {
                    int yLineSize = y*outputFrame->linesize[0];
                    int y1 = y*size_x;
                    int xy = x + y1;
                    int x3  = x*3;
                    intensity[xy] 	=   red[xy]       = c1255*data[yLineSize+x3+0];
                    intensity[xy] 	+=  green[xy]     = c1255*data[yLineSize+x3+1];
                    intensity[xy] 	+=  blue[xy]      = c1255*data[yLineSize+x3+2];
                    intensity[xy]   *=  c13;
                }
            av_packet_unref(&packet);
        }
    }
}

InputVideo::~InputVideo()
{
    //av_freep(&inputFrame->data[0]); // Freed in decoder function
    av_free(inputFrame);
	if (outputFrame)
		av_freep(&outputFrame->data[0]);
    av_free(outputFrame);
    avcodec_close(avctx);
    avformat_close_input(&input_format_context);
}
static InitClass init("InputVideo", &InputVideo::Create, "Source/Modules/IOModules/Video/InputVideo/");
