//
//	InputVideoFile.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2012 Birger Johansson
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

// Based on FFMpeg tutorial http://dranger.com/ffmpeg/


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
    
    // Register all formats and codecs
    av_register_all();
    
    /// Open video file
    if(avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0)
    {
        Notify(msg_fatal_error, "Could not open file.\n");
        return;
    }
    
    /// Retrieve stream information
    if(avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        Notify(msg_fatal_error, "Couldn't find stream information\n");
        return;
    }
    
    /// Dump information about file onto standard error
    //av_dump_format(pFormatCtx, 0, filename, 0);
    
    /// Find the first video stream
    videoStreamIdx=-1;
    for(int i=0; i<pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) { //CODEC_TYPE_VIDEO
            videoStreamIdx=i;
            break;
        }
    if(videoStreamIdx==-1)
    {
        Notify(msg_fatal_error, "Didn't find a video stream\n");
        return;
    }
    
    /// Get a pointer to the codec context for the video stream
    pCodecCtx = pFormatCtx->streams[videoStreamIdx]->codec;
    
    /// Find the decoder for the video stream
    pCodec = avcodec_find_decoder( pCodecCtx->codec_id);
    if(pCodec==NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        return; // Codec not found
    }
    /// Open codec
    if( avcodec_open2(pCodecCtx, pCodec, NULL) < 0 )
    {
        Notify(msg_fatal_error, "Could not open codec\n");
        return;
    }
    
    /// Allocate video frame
    pFrame = avcodec_alloc_frame();
    
    /// Allocate an AVFrame structure
    pFrameRGB = avcodec_alloc_frame();
    if(pFrameRGB==NULL)
    {
        Notify(msg_fatal_error, "Could not allocate AVFrame\n");
        return;
    }
    
    
    // Check native size
    
    size_x = GetIntValue("size_x");
    size_y = GetIntValue("size_y");
    
    native_size_x = int(pCodecCtx->width);
    native_size_y = int(pCodecCtx->height);
    
    if(size_x == 0 || size_y == 0)
    {
        size_x = native_size_x;
        size_y = native_size_y;
    }
    
    /// Determine required buffer size and allocate buffer
    numBytes = avpicture_get_size(PIX_FMT_RGB24, size_x, size_y);
    
    buffer = (uint8_t *) av_malloc(numBytes*sizeof(uint8_t));
    
    /// Assign appropriate parts of buffer to image planes in pFrameRGB
    avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
                   size_x, size_y);
    
    
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

void
InputVideoFile::Tick()
{
    const float c13 = 1.0/3.0;
    const float c1255 = 1.0/255.0;
    restart[0] = (restart[0] == -1 ? 1 : 0);
    frameFinished = false;
    
    //omp_set_num_threads(4);

    
    while (!frameFinished)
    {
        if(av_read_frame(pFormatCtx, &packet)>=0)
        {
            // Is this a packet from the video stream?
            if(packet.stream_index==videoStreamIdx)
            {
                
                /// Decode video frame
                avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
                
                // Did we get a video frame?
                if(frameFinished) {
                    
                    static struct SwsContext *img_convert_ctx;
                    
                    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                                     size_x, size_y, PIX_FMT_RGB24,
                                                     SWS_BICUBIC, NULL, NULL, NULL);
                    
                    sws_scale(img_convert_ctx, (const uint8_t * const *)pFrame->data,
                              pFrame->linesize, 0, pCodecCtx->height,
                              pFrameRGB->data, pFrameRGB->linesize);
                    
                    unsigned char * r = pFrameRGB->data[0];
                    
                    // Write pixel data
                    for(int y=0; y<size_y; y++)

                        for(int x=0; x<size_x; x++)
                        {
                            //red[x + y*size_x] = float(r[y*pFrameRGB->linesize[0]+x*3]/255.0f);
                            //green[x + y*size_x] = float(r[y*pFrameRGB->linesize[0]+x*3+1]/255.0f);
                            //blue[x + y*size_x] = float(r[y*pFrameRGB->linesize[0]+x*3+2]/255.0f);
                            
                            int yLineSize = y*pFrameRGB->linesize[0];
                            int y1 = y*size_x;
                            int xy = x + y1;
                            int x3  = x*3;
                            red[xy]       = c1255*r[yLineSize+x3+0];
                            green[xy]     = c1255*r[yLineSize+x3+1];
                            blue[xy]      = c1255*r[yLineSize+x3+2];
                            intensity[xy]*=c13;
                        }
                }
                
            }
            //multiply(intensity, c13, size_x*size_y);
            // Free the packet that was allocated by av_read_frame
            av_free_packet(&packet);
        }
        
        else
        {
            if (loop)
            {
                restart[0] = 1;
                
                if(av_seek_frame(pFormatCtx, 0, 0, AVSEEK_FLAG_ANY) < 0)
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
    // Free the RGB image
    av_free(buffer);
    av_free(pFrameRGB);
    
    // Free the YUV frame
    av_free(pFrame);
    
    // Close the codec
    avcodec_close(pCodecCtx);
    
    // Close the video file
    avformat_close_input(&pFormatCtx);
}
static InitClass init("InputVideoFile", &InputVideoFile::Create, "Source/Modules/IOModules/FileInput/InputVideoFile/");


