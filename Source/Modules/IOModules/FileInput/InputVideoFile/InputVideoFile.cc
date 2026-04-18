//
//    InputVideoFile.cc        This file is a part of the IKAROS project
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

#include "ikaros.h"

#include <cstring>
#include <string>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

using namespace ikaros;

namespace
{
std::string
ffmpeg_error_string(int error_code)
{
    char error_buffer[AV_ERROR_MAX_STRING_SIZE] = {};
    av_make_error_string(error_buffer, sizeof(error_buffer), error_code);
    return error_buffer;
}

int
decode_packet(AVCodecContext * codec_context, AVFrame * frame, int * got_frame, AVPacket * packet)
{
    *got_frame = 0;

    if(packet != nullptr)
    {
        const int send_result = avcodec_send_packet(codec_context, packet);
        if(send_result < 0)
            return send_result == AVERROR_EOF ? 0 : send_result;
    }

    const int receive_result = avcodec_receive_frame(codec_context, frame);
    if(receive_result < 0 && receive_result != AVERROR(EAGAIN) && receive_result != AVERROR_EOF)
        return receive_result;

    if(receive_result >= 0)
        *got_frame = 1;

    return 0;
}

bool
probe_video_size(const std::string & filename, int & width, int & height, std::string & error_message)
{
    AVFormatContext * format_context = nullptr;
    width = 0;
    height = 0;

    const int open_result = avformat_open_input(&format_context, filename.c_str(), nullptr, nullptr);
    if(open_result != 0)
    {
        error_message = "Could not open file \"" + filename + "\": " + ffmpeg_error_string(open_result);
        return false;
    }

    auto cleanup = [&]()
    {
        if(format_context != nullptr)
            avformat_close_input(&format_context);
    };

    const int stream_info_result = avformat_find_stream_info(format_context, nullptr);
    if(stream_info_result < 0)
    {
        error_message = "Could not read stream information: " + ffmpeg_error_string(stream_info_result);
        cleanup();
        return false;
    }

    for(unsigned int i = 0; i < format_context->nb_streams; ++i)
    {
        if(format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            width = format_context->streams[i]->codecpar->width;
            height = format_context->streams[i]->codecpar->height;
            cleanup();
            return width > 0 && height > 0;
        }
    }

    error_message = "InputVideoFile did not find a video stream.";
    cleanup();
    return false;
}
}

class InputVideoFile : public Module
{
public:
    ~InputVideoFile() override
    {
        if(image_convert_context != nullptr)
            sws_freeContext(image_convert_context);

        if(output_frame != nullptr)
        {
            av_freep(&output_frame->data[0]);
            av_frame_free(&output_frame);
        }

        if(input_frame != nullptr)
            av_frame_free(&input_frame);

        if(codec_context != nullptr)
            avcodec_free_context(&codec_context);

        if(input_format_context != nullptr)
            avformat_close_input(&input_format_context);
    }

private:
    parameter loop;
    parameter printInfo;
    parameter size_x;
    parameter size_y;
    parameter filename;

    matrix intensity;
    matrix red;
    matrix green;
    matrix blue;
    matrix output;
    matrix restart;

    AVFormatContext * input_format_context = nullptr;
    AVCodecContext * codec_context = nullptr;
    AVFrame * input_frame = nullptr;
    AVFrame * output_frame = nullptr;
    SwsContext * image_convert_context = nullptr;

    int video_stream_id = -1;
    int output_width = 0;
    int output_height = 0;
    bool restart_next_frame = true;

    void
    SetParameters() override
    {
        Bind(filename, "filename");

        const std::string file_to_open = GetValue("filename");
        const int requested_width = GetIntValue("size_x", 0);
        const int requested_height = GetIntValue("size_y", 0);

        if(file_to_open.empty())
            return;

        if(requested_width > 0 && requested_height > 0)
            return;

        int detected_width = 0;
        int detected_height = 0;
        std::string error_message;
        if(!probe_video_size(file_to_open, detected_width, detected_height, error_message))
        {
            Notify(msg_fatal_error, error_message.empty() ? "InputVideoFile could not determine the movie size." : error_message);
            return;
        }

        if(requested_width == 0)
            SetParameter("size_x", std::to_string(detected_width));

        if(requested_height == 0)
            SetParameter("size_y", std::to_string(detected_height));
    }

    bool
    OpenInput()
    {
        const std::string file_to_open = filename.as_string();
        if(file_to_open.empty())
        {
            Notify(msg_fatal_error, "InputVideoFile requires a non-empty filename.");
            return false;
        }

        const int open_result = avformat_open_input(&input_format_context, file_to_open.c_str(), nullptr, nullptr);
        if(open_result != 0)
        {
            Notify(msg_fatal_error, "Could not open file \"" + file_to_open + "\": " + ffmpeg_error_string(open_result));
            return false;
        }

        const int stream_info_result = avformat_find_stream_info(input_format_context, nullptr);
        if(stream_info_result < 0)
        {
            Notify(msg_fatal_error, "Could not read stream information: " + ffmpeg_error_string(stream_info_result));
            return false;
        }

        if(printInfo)
            av_dump_format(input_format_context, 0, file_to_open.c_str(), 0);

        av_log_set_level(AV_LOG_FATAL);

        for(unsigned int i = 0; i < input_format_context->nb_streams; ++i)
        {
            if(input_format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                video_stream_id = static_cast<int>(i);
                break;
            }
        }

        if(video_stream_id == -1)
        {
            Notify(msg_fatal_error, "InputVideoFile did not find a video stream.");
            return false;
        }

        const AVCodec * input_codec = avcodec_find_decoder(input_format_context->streams[video_stream_id]->codecpar->codec_id);
        if(input_codec == nullptr)
        {
            Notify(msg_fatal_error, "InputVideoFile found an unsupported codec.");
            return false;
        }

        codec_context = avcodec_alloc_context3(input_codec);
        if(codec_context == nullptr)
        {
            Notify(msg_fatal_error, "Could not allocate a decoding context.");
            return false;
        }

        const int context_result = avcodec_parameters_to_context(
            codec_context,
            input_format_context->streams[video_stream_id]->codecpar);
        if(context_result < 0)
        {
            Notify(msg_fatal_error, "Could not copy codec parameters: " + ffmpeg_error_string(context_result));
            return false;
        }

        const int codec_open_result = avcodec_open2(codec_context, input_codec, nullptr);
        if(codec_open_result < 0)
        {
            Notify(msg_fatal_error, "Could not open codec: " + ffmpeg_error_string(codec_open_result));
            return false;
        }

        input_frame = av_frame_alloc();
        if(input_frame == nullptr)
        {
            Notify(msg_fatal_error, "Could not allocate input frame.");
            return false;
        }

        output_frame = av_frame_alloc();
        if(output_frame == nullptr)
        {
            Notify(msg_fatal_error, "Could not allocate output frame.");
            return false;
        }

        return true;
    }

    bool
    ConfigureOutput()
    {
        output_width = size_x.as_int();
        output_height = size_y.as_int();

        if(output_width <= 0 || output_height <= 0)
        {
            Notify(msg_fatal_error, "InputVideoFile requires positive size_x and size_y.");
            return false;
        }

        output_frame->format = AV_PIX_FMT_RGB24;
        output_frame->width = output_width;
        output_frame->height = output_height;

        const int image_result = av_image_alloc(
            output_frame->data,
            output_frame->linesize,
            output_width,
            output_height,
            AV_PIX_FMT_RGB24,
            32);
        if(image_result < 0)
        {
            Notify(msg_fatal_error, "Could not allocate output image: " + ffmpeg_error_string(image_result));
            return false;
        }

        return true;
    }

    bool
    DecodeNextFrame()
    {
        AVPacket packet{};
        int got_frame = 0;

        while(true)
        {
            const int read_result = av_read_frame(input_format_context, &packet);
            if(read_result >= 0)
            {
                if(packet.stream_index == video_stream_id)
                {
                    const int decode_result = decode_packet(codec_context, input_frame, &got_frame, &packet);
                    av_packet_unref(&packet);

                    if(decode_result < 0)
                    {
                        Notify(msg_warning, "InputVideoFile decode error: " + ffmpeg_error_string(decode_result));
                        return false;
                    }

                    if(got_frame)
                        return true;
                }
                else
                {
                    av_packet_unref(&packet);
                }

                continue;
            }

            const int flush_result = decode_packet(codec_context, input_frame, &got_frame, nullptr);
            if(flush_result < 0)
            {
                Notify(msg_warning, "InputVideoFile flush error: " + ffmpeg_error_string(flush_result));
                return false;
            }

            if(got_frame)
                return true;

            if(!loop)
                return false;

            restart_next_frame = true;
            avcodec_flush_buffers(codec_context);
            if(av_seek_frame(input_format_context, video_stream_id, 0, AVSEEK_FLAG_BACKWARD) < 0)
            {
                Notify(msg_warning, "InputVideoFile could not seek back to the start of the movie.");
                return false;
            }
        }
    }

    bool
    ConvertFrame()
    {
        image_convert_context = sws_getCachedContext(
            image_convert_context,
            codec_context->width,
            codec_context->height,
            codec_context->pix_fmt,
            output_width,
            output_height,
            AV_PIX_FMT_RGB24,
            SWS_BICUBIC,
            nullptr,
            nullptr,
            nullptr);

        if(image_convert_context == nullptr)
        {
            Notify(msg_warning, "InputVideoFile could not create a conversion context.");
            return false;
        }

        const int scaled_height = sws_scale(
            image_convert_context,
            reinterpret_cast<const uint8_t * const *>(input_frame->data),
            input_frame->linesize,
            0,
            codec_context->height,
            output_frame->data,
            output_frame->linesize);

        if(scaled_height <= 0)
        {
            Notify(msg_warning, "InputVideoFile could not convert the current frame.");
            return false;
        }

        return true;
    }

    void
    CopyFrameToOutputs()
    {
        const float byte_to_float_scale = 1.0f / 255.0f;
        const float one_third = 1.0f / 3.0f;
        const uint8_t * data = output_frame->data[0];

        for(int row = 0; row < output_height; ++row)
        {
            const uint8_t * row_data = data + row * output_frame->linesize[0];
            for(int col = 0; col < output_width; ++col)
            {
                const int x3 = col * 3;
                const float red_value = static_cast<float>(row_data[x3]) * byte_to_float_scale;
                const float green_value = static_cast<float>(row_data[x3 + 1]) * byte_to_float_scale;
                const float blue_value = static_cast<float>(row_data[x3 + 2]) * byte_to_float_scale;

                red(row, col) = red_value;
                green(row, col) = green_value;
                blue(row, col) = blue_value;
                intensity(row, col) = (red_value + green_value + blue_value) * one_third;
            }
        }
    }

    void
    Init() override
    {
        Bind(loop, "loop");
        Bind(printInfo, "info");
        Bind(size_x, "size_x");
        Bind(size_y, "size_y");
        Bind(filename, "filename");

        Bind(intensity, "INTENSITY");
        Bind(red, "RED");
        Bind(green, "GREEN");
        Bind(blue, "BLUE");
        Bind(output, "OUTPUT");
        Bind(restart, "RESTART");

        if(!OpenInput())
            return;

        if(!ConfigureOutput())
            return;

        restart[0] = 1.0f;
    }

    void
    Tick() override
    {
        restart[0] = restart_next_frame ? 1.0f : 0.0f;
        restart_next_frame = false;

        if(input_format_context == nullptr || codec_context == nullptr)
            return;

        if(!DecodeNextFrame())
            return;

        if(!ConvertFrame())
            return;

        CopyFrameToOutputs();
    }
};

INSTALL_CLASS(InputVideoFile)
