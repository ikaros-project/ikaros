//
//	  InputVideo.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2016 Birger Johansson, 2025 Christian Balkenius
//

#define __STDC_CONSTANT_MACROS
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>
}

#include <regex>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>

#include "ikaros.h"

using namespace ikaros;

static std::vector<std::string> 
get_avfoundation_devices() 
{
    std::vector<std::string> devices;
    char buffer[512];
    std::string result;
    std::string cmd = "ffmpeg -f avfoundation -list_devices true -i dummy 2>&1";

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) 
        throw std::runtime_error("popen() failed");

    while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) 
        result += buffer;

    // Parse lines that look like: "[0] FaceTime HD Camera"
    std::istringstream stream(result);
    std::string line;
    std::regex device_regex(R"(\[\d+\]\s+(.+))");
    std::smatch match;

    bool in_video_section = false;

    while (std::getline(stream, line)) 
    {
        if (line.find("AVFoundation video devices") != std::string::npos) 
            in_video_section = true;
        else if (line.find("AVFoundation audio devices") != std::string::npos) 
            in_video_section = false;  // stop parsing after video section
        else if (in_video_section && std::regex_search(line, match, device_regex))
            devices.push_back(match[1]);
    }
    return devices;
}


static int
map_device_name_to_index(const std::vector<std::string> &devices, const std::string &device_name, int device_index)
{
    int ix= 0;
    for (size_t i = 0; i < devices.size(); ++i) 
    {
        if (devices[i] == device_name) 
        {
            if(ix ==device_index)
                return static_cast<int>(i);
            else
                ix++;   
        }
    }
    return -1; // Not found
}   


class InputVideo : public Module
{
    parameter frameRate;
    parameter id;
    parameter size_x;
    parameter size_y;
    parameter listDevices;

    parameter device_name;
    parameter device_index;

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
    AVPacket * packet = av_packet_alloc();
    SwsContext *img_convert_ctx;
    AVDictionary *options = nullptr;

    std::thread capture_thread;
    std::mutex frame_mutex;
    std::condition_variable frame_cv;
    std::atomic<bool> stop_capture{false};
    std::vector<uint8_t> latest_frame_data;
    int latest_frame_linesize = 0;
    bool frame_ready = false;



    void 
    StartCaptureThread()
    {
        stop_capture = false;
        capture_thread = std::thread([this]() {
            int gotFrame = 0;
            while (!stop_capture)
            {
                gotFrame = 0;
                while (av_read_frame(input_format_context, packet) >= 0) // Try to read frames and skip older ones
                {
                    if (packet->stream_index != videoStreamId)
                    {
                        av_packet_unref(packet);
                        continue;
                    }

                    if (packet->size <= 0) // Check packet size
                    {
                        std::cerr << "Warning: received empty or corrupt packet" << std::endl;
                        av_packet_unref(packet);
                        continue;
                    }

                    Decode(avctx, inputFrame, &gotFrame, packet);
                    av_packet_unref(packet);
                    if (gotFrame)
                        break;
                }

                if (!gotFrame)
                    Decode(avctx, inputFrame, &gotFrame, nullptr);

                if (gotFrame)
                {
                    img_convert_ctx = sws_getCachedContext(img_convert_ctx, avctx->width, avctx->height, avctx->pix_fmt,  size_x, size_y, AV_PIX_FMT_RGB24, SWS_BICUBIC, nullptr, nullptr, nullptr);
                    if (!img_convert_ctx)
                        continue;

                    int h = sws_scale(img_convert_ctx,  (const uint8_t *const *)inputFrame->data, inputFrame->linesize,  0, avctx->height, outputFrame->data, outputFrame->linesize);
                    if (h <= 0)
                    {
                        std::cerr << "sws_scale failed or returned empty image" << std::endl;
                        continue;
                    }

                    std::lock_guard<std::mutex> lock(frame_mutex);

                    int data_size = outputFrame->linesize[0] * size_y;
                    latest_frame_data.assign(outputFrame->data[0], outputFrame->data[0] + data_size);
                    latest_frame_linesize = outputFrame->linesize[0];
                    frame_ready = true;
                    frame_cv.notify_one();
                }
            }
        });
    }



    void StopCaptureThread()
    {
        stop_capture = true;
        if (capture_thread.joinable())
            capture_thread.join();
    }


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

        Bind(device_name, "device_name");
        Bind(device_index, "device_index");

        auto dev_list = get_avfoundation_devices();

        if(listDevices)
            for(int i = 0; i < dev_list.size(); i++)
                std::cout << "Device " << std::to_string(i) << ": " << dev_list[i] << std::endl;

        if(!device_name.empty()) // Remapping id
        {
            int new_index = map_device_name_to_index(dev_list, device_name.as_string(), device_index.as_int());
            if(new_index == -1)
                throw  fatal_error("Device \"" + device_name.as_string() + "\" with index " + device_index.as_int_string()+" not found.");
 
            id = std::to_string(new_index); // Set the id to the new index
            std::cout << "Using device: " <<  id.as_int_string() << "  = " << dev_list[new_index] << std::endl;
        }

        // Setting options
        std::string sizeString = size_x.as_int_string() + "x" + size_y.as_int_string();
        std::string frameRateString = frameRate.as_int_string();
        std::string idString = id.as_int_string();

        input_format_context = avformat_alloc_context();
        avdevice_register_all();

        const AVInputFormat *ifmt = av_find_input_format("avfoundation");
        
        av_dict_set(&options, "video_size", sizeString.c_str(), 0);
        av_dict_set(&options, "pixel_format", "1", 0); // 1 = uyvy422
        av_dict_set(&options, "framerate", frameRateString.c_str(), 0);

        if(avformat_open_input(&input_format_context, idString.c_str(), ifmt, &options) != 0)
            throw fatal_error("Couldn't open input stream.");

        if(avformat_find_stream_info(input_format_context, nullptr) < 0)
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
        if(input_codec == nullptr)
            throw fatal_error("Unsupported codec!");

        if((avctx = avcodec_alloc_context3(input_codec)) == nullptr)
            throw fatal_error("Could not allocate a decoding context");

        if(avcodec_parameters_to_context(avctx, (*input_format_context).streams[videoStreamId]->codecpar) < 0)
            throw fatal_error("Could not copy codec parameters to context");

        if(avcodec_open2(avctx, input_codec, nullptr) < 0)
            throw fatal_error("Could not open codec");

        // Input
        if((inputFrame = av_frame_alloc()) == nullptr)
            throw fatal_error("Could not allocate input AVFrame");

        // Output (after resize and convertions)
        if((outputFrame = av_frame_alloc()) == nullptr)
            throw fatal_error("Could not allocate output AVFrame");

        outputFrame->format = AV_PIX_FMT_RGB24;
        outputFrame->width = size_x;
        outputFrame->height = size_y;

    if (av_image_alloc(outputFrame->data, outputFrame->linesize, size_x, size_y, AV_PIX_FMT_RGB24, 32) < 0)
        throw fatal_error("Could not allocate image buffer for outputFrame");


        StartCaptureThread();
    }




    int
    Decode(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt)
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

        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) 
        {
            char errbuf[128];
            av_strerror(ret, errbuf, sizeof(errbuf));
            std::cerr << "Decoding error: " << errbuf << std::endl;
            return ret;
     }

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


        std::unique_lock<std::mutex> lock(frame_mutex);
        if (!frame_cv.wait_for(lock, std::chrono::milliseconds(1), [this]{ return frame_ready; }))
            return; // No new frame available

        unsigned char *data = latest_frame_data.data();
        int linesize = latest_frame_linesize;
        float * r = output[0].data();
        float * g = output[1].data();
        float * b = output[2].data();

        float * r_ = red.data();
        float * g_ = green.data();
        float * b_ = blue.data();

        float * intens = intensity.data();
        int p = 0;
    for (int row = 0; row < sy; ++row) 
    {
            int row_offset = row * linesize;
            for (int col = 0; col < sx; ++col, p++) 
            {
                int idx = row_offset + col * 3;
                float rf = c1_255 * data[idx];
                float gf = c1_255 * data[idx + 1];
                float bf = c1_255 * data[idx + 2];

                r[p] = r_[p] = rf;
                g[p] = g_[p] = gf;
                b[p] = b_[p] = bf;
                intens[p] = c1_3 * (rf + gf + bf);
            }
        }
        frame_ready = false;
    }

    ~InputVideo()
    {
        StopCaptureThread();
        if (inputFrame)
            av_frame_free(&inputFrame);

        if (outputFrame)
        {
            av_freep(&outputFrame->data[0]); // Free data allocated by av_image_alloc()
            av_frame_free(&outputFrame);
        }

        if (avctx) 
            avcodec_free_context(&avctx);

        if (input_format_context)
            avformat_close_input(&input_format_context);

        if (options)
            av_dict_free(&options);

        if (img_convert_ctx)
            sws_freeContext(img_convert_ctx);
    }
};

INSTALL_CLASS(InputVideo)
