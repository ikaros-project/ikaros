//
//	  InputVideoFile.cc		This file is a part of the IKAROS project
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

//#define MY_PARALLEL_CODE
using namespace ikaros;


#ifdef MY_PARALLEL_CODE
#include <thread>

using namespace std;

#endif


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
    if (printInfo)
        av_dump_format(pFormatCtx, 0, filename, 0);
    
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
    pFrame = av_frame_alloc();
    
    /// Allocate an AVFrame structure
    pFrameRGB = av_frame_alloc();
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
    numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, size_x, size_y);
    buffer = (uint8_t *) av_malloc(numBytes*sizeof(uint8_t));
    
    /// Assign appropriate parts of buffer to image planes in pFrameRGB
    avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_RGB24,
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
                    
                    img_convert_ctx = sws_getCachedContext(img_convert_ctx,pCodecCtx->width, pCodecCtx->height,
                                                           pCodecCtx->pix_fmt,
                                                           size_x, size_y, AV_PIX_FMT_RGB24,
                                                           SWS_BICUBIC, NULL, NULL, NULL);
                    
                    
                    sws_scale(img_convert_ctx, (const uint8_t * const *)pFrame->data,
                              pFrame->linesize, 0, pCodecCtx->height,
                              pFrameRGB->data, pFrameRGB->linesize);
                    
                    unsigned char * data = pFrameRGB->data[0];
                    
#ifdef MY_PARALLEL_CODE
                    //printf("Parallel Code\n");

                    // Parallel  Code
                    //
                    // iMac 8 threads
//                IKAROS: Reading XML file "InputVideoFile_test.ikc".
//                IKAROS: Start
//                IKAROS: Stop (2000 ticks, 13.90 s, 143.90 ticks/s, 0.007 s/tick)
//                IKAROS:
//                IKAROS: Time (ms):
//                IKAROS: -------------------
//                IKAROS: Modules:   13743.55
//                IKAROS: Other:       154.69
//                IKAROS: -------------------
//                IKAROS: Total:     13898.24
//                IKAROS:
//                IKAROS: Time in Each Module:
//                IKAROS:
//                IKAROS: Module              Class                    Count  Avg (ms)    Time %
//                IKAROS: ----------------------------------------------------------------------
//                IKAROS: InputVideoFile      InputVideoFile            2000      6.87     100.0
//                IKAROS: ----------------------------------------------------------------------
//                IKAROS: Note: Time is real-time, not time in thread.
//                IKAROS: 

                    
                    const int nrOfCores = 8; //thread::hardware_concurrency();
                    // Create some threads pointers
                    thread t[nrOfCores];
                    //printf("Number of threads %i\n",thread::hardware_concurrency());
                    int span = size_y/nrOfCores;
                    
                    for(int j=0; j<nrOfCores; j++)
                    {
                        t[j] = thread([&,j]()
                                      {
                                          //printf("I am thread %i and I will take care of %i to %i\n",j, j*span,j*span+span);
                                          
                                          for(int y=j*span; y<j*span+span; y++)
                                          {
                                              for(int x=0; x<size_x; x++)
                                              {
                                                  int yLineSize = y*pFrameRGB->linesize[0];
                                                  int y1 = y*size_x;
                                                  int xy = x + y1;
                                                  int x3  = x*3;
                                                  intensity[xy] 	=   red[xy]       = c1255*data[yLineSize+x3+0];
                                                  intensity[xy] 	+=  green[xy]     = c1255*data[yLineSize+x3+1];
                                                  intensity[xy] 	+=  blue[xy]      = c1255*data[yLineSize+x3+2];
                                                  intensity[xy]     *=  c13;
                                                  
                                              }
                                          }
                                      });
                        
                    }
                    
                    for(int j=0; j<nrOfCores; j++)
                    {
                        t[j].join();
                    }
#else
                    
                    //printf("Serial Code\n");
                    // Serial Code
                    //
                    // iMac 1 thread
                    
//                IKAROS: Reading XML file "InputVideoFile_test.ikc".
//                IKAROS: Start
//                IKAROS: Stop (2000 ticks, 26.42 s, 75.70 ticks/s, 0.013 s/tick)
//                IKAROS:
//                IKAROS: Time (ms):
//                IKAROS: -------------------
//                IKAROS: Modules:   26202.73
//                IKAROS: Other:       215.61
//                IKAROS: -------------------
//                IKAROS: Total:     26418.34
//                IKAROS:
//                IKAROS: Time in Each Module:
//                IKAROS:
//                IKAROS: Module              Class                    Count  Avg (ms)    Time %
//                IKAROS: ----------------------------------------------------------------------
//                IKAROS: InputVideoFile      InputVideoFile            2000     13.10     100.0
//                IKAROS: ----------------------------------------------------------------------
//                IKAROS: Note: Time is real-time, not time in thread.
                    
                    
                    
                    // Write pixel data
                    for(int y=0; y<size_y; y++)
                        for(int x=0; x<size_x; x++)
                        {
                            int yLineSize = y*pFrameRGB->linesize[0];
                            int y1 = y*size_x;
                            int xy = x + y1;
                            int x3  = x*3;
                            intensity[xy] 	=   red[xy]       = c1255*data[yLineSize+x3+0];
                            intensity[xy] 	+=  green[xy]     = c1255*data[yLineSize+x3+1];
                            intensity[xy] 	+=  blue[xy]      = c1255*data[yLineSize+x3+2];
                            intensity[xy]   *=  c13;
                        }
#endif
                    
                }
                
            }
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


