// image_file_format.h
// Copyright (C) 2023-2025  Christian Balkenius

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

namespace ikaros
{
    class matrix;

    struct image_info
    {
        int width;
        int height;
        int channels;
    };

    class jpeg_data
    {
    public:
        jpeg_data() noexcept = default;
        jpeg_data(const jpeg_data &) = delete;
        jpeg_data & operator=(const jpeg_data &) = delete;
        jpeg_data(jpeg_data && other) noexcept :
            data_(std::move(other.data_)),
            size_(std::exchange(other.size_, 0))
        {
        }

        jpeg_data & operator=(jpeg_data && other) noexcept
        {
            if(this != &other)
            {
                data_ = std::move(other.data_);
                size_ = std::exchange(other.size_, 0);
            }
            return *this;
        }

        [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
        [[nodiscard]] std::size_t size() const noexcept { return size_; }
        [[nodiscard]] const std::uint8_t * data() const noexcept { return data_.get(); }
        [[nodiscard]] std::span<const std::uint8_t> bytes() const noexcept
        {
            return {data_.get(), size_};
        }

    private:
        struct FreeDeleter
        {
            void operator()(std::uint8_t * data) const noexcept;
        };

        jpeg_data(std::uint8_t * data, std::size_t size) noexcept : data_(data), size_(size) {}

        std::unique_ptr<std::uint8_t, FreeDeleter> data_;
        std::size_t size_ = 0;

        friend class JpegCompressor;
    };

    [[nodiscard]] jpeg_data create_color_jpeg(const matrix & image, int quality = 100);
    [[nodiscard]] jpeg_data create_gray_jpeg(const matrix & image, float minimum = 0,
                                             float maximum = 1, int quality = 100);
    [[nodiscard]] jpeg_data create_pseudocolor_jpeg(const matrix & image, float minimum = 0,
                                                    float maximum = 1,
                                                    std::string_view table = "fire",
                                                    int quality = 100);

    [[nodiscard]] image_info jpeg_get_info(const std::filesystem::path & filename);
    // Decoded RGB images use [channel, height, width]. Existing destinations are reused.
    void jpeg_get_image(matrix & image, const std::filesystem::path & filename);
    [[nodiscard]] matrix jpeg_get_image(const std::filesystem::path & filename);


    [[nodiscard]] image_info png_get_info(const std::filesystem::path & filename);
    // Decoded RGB images use [channel, height, width]. Existing destinations are reused.
    void png_get_image(matrix & image, const std::filesystem::path & filename);
    [[nodiscard]] matrix png_get_image(const std::filesystem::path & filename);
};
