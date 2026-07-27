#include "webui_image_encoder.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "image_file_formats.h"
#include "utilities.h"

namespace ikaros
{
    std::string
    serialize_webui_image(const matrix & image, const std::string & format, int quality)
    {
        jpeg_data jpeg;

        if(format == "rgb" && image.rank() == 3 && image.size(0) == 3)
            jpeg = create_color_jpeg(image, quality);
        else if(format == "gray" && image.rank() == 2)
            jpeg = create_gray_jpeg(image, 0, 1, quality);
        else if(image.rank() == 2)
            jpeg = create_pseudocolor_jpeg(image, 0, 1, format, quality);

        if(jpeg.empty())
            return "\"\"";

        std::string result = "\"data:image/jpeg;base64,";
        result += base64_encode(jpeg.data(), jpeg.size());
        result += "\"";
        return result;
    }


    WebUIImageEncoderPool::WebUIImageEncoderPool(std::size_t worker_count)
    {
        if(worker_count == 0)
            throw std::invalid_argument("WebUI image encoder requires at least one worker.");

        try
        {
            workers_.reserve(worker_count);
            for(std::size_t i = 0; i < worker_count; ++i)
                workers_.emplace_back(&WebUIImageEncoderPool::Worker, this);
        }
        catch(...)
        {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                stopping_ = true;
            }
            condition_.notify_all();
            for(std::thread & worker : workers_)
                if(worker.joinable())
                    worker.join();
            throw;
        }
    }


    WebUIImageEncoderPool::~WebUIImageEncoderPool()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
            queue_.clear();
        }
        condition_.notify_all();
        for(std::thread & worker : workers_)
            if(worker.joinable())
                worker.join();
    }


    void
    WebUIImageEncoderPool::Reset(std::uint64_t generation)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        generation_ = generation;
        ++batch_;
        queue_.clear();
        staging_images_.clear();
        prepared_sources_.clear();
        latest_images_.clear();
        failures_.clear();
        outstanding_ = 0;
        batch_active_ = false;
    }


    bool
    WebUIImageEncoderPool::Busy() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return batch_active_;
    }


    std::shared_ptr<const matrix>
    WebUIImageEncoderPool::Capture(std::uint64_t generation,
                                   const std::string & source,
                                   const matrix & image)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(generation != generation_ || batch_active_ || stopping_)
            return nullptr;

        auto & staging = staging_images_[source];
        if(prepared_sources_.contains(source))
            return staging;
        if(staging == nullptr)
            staging = std::make_shared<matrix>();
        if(staging->is_uninitialized() || staging->shape() != image.shape())
            *staging = image.clone();
        else
            staging->copy(image);
        prepared_sources_.insert(source);
        return staging;
    }


    bool
    WebUIImageEncoderPool::Submit(std::uint64_t generation,
                                  std::vector<Request> requests)
    {
        if(requests.empty())
            return false;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(generation != generation_ || batch_active_ || stopping_)
                return false;

            ++batch_;
            outstanding_ = requests.size();
            batch_active_ = true;
            try
            {
                std::unordered_set<std::string> retained_sources;
                retained_sources.reserve(requests.size());
                for(const Request & request : requests)
                    retained_sources.insert(request.source);
                for(auto it = staging_images_.begin(); it != staging_images_.end();)
                    if(!retained_sources.contains(it->first))
                        it = staging_images_.erase(it);
                    else
                        ++it;
                prepared_sources_.clear();

                for(Request & request : requests)
                {
                    if(request.image == nullptr)
                        throw std::invalid_argument("WebUI image request has no captured image.");
                    queue_.push_back({generation, batch_, std::move(request)});
                }
            }
            catch(...)
            {
                queue_.erase(std::remove_if(queue_.begin(), queue_.end(),
                                            [this](const WorkItem & item)
                                            {
                                                return item.batch == batch_;
                                            }),
                             queue_.end());
                outstanding_ = 0;
                batch_active_ = false;
                prepared_sources_.clear();
                throw;
            }
        }
        condition_.notify_all();
        return true;
    }


    std::shared_ptr<const std::string>
    WebUIImageEncoderPool::Latest(std::uint64_t generation,
                                  const std::string & key) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(generation != generation_)
            return nullptr;
        auto it = latest_images_.find(key);
        return it == latest_images_.end() ? nullptr : it->second;
    }


    void
    WebUIImageEncoderPool::RetainLatest(
        std::uint64_t generation,
        const std::unordered_set<std::string> & keys)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(generation != generation_)
            return;
        for(auto it = latest_images_.begin(); it != latest_images_.end();)
            if(!keys.contains(it->first))
                it = latest_images_.erase(it);
            else
                ++it;
    }


    std::vector<WebUIImageEncoderPool::Failure>
    WebUIImageEncoderPool::TakeFailures(std::uint64_t generation)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(generation != generation_)
            return {};
        std::vector<Failure> result = std::move(failures_);
        failures_.clear();
        return result;
    }


    void
    WebUIImageEncoderPool::Worker()
    {
        while(true)
        {
            WorkItem item;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this]
                                {
                                    return stopping_ || !queue_.empty();
                                });
                if(stopping_)
                    return;
                item = std::move(queue_.front());
                queue_.pop_front();
            }

            std::shared_ptr<const std::string> encoded;
            std::string failure;
            try
            {
                encoded = std::make_shared<const std::string>(
                    serialize_webui_image(*item.request.image,
                                          item.request.format,
                                          item.request.quality));
            }
            catch(const std::exception & error)
            {
                failure = error.what();
            }
            catch(...)
            {
                failure = "Unknown image encoding error.";
            }

            {
                std::lock_guard<std::mutex> lock(mutex_);
                if(item.generation != generation_ || item.batch != batch_)
                    continue;

                if(encoded != nullptr)
                    latest_images_[item.request.key] = std::move(encoded);
                else
                    failures_.push_back({item.request.key, std::move(failure)});

                if(outstanding_ > 0)
                    --outstanding_;
                if(outstanding_ == 0)
                    batch_active_ = false;
            }
        }
    }
}
