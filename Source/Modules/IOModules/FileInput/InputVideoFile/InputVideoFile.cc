//
//	  InputVideoFile.cc		This file is a part of the IKAROS project
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

#include "InputVideoFile.h"

using namespace ikaros;

InputVideoFile::InputVideoFile(Parameter * p):
Module(p)
{
    
    filename = GetValue("filename");
    if (filename == NULL)
    {
        Notify(msg_fatal_error, "No input file parameter supplied.\n");
        return;
    }
    
    loop = GetBoolValue("loop", false);
    printInfo = GetBoolValue("info", false);
    
    // Register all formats and codecs
    av_register_all();
    
    /// Open video file
    if(avformat_open_input(&input_format_context, filename, NULL, NULL) != 0) // Allocating input_format_context
    {
        Notify(msg_fatal_error, "Could not open file.\n");
        return;
    }
    
    /// Retrieve stream information
    if(avformat_find_stream_info(input_format_context, NULL) < 0)
    {
        Notify(msg_fatal_error, "Couldn't find stream information\n");
        return;
    }
    
    /// Dump information about file onto standard error
    if (printInfo)
        av_dump_format(input_format_context, 0, filename, 0);
    
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
        fprintf(stderr, "Unsupported codec!\n");
        return; // Codec not found
    }
    
    // Creating decoding context
    avctx = avcodec_alloc_context3(input_codec);
    if (!avctx) {
        fprintf(stderr, "Could not allocate a decoding context\n");
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
    outputFrame = av_frame_alloc();   // Output (after resize and convertions)
    
    if(inputFrame == NULL || outputFrame==NULL)
    {
        Notify(msg_fatal_error, "Could not allocate AVFrame\n");
        return;
    }
    
    // Is the output size set in ikc file? If not use native size
    size_x = GetIntValue("size_x");
    size_y = GetIntValue("size_y");
    
    native_size_x = int(avctx->width);
    native_size_y = int(avctx->height);
    
    if(size_x == 0 || size_y == 0)
    {
        size_x = native_size_x;
        size_y = native_size_y;
    }
    
    /// Determine required buffer size and allocate buffer
    numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, size_x, size_y,1);
    buffer = (uint8_t *) av_malloc(numBytes*sizeof(uint8_t));
    
    /// Assign appropriate parts of buffer to image planes in outputFrame
    av_image_fill_arrays(outputFrame->data,outputFrame->linesize, buffer, AV_PIX_FMT_RGB24,size_x, size_y,1);
    
    AddOutput("INTENSITY", size_x, size_y);
    AddOutput("RED", size_x, size_y);
    AddOutput("GREEN", size_x, size_y);
    AddOutput("BLUE", size_x, size_y);
    
    AddOutput("RESTART", 1);
}
void
InputVideoFile::Init()
{
    
    intensity	= GetOutputArray("INTENSITY");
    red			= GetOutputArray("RED");
    green		= GetOutputArray("GREEN");
    blue		= GetOutputArray("BLUE");
    
    restart     = GetOutputArray("RESTART");
    restart[0]  = -1; // indicates first tick
}


// The flush packet is a non-NULL packet with size 0 and data NULL
int
InputVideoFile::decode(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt)
{
    int ret;
    *got_frame = 0;
    if (pkt) {
        ret = avcodec_send_packet(avctx, pkt);
        // In particular, we don't expect AVERROR(EAGAIN), because we read all
        // decoded frames with avcodec_receive_frame() until done.
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
InputVideoFile::Tick()
{
    const float c13 = 1.0/3.0;
    const float c1255 = 1.0/255.0;
    restart[0] = (restart[0] == -1 ? 1 : 0);
    
    int gotFrame = 0;
    while (!gotFrame)                                           // Keep reading from source until we get a video packet
    {
        if(av_read_frame(input_format_context, &packet)>=0)     // Read packet from source
        {
            if(packet.stream_index==videoStreamId)              // Is this a packet from the video stream?
            {
                decode(avctx, inputFrame, &gotFrame, &packet);
                if (gotFrame)                                   // Decode gave us a frame
                {
                    // Convert teh frame to AV_PIX_FMT_RGB24 format
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
                }
            }
        }
        else
        {
            if (loop)
            {
                restart[0] = 1;
                avcodec_flush_buffers(avctx);                                       // Need to be done before rewinding
                if(av_seek_frame(input_format_context, 0, 0, AVSEEK_FLAG_ANY) < 0)  // Rewind movie
                    printf("error while seeking\n");
            }
            else
            {
                Notify(msg_end_of_file, "End of movie");
                return;
            }
        }
    }
}


InputVideoFile::~InputVideoFile()
{
    av_free(buffer);
    av_free(outputFrame);
    av_free(inputFrame);
    avcodec_close(avctx);
    avformat_close_input(&input_format_context);
}
static InitClass init("InputVideoFile", &InputVideoFile::Create, "Source/Modules/IOModules/FileInput/InputVideoFile/");