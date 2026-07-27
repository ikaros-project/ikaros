#include <chrono>
#include <functional>
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
    void require(bool condition, const std::string & message)
    {
        if(!condition)
            throw std::runtime_error("WebUIImageEncoderTestModule: " + message);
    }


    void wait_until(const std::function<bool()> & predicate,
                    const std::string & message)
    {
        const auto deadline = std::chrono::steady_clock::now() + 5s;
        while(!predicate())
        {
            if(std::chrono::steady_clock::now() >= deadline)
                throw std::runtime_error("WebUIImageEncoderTestModule: " + message);
            std::this_thread::sleep_for(1ms);
        }
    }
}


class WebUIImageEncoderTestModule: public Module
{
public:
    void
    Tick() override
    {
        bool rejected_zero_workers = false;
        try
        {
            WebUIImageEncoderPool invalid_pool(0);
        }
        catch(const std::invalid_argument &)
        {
            rejected_zero_workers = true;
        }
        require(rejected_zero_workers, "zero-worker pool was accepted");

        WebUIImageEncoderPool pool(1);
        constexpr std::uint64_t first_generation = 17;
        pool.Reset(first_generation);

        matrix gray(1024, 1024);
        for(int y = 0; y < gray.size(0); ++y)
            for(int x = 0; x < gray.size(1); ++x)
                gray(y, x) = static_cast<float>((17 * x + 31 * y) & 255) / 255.0f;

        auto captured = pool.Capture(first_generation, "Source.OUTPUT", gray);
        require(captured != nullptr && captured->shape() == gray.shape(),
                "image capture failed");

        std::vector<WebUIImageEncoderPool::Request> requests;
        for(int i = 0; i < 8; ++i)
            requests.push_back({
                "image-" + std::to_string(i),
                "Source.OUTPUT",
                "gray",
                70,
                captured,
            });

        require(pool.Submit(first_generation, std::move(requests)),
                "first image batch was rejected");
        require(pool.Busy(), "submitted image batch was not marked active");
        require(pool.Latest(first_generation, "image-7") == nullptr,
                "image submission waited for the complete batch");
        require(!pool.Submit(first_generation, {{
                    "coalesced", "Source.OUTPUT", "gray", 70, captured,
                }}),
                "a second batch was queued while encoding was active");

        wait_until([&pool] { return !pool.Busy(); },
                   "image batch did not complete");
        auto encoded = pool.Latest(first_generation, "image-7");
        require(encoded != nullptr &&
                encoded->starts_with("\"data:image/jpeg;base64,") &&
                encoded->ends_with("\""),
                "completed image was not published");

        gray(0, 0) = 1.0f;
        auto reused_capture = pool.Capture(first_generation, "Source.OUTPUT", gray);
        require(reused_capture == captured && (*reused_capture)(0, 0) == 1.0f,
                "staging image was not reused or refreshed");
        require(pool.Submit(first_generation, {{
                    "image-7", "Source.OUTPUT", "gray", 0, reused_capture,
                }}),
                "failing image batch was rejected");
        wait_until([&pool] { return !pool.Busy(); },
                   "failing image batch did not complete");
        const auto failures = pool.TakeFailures(first_generation);
        require(failures.size() == 1 && failures.front().key == "image-7" &&
                pool.Latest(first_generation, "image-7") == encoded,
                "encoding failure replaced the last image or lost its context");

        auto stale_capture = pool.Capture(first_generation, "Source.OUTPUT", gray);
        require(pool.Submit(first_generation, {{
                    "stale", "Source.OUTPUT", "gray", 70, stale_capture,
                }}),
                "stale-generation batch was rejected");

        constexpr std::uint64_t second_generation = 18;
        pool.Reset(second_generation);
        require(!pool.Busy() && pool.Latest(first_generation, "image-7") == nullptr,
                "reset retained old-generation state");

        matrix replacement(2, 2);
        replacement.set(0.5f);
        auto replacement_capture = pool.Capture(
            second_generation, "Replacement.OUTPUT", replacement);
        require(pool.Submit(second_generation, {{
                    "replacement", "Replacement.OUTPUT", "gray", 70,
                    replacement_capture,
                }}),
                "new-generation batch was rejected");
        wait_until([&pool] { return !pool.Busy(); },
                   "new-generation batch did not complete");
        require(pool.Latest(second_generation, "replacement") != nullptr,
                "new-generation image was not published");

        pool.RetainLatest(second_generation, {});
        require(pool.Latest(second_generation, "replacement") == nullptr,
                "inactive cached image was not released");

        std::cout << "WEBUI IMAGE ENCODER TEST OK\n";
    }
};

INSTALL_CLASS(WebUIImageEncoderTestModule)
