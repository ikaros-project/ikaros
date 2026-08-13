#pragma once

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "matrix.h"

namespace ikaros
{
    std::string serialize_webui_image(const matrix & image,
                                      const std::string & format,
                                      int quality);

    class WebUIImageEncoderPool
    {
    public:
        struct Request
        {
            std::string key;
            std::string source;
            std::string format;
            int quality = 75;
            std::shared_ptr<const matrix> image;
        };

        struct Failure
        {
            std::string key;
            std::string message;
        };

        explicit WebUIImageEncoderPool(std::size_t worker_count);
        ~WebUIImageEncoderPool();

        WebUIImageEncoderPool(const WebUIImageEncoderPool &) = delete;
        WebUIImageEncoderPool & operator=(const WebUIImageEncoderPool &) = delete;

        void Reset(std::uint64_t generation);
        bool Busy() const;
        std::shared_ptr<const matrix> Capture(std::uint64_t generation,
                                              const std::string & source,
                                              const matrix & image);
        bool Submit(std::uint64_t generation, std::vector<Request> requests);
        std::shared_ptr<const std::string> Latest(std::uint64_t generation,
                                                  const std::string & key) const;
        void RetainLatest(std::uint64_t generation,
                          const std::unordered_set<std::string> & keys);
        std::vector<Failure> TakeFailures(std::uint64_t generation);

    private:
        struct WorkItem
        {
            std::uint64_t generation = 0;
            std::uint64_t batch = 0;
            Request request;
        };

        void Worker();

        mutable std::mutex mutex_;
        std::condition_variable condition_;
        std::vector<std::thread> workers_;
        std::deque<WorkItem> queue_;
        std::unordered_map<std::string, std::shared_ptr<matrix>> staging_images_;
        std::unordered_set<std::string> prepared_sources_;
        std::unordered_map<std::string, std::shared_ptr<const std::string>> latest_images_;
        std::vector<Failure> failures_;
        std::uint64_t generation_ = 0;
        std::uint64_t batch_ = 0;
        std::size_t outstanding_ = 0;
        bool batch_active_ = false;
        bool stopping_ = false;
    };
}
