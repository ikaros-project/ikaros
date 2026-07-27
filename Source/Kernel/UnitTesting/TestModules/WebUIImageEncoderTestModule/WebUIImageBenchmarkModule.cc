#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "ikaros.h"

using namespace ikaros;
using namespace std::chrono_literals;

namespace
{
    double
    median(std::vector<double> values)
    {
        std::sort(values.begin(), values.end());
        const std::size_t middle = values.size() / 2;
        if(values.size() % 2 == 0)
            return 0.5 * (values[middle - 1] + values[middle]);
        return values[middle];
    }


    void
    wait_for_encoder(WebUIImageEncoderPool & encoder)
    {
        const auto deadline = std::chrono::steady_clock::now() + 10s;
        while(encoder.Busy())
        {
            if(std::chrono::steady_clock::now() >= deadline)
                throw std::runtime_error("WebUI image benchmark encoder timed out");
            std::this_thread::yield();
        }
    }
}


class WebUIImageBenchmarkModule: public Module
{
    parameter width;
    parameter height;
    parameter iterations;
    parameter quality;
    matrix image;
    std::size_t phase = 0;

public:
    void
    Init() override
    {
        Bind(width, "width");
        Bind(height, "height");
        Bind(iterations, "iterations");
        Bind(quality, "quality");
        Bind(image, "IMAGE");

        const int imageWidth = width.as_int();
        const int imageHeight = height.as_int();
        const int measuredIterations = iterations.as_int();
        const int jpegQuality = quality.as_int();
        if(imageWidth <= 0 || imageHeight <= 0 || measuredIterations <= 0 ||
           jpegQuality < 1 || jpegQuality > 100)
            throw std::invalid_argument("Invalid WebUI image benchmark parameters");
        if(image.rank() != 2 || image.size(0) != imageHeight ||
           image.size(1) != imageWidth)
            throw std::runtime_error("WebUI image benchmark output has the wrong shape");

        for(int y = 0; y < imageHeight; ++y)
            for(int x = 0; x < imageWidth; ++x)
                image(y, x) = static_cast<float>((17 * x + 31 * y) & 255) / 255.0f;

        constexpr std::uint64_t generation = 1;
        const std::string source = path_ + ".IMAGE";
        WebUIImageEncoderPool encoder(1);
        encoder.Reset(generation);

        auto submit_capture = [&](const std::string & key,
                                  const std::shared_ptr<const matrix> & captured)
        {
            if(captured == nullptr ||
               !encoder.Submit(generation, {{
                   key,
                   source,
                   "gray",
                   jpegQuality,
                   captured,
               }}))
                throw std::runtime_error("Could not submit WebUI image benchmark capture");
            wait_for_encoder(encoder);
        };

        submit_capture("warmup", encoder.Capture(generation, source, image));
        serialize_webui_image(image, "gray", jpegQuality);

        std::vector<double> captureMilliseconds;
        std::vector<double> jpegBase64Milliseconds;
        captureMilliseconds.reserve(measuredIterations);
        jpegBase64Milliseconds.reserve(measuredIterations);
        std::size_t encodedChecksum = 0;

        for(int iteration = 0; iteration < measuredIterations; ++iteration)
        {
            image(0, 0) = static_cast<float>(iteration & 255) / 255.0f;

            const auto captureStart = std::chrono::steady_clock::now();
            auto captured = encoder.Capture(generation, source, image);
            const auto captureEnd = std::chrono::steady_clock::now();
            captureMilliseconds.push_back(
                std::chrono::duration<double, std::milli>(
                    captureEnd - captureStart).count());
            submit_capture("capture", captured);

            const auto encodeStart = std::chrono::steady_clock::now();
            const std::string encoded =
                serialize_webui_image(image, "gray", jpegQuality);
            const auto encodeEnd = std::chrono::steady_clock::now();
            jpegBase64Milliseconds.push_back(
                std::chrono::duration<double, std::milli>(
                    encodeEnd - encodeStart).count());
            encodedChecksum += encoded.size();
        }

        std::cout << std::fixed << std::setprecision(6)
                  << path_ << " WEBUI IMAGE CORE BENCHMARK"
                  << " width=" << imageWidth
                  << " height=" << imageHeight
                  << " iterations=" << measuredIterations
                  << " capture_ms=" << median(captureMilliseconds)
                  << " jpeg_base64_ms=" << median(jpegBase64Milliseconds)
                  << " checksum=" << encodedChecksum
                  << std::endl;
    }


    void
    Tick() override
    {
        ++phase;
        image(0, 0) = static_cast<float>(phase & 255) / 255.0f;
    }
};

INSTALL_CLASS(WebUIImageBenchmarkModule)
