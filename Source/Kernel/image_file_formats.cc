// image_file_format.cc
// Copyright (C) 2023-2025  Christian Balkenius

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <memory>
#include <new>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "matrix.h"
#include "image_file_formats.h"
#include "color_tables.h"

#if defined(__APPLE__) && defined(IKAROS_MATRIX_ACCELERATE) && IKAROS_MATRIX_ACCELERATE
// Avoid Accelerate.h, which pulls deprecated compatibility headers into C++ builds.
#include <Accelerate/../Frameworks/vImage.framework/Headers/Conversion.h>
#define IKAROS_IMAGE_ACCELERATE 1
#else
#define IKAROS_IMAGE_ACCELERATE 0
#endif

extern "C"
{
#include "jpeglib.h"
#include <setjmp.h>
#include <png.h>
}
namespace ikaros
{


    void
    jpeg_data::FreeDeleter::operator()(std::uint8_t * data) const noexcept
    {
        std::free(data);
    }


    struct jpeg_error_context
    {
        jpeg_error_mgr pub;
        jmp_buf setjmp_buffer;
        char message[JMSG_LENGTH_MAX];
        bool reject_warnings = false;
    };


    static void
    jpeg_error_exit(j_common_ptr cinfo)
    {
        auto * error = reinterpret_cast<jpeg_error_context *>(cinfo->err);
        (*cinfo->err->format_message)(cinfo, error->message);
        longjmp(error->setjmp_buffer, 1);
    }


    static void
    jpeg_emit_message(j_common_ptr cinfo, int message_level)
    {
        if(message_level >= 0)
            return;

        auto * error = reinterpret_cast<jpeg_error_context *>(cinfo->err);
        ++error->pub.num_warnings;
        if(error->reject_warnings)
            jpeg_error_exit(cinfo);
    }


    static void
    validate_float_to_byte_range(float minimum, float maximum)
    {
        if(!std::isfinite(minimum) || !std::isfinite(maximum))
            throw std::invalid_argument("JPEG conversion range must be finite.");
        if(maximum < minimum)
            throw std::invalid_argument("JPEG conversion maximum must not be less than its minimum.");
        if(maximum > minimum && !std::isfinite(maximum - minimum))
            throw std::invalid_argument("JPEG conversion range is too wide.");
    }


    static inline unsigned char
    normalized_float_to_byte(float value, float minimum, float maximum, float scale) noexcept
    {
        if(!(value > minimum))
            return 0;
        if(value >= maximum)
            return 255;
        return static_cast<unsigned char>((value - minimum) * scale + 0.5f);
    }


    static void
    float_to_byte(std::span<unsigned char> result, std::span<const float> source,
                  float minimum, float maximum)
    {
        const std::size_t size = source.size();
        if(maximum == minimum)
        {
            std::fill(result.begin(), result.end(), static_cast<unsigned char>(0));
            return;
        }

#if IKAROS_IMAGE_ACCELERATE
        vImage_Buffer source_buffer =
        {
            const_cast<float *>(source.data()),
            1,
            static_cast<vImagePixelCount>(size),
            size * sizeof(float),
        };
        vImage_Buffer result_buffer =
        {
            result.data(),
            1,
            static_cast<vImagePixelCount>(size),
            size * sizeof(unsigned char),
        };
        if(vImageConvert_PlanarFtoPlanar8(&source_buffer, &result_buffer,
                                          maximum, minimum, kvImageDoNotTile) == kvImageNoError)
            return;
#endif

        const float scale = 255.0f / (maximum - minimum);
        for(std::size_t i = 0; i < size; ++i)
            result[i] = normalized_float_to_byte(source[i], minimum, maximum, scale);
    }


    static void
    float_rgb_to_byte(std::span<unsigned char> result,
                      std::span<unsigned char> planar_buffer,
                      std::span<const float> red, std::span<const float> green,
                      std::span<const float> blue)
    {
        const std::size_t size = red.size();
#if IKAROS_IMAGE_ACCELERATE
        const auto width = static_cast<vImagePixelCount>(size);
        const auto float_row_bytes = size * sizeof(float);
        const auto byte_row_bytes = size * sizeof(unsigned char);
        vImage_Buffer source_red{const_cast<float *>(red.data()), 1, width, float_row_bytes};
        vImage_Buffer source_green{const_cast<float *>(green.data()), 1, width, float_row_bytes};
        vImage_Buffer source_blue{const_cast<float *>(blue.data()), 1, width, float_row_bytes};
        vImage_Buffer planar_red{planar_buffer.data(), 1, width, byte_row_bytes};
        vImage_Buffer planar_green{planar_buffer.data() + size, 1, width, byte_row_bytes};
        vImage_Buffer planar_blue{planar_buffer.data() + 2 * size, 1, width, byte_row_bytes};
        vImage_Buffer destination_buffer{result.data(), 1, width, 3 * byte_row_bytes};

        if(vImageConvert_PlanarFtoPlanar8(&source_red, &planar_red, 1, 0,
                                          kvImageDoNotTile) == kvImageNoError &&
           vImageConvert_PlanarFtoPlanar8(&source_green, &planar_green, 1, 0,
                                          kvImageDoNotTile) == kvImageNoError &&
           vImageConvert_PlanarFtoPlanar8(&source_blue, &planar_blue, 1, 0,
                                          kvImageDoNotTile) == kvImageNoError &&
           vImageConvert_Planar8toRGB888(&planar_red, &planar_green, &planar_blue,
                                         &destination_buffer, kvImageDoNotTile) == kvImageNoError)
            return;
#else
        static_cast<void>(planar_buffer);
#endif

        unsigned char * destination = result.data();
        for(std::size_t i = 0; i < size; ++i)
        {
            *destination++ = normalized_float_to_byte(red[i], 0, 1, 255);
            *destination++ = normalized_float_to_byte(green[i], 0, 1, 255);
            *destination++ = normalized_float_to_byte(blue[i], 0, 1, 255);
        }
    }


    static void
    rgb8_to_planar_float(const unsigned char * source, float * red, float * green,
                         float * blue, std::size_t size) noexcept
    {
        constexpr float scale = 1.0f / 255.0f;
        for(std::size_t i = 0; i < size; ++i)
        {
            red[i] = source[3 * i] * scale;
            green[i] = source[3 * i + 1] * scale;
            blue[i] = source[3 * i + 2] * scale;
        }
    }


    class JpegCompressor
    {
    public:
        JpegCompressor() = default;
        JpegCompressor(const JpegCompressor &) = delete;
        JpegCompressor & operator=(const JpegCompressor &) = delete;

        ~JpegCompressor()
        {
            reset();
        }

        template<typename RowConverter>
        jpeg_data encode(long width, long height, int components, J_COLOR_SPACE color_space,
                         int quality, std::span<JSAMPLE> row, RowConverter && convert_row)
        {
            if(width <= 0 || height <= 0 ||
               width > static_cast<long>(std::numeric_limits<JDIMENSION>::max()) ||
               height > static_cast<long>(std::numeric_limits<JDIMENSION>::max()))
                throw std::runtime_error("JPEG encoding failed: Invalid image dimensions");
            if(quality < 1 || quality > 100)
                throw std::invalid_argument("JPEG quality must be between 1 and 100");

            cinfo_.err = jpeg_std_error(&error_.pub);
            error_.pub.error_exit = jpeg_error_exit;
            error_.pub.emit_message = jpeg_emit_message;
            error_.reject_warnings = false;

            if(setjmp(error_.setjmp_buffer))
            {
                const std::string message(error_.message);
                reset();
                throw std::runtime_error("JPEG encoding failed: " + message);
            }

            created_ = true;
            jpeg_create_compress(&cinfo_);

            cinfo_.image_width = static_cast<JDIMENSION>(width);
            cinfo_.image_height = static_cast<JDIMENSION>(height);
            cinfo_.input_components = components;
            cinfo_.in_color_space = color_space;

            const std::size_t pixel_count = checked_pixel_count(width, height);
            // Very high-quality JPEGs can approach two compressed bytes per pixel.
            const unsigned long bytes_per_pixel = quality > 90 ? 2 : 1;
            if(pixel_count > std::numeric_limits<unsigned long>::max() / bytes_per_pixel)
                throw std::length_error("JPEG output capacity overflow");
            output_size_ = std::max<unsigned long>(4096,
                                                   static_cast<unsigned long>(pixel_count) *
                                                       bytes_per_pixel);
            output_buffer_ = static_cast<unsigned char *>(std::malloc(output_size_));
            if(output_buffer_ == nullptr)
                throw std::bad_alloc();
            jpeg_mem_dest(&cinfo_, &output_buffer_, &output_size_);

            jpeg_set_defaults(&cinfo_);
            jpeg_set_quality(&cinfo_, quality, TRUE);
            jpeg_start_compress(&cinfo_, TRUE);

            while(cinfo_.next_scanline < cinfo_.image_height)
            {
                convert_row(cinfo_.next_scanline, row);
                JSAMPROW row_pointer = row.data();
                jpeg_write_scanlines(&cinfo_, &row_pointer, 1);
            }

            jpeg_finish_compress(&cinfo_);
            jpeg_data result(output_buffer_, static_cast<std::size_t>(output_size_));
            output_buffer_ = nullptr;
            reset();
            return result;
        }

    private:
        static std::size_t checked_pixel_count(long width, long height)
        {
            const auto unsigned_width = static_cast<std::size_t>(width);
            const auto unsigned_height = static_cast<std::size_t>(height);
            if(unsigned_height > std::numeric_limits<std::size_t>::max() / unsigned_width)
                throw std::length_error("JPEG image size overflow");
            return unsigned_width * unsigned_height;
        }

        void reset() noexcept
        {
            if(created_)
                jpeg_destroy_compress(&cinfo_);
            created_ = false;
            std::free(output_buffer_);
            output_buffer_ = nullptr;
            output_size_ = 0;
        }

        jpeg_compress_struct cinfo_{};
        jpeg_error_context error_{};
        unsigned char * output_buffer_ = nullptr;
        unsigned long output_size_ = 0;
        bool created_ = false;
    };


    jpeg_data
    create_gray_jpeg(const matrix & image, float minimum, float maximum, int quality)
    {
        if(image.rank() != 2)
            return {};
        validate_float_to_byte_range(minimum, maximum);

        const long width = image.size(1);
        const long height = image.size(0);
        std::vector<JSAMPLE> image_row(static_cast<std::size_t>(width));
        JpegCompressor compressor;
        return compressor.encode(width, height, 1, JCS_GRAYSCALE, quality, image_row,
                                 [&](JDIMENSION row, std::span<JSAMPLE> destination)
                                 {
                                     const auto source = std::span(
                                         image.logical_block_data(static_cast<int>(row)),
                                         static_cast<std::size_t>(width));
                                     float_to_byte(destination, source, minimum, maximum);
                                 });
    }
    
    
    jpeg_data
    create_pseudocolor_jpeg(const matrix & image, float minimum, float maximum,
                            std::string_view table, int quality)
    {
        if(image.rank() != 2)
            return {};
        const image_color_tables::ColorTable * colors =
            image_color_tables::find_color_table(table);
        if(colors == nullptr)
            return {};
        validate_float_to_byte_range(minimum, maximum);

        const long width = image.size(1);
        const long height = image.size(0);
        const std::size_t row_width = static_cast<std::size_t>(width);
        std::vector<JSAMPLE> image_row(3 * row_width);
        std::vector<unsigned char> normalized_row(row_width);
        JpegCompressor compressor;
        return compressor.encode(width, height, 3, JCS_RGB, quality, image_row,
                                 [&](JDIMENSION row, std::span<JSAMPLE> destination)
                                 {
                                     const auto source = std::span(
                                         image.logical_block_data(static_cast<int>(row)),
                                         row_width);
                                     float_to_byte(normalized_row, source, minimum, maximum);
                                     for(std::size_t x = 0; x < row_width; ++x)
                                     {
                                         const auto & color = (*colors)[normalized_row[x]];
                                         destination[3 * x] = static_cast<JSAMPLE>(color[0]);
                                         destination[3 * x + 1] = static_cast<JSAMPLE>(color[1]);
                                         destination[3 * x + 2] = static_cast<JSAMPLE>(color[2]);
                                     }
                                 });
    }
    jpeg_data
    create_color_jpeg(const matrix & image, int quality)
    {
        if(image.rank() != 3 || image.size(0) != 3)
            return {};

        const long width = image.size(2);
        const long height = image.size(1);
        const std::size_t row_width = static_cast<std::size_t>(width);
        std::vector<JSAMPLE> image_storage((IKAROS_IMAGE_ACCELERATE ? 6 : 3) * row_width);
        std::span<JSAMPLE> image_row(image_storage.data(), 3 * row_width);
        std::span<JSAMPLE> planar_buffer;
#if IKAROS_IMAGE_ACCELERATE
        planar_buffer = std::span(image_storage).subspan(3 * row_width);
#endif

        JpegCompressor compressor;
        return compressor.encode(width, height, 3, JCS_RGB, quality, image_row,
                                 [&](JDIMENSION row, std::span<JSAMPLE> destination)
                                 {
                                     const auto row_index = static_cast<int>(row);
                                     const auto red = std::span(
                                         image.logical_block_data(row_index), row_width);
                                     const auto green = std::span(
                                         image.logical_block_data(static_cast<int>(height) +
                                                                  row_index),
                                         row_width);
                                     const auto blue = std::span(
                                         image.logical_block_data(2 * static_cast<int>(height) +
                                                                  row_index),
                                         row_width);
                                     float_rgb_to_byte(destination, planar_buffer,
                                                       red, green, blue);
                                 });
    }


    struct FileCloser
    {
        void operator()(FILE * file) const noexcept
        {
            if(file != nullptr)
                std::fclose(file);
        }
    };


    class JpegDecompressor
    {
    public:
        JpegDecompressor() = default;
        JpegDecompressor(const JpegDecompressor &) = delete;
        JpegDecompressor & operator=(const JpegDecompressor &) = delete;

        ~JpegDecompressor()
        {
            reset();
        }

        template<typename Operation>
        std::invoke_result_t<Operation &, jpeg_decompress_struct &>
        read(const std::filesystem::path & filename, bool reject_warnings,
             Operation && operation)
        {
            file_.reset(std::fopen(filename.string().c_str(), "rb"));
            if(file_ == nullptr)
                throw std::runtime_error("Can't open " + filename.string());

            cinfo_.err = jpeg_std_error(&error_.pub);
            error_.pub.error_exit = jpeg_error_exit;
            error_.pub.emit_message = jpeg_emit_message;
            error_.reject_warnings = reject_warnings;

            if(setjmp(error_.setjmp_buffer))
            {
                const std::string message(error_.message);
                reset();
                throw std::runtime_error("JPEG read failed for \"" + filename.string() +
                                         "\": " + message);
            }

            created_ = true;
            jpeg_create_decompress(&cinfo_);
            jpeg_stdio_src(&cinfo_, file_.get());
            jpeg_read_header(&cinfo_, TRUE);

            using Result = std::invoke_result_t<Operation &, jpeg_decompress_struct &>;
            if constexpr(std::is_void_v<Result>)
            {
                operation(cinfo_);
                reset();
            }
            else
            {
                Result result = operation(cinfo_);
                reset();
                return result;
            }
        }

    private:
        void reset() noexcept
        {
            if(created_)
                jpeg_destroy_decompress(&cinfo_);
            created_ = false;
            file_.reset();
        }

        jpeg_decompress_struct cinfo_{};
        jpeg_error_context error_{};
        std::unique_ptr<FILE, FileCloser> file_;
        bool created_ = false;
    };


    static int
    checked_jpeg_dimension(JDIMENSION value, const std::filesystem::path & filename)
    {
        if(value > static_cast<JDIMENSION>(std::numeric_limits<int>::max()))
            throw std::runtime_error("JPEG dimensions exceed matrix limits for \"" +
                                     filename.string() + "\"");
        return static_cast<int>(value);
    }


    image_info
    jpeg_get_info(const std::filesystem::path & filename)
    {
        JpegDecompressor decompressor;
        return decompressor.read(filename, false,
                                 [&filename](const jpeg_decompress_struct & cinfo)
        {
            return image_info
            {
                checked_jpeg_dimension(cinfo.image_width, filename),
                checked_jpeg_dimension(cinfo.image_height, filename),
                cinfo.num_components,
            };
        });
    }


    static void
    prepare_rgb_image(matrix & image, int height, int width,
                      const std::filesystem::path & filename)
    {
        if(image.is_uninitialized())
        {
            image.realloc(3, height, width);
            return;
        }
        if(image.rank() != 3 || image.size(0) != 3 ||
           image.size(1) != height || image.size(2) != width)
            throw std::invalid_argument("RGB image destination has the wrong shape for \"" +
                                        filename.string() + "\"");
    }


    void
    jpeg_get_image(matrix & image, const std::filesystem::path & filename)
    {
        JpegDecompressor decompressor;
        decompressor.read(filename, true, [&](jpeg_decompress_struct & cinfo)
        {
            cinfo.out_color_space = JCS_RGB;
            jpeg_start_decompress(&cinfo);

            if(cinfo.output_components != 3)
                throw std::runtime_error("JPEG decoder did not produce RGB output for \"" +
                                         filename.string() + "\"");
            if(cinfo.output_width > static_cast<JDIMENSION>(std::numeric_limits<int>::max()) ||
               cinfo.output_height > static_cast<JDIMENSION>(std::numeric_limits<int>::max()))
                throw std::runtime_error("JPEG dimensions exceed matrix limits for \"" +
                                         filename.string() + "\"");

            const int width = static_cast<int>(cinfo.output_width);
            const int height = static_cast<int>(cinfo.output_height);
            const JDIMENSION row_stride = cinfo.output_width * cinfo.output_components;

            prepare_rgb_image(image, height, width, filename);

            JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)(
                reinterpret_cast<j_common_ptr>(&cinfo), JPOOL_IMAGE, row_stride, 1);

            int row = 0;
            while(cinfo.output_scanline < cinfo.output_height)
            {
                jpeg_read_scanlines(&cinfo, buffer, 1);
                float * red_row = image.logical_block_data(row);
                float * green_row = image.logical_block_data(height + row);
                float * blue_row = image.logical_block_data(2 * height + row);
                rgb8_to_planar_float(buffer[0], red_row, green_row, blue_row,
                                     static_cast<std::size_t>(width));
                ++row;
            }

            jpeg_finish_decompress(&cinfo);
        });
    }


    matrix
    jpeg_get_image(const std::filesystem::path & filename)
    {
        matrix image;
        jpeg_get_image(image, filename);
        return image;
    }

    //
    // PNG Images
    //

    struct png_error_context
    {
        char message[256]{};
    };


    static void
    png_reader_error_exit(png_structp png, png_const_charp message)
    {
        auto * error = static_cast<png_error_context *>(png_get_error_ptr(png));
        if(error != nullptr)
            std::snprintf(error->message, sizeof(error->message), "%s",
                          message == nullptr ? "unknown libpng error" : message);
        png_longjmp(png, 1);
    }


    class PngReader
    {
    public:
        PngReader() = default;
        PngReader(const PngReader &) = delete;
        PngReader & operator=(const PngReader &) = delete;

        ~PngReader()
        {
            reset();
        }

        template<typename Operation>
        std::invoke_result_t<Operation &, png_structp, png_infop, PngReader &>
        read(const std::filesystem::path & filename, Operation && operation)
        {
            file_.reset(std::fopen(filename.string().c_str(), "rb"));
            if(file_ == nullptr)
                throw std::runtime_error("Can't open PNG file \"" + filename.string() + "\"");

            png_byte signature[8]{};
            if(std::fread(signature, 1, sizeof(signature), file_.get()) != sizeof(signature))
                throw std::runtime_error("PNG read failed for \"" + filename.string() +
                                         "\": incomplete file signature");
            if(png_sig_cmp(signature, 0, sizeof(signature)) != 0)
                throw std::runtime_error("PNG read failed for \"" + filename.string() +
                                         "\": invalid file signature");

            error_.message[0] = '\0';
            png_ = png_create_read_struct(PNG_LIBPNG_VER_STRING, &error_,
                                          png_reader_error_exit, nullptr);
            if(png_ == nullptr)
                throw std::runtime_error("PNG read failed for \"" + filename.string() +
                                         "\": could not create the read structure");

            if(setjmp(png_jmpbuf(png_)))
            {
                const std::string message = error_.message[0] == '\0' ?
                                            "unknown libpng error" : error_.message;
                reset();
                throw std::runtime_error("PNG read failed for \"" + filename.string() +
                                         "\": " + message);
            }

            info_ = png_create_info_struct(png_);
            if(info_ == nullptr)
                throw std::runtime_error("PNG read failed for \"" + filename.string() +
                                         "\": could not create the info structure");

            png_init_io(png_, file_.get());
            png_set_sig_bytes(png_, sizeof(signature));
            png_read_info(png_, info_);

            using Result = std::invoke_result_t<Operation &, png_structp, png_infop,
                                                PngReader &>;
            if constexpr(std::is_void_v<Result>)
            {
                operation(png_, info_, *this);
                reset();
            }
            else
            {
                Result result = operation(png_, info_, *this);
                reset();
                return result;
            }
        }

        png_bytepp
        allocate_rows(png_uint_32 height, png_size_t row_bytes)
        {
            if(height != 0 && row_bytes > std::numeric_limits<std::size_t>::max() / height)
                throw std::runtime_error("PNG image dimensions exceed addressable memory");

            pixels_.resize(static_cast<std::size_t>(height) * row_bytes);
            rows_.resize(height);
            for(png_uint_32 row = 0; row < height; ++row)
                rows_[row] = pixels_.data() + static_cast<std::size_t>(row) * row_bytes;
            return rows_.data();
        }

    private:
        void
        reset() noexcept
        {
            if(png_ != nullptr)
                png_destroy_read_struct(&png_, info_ == nullptr ? nullptr : &info_, nullptr);
            info_ = nullptr;
            rows_.clear();
            pixels_.clear();
            file_.reset();
        }

        png_error_context error_;
        std::unique_ptr<FILE, FileCloser> file_;
        png_structp png_ = nullptr;
        png_infop info_ = nullptr;
        std::vector<png_byte> pixels_;
        std::vector<png_bytep> rows_;
    };


    static int
    checked_png_dimension(png_uint_32 value, const std::filesystem::path & filename)
    {
        if(value > static_cast<png_uint_32>(std::numeric_limits<int>::max()))
            throw std::runtime_error("PNG dimensions exceed matrix limits for \"" +
                                     filename.string() + "\"");
        return static_cast<int>(value);
    }


    image_info
    png_get_info(const std::filesystem::path & filename)
    {
        PngReader reader;
        return reader.read(filename, [&filename](png_structp png, png_infop info, PngReader &)
        {
            return image_info
            {
                checked_png_dimension(png_get_image_width(png, info), filename),
                checked_png_dimension(png_get_image_height(png, info), filename),
                png_get_channels(png, info),
            };
        });
    }


    void
    png_get_image(matrix & image, const std::filesystem::path & filename)
    {
        PngReader reader;
        reader.read(filename, [&](png_structp png, png_infop info, PngReader & png_reader)
        {
            const int width = checked_png_dimension(png_get_image_width(png, info), filename);
            const int height = checked_png_dimension(png_get_image_height(png, info), filename);
            const int color_type = png_get_color_type(png, info);
            const int bit_depth = png_get_bit_depth(png, info);

            if(color_type == PNG_COLOR_TYPE_PALETTE)
                png_set_palette_to_rgb(png);
            if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
                png_set_expand_gray_1_2_4_to_8(png);
            if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
                png_set_gray_to_rgb(png);
            if(png_get_valid(png, info, PNG_INFO_tRNS))
                png_set_tRNS_to_alpha(png);
            if(bit_depth == 16)
                png_set_strip_16(png);
            if((color_type & PNG_COLOR_MASK_ALPHA) ||
               png_get_valid(png, info, PNG_INFO_tRNS))
                png_set_strip_alpha(png);
            if(png_get_interlace_type(png, info) == PNG_INTERLACE_ADAM7)
                png_set_interlace_handling(png);

            png_read_update_info(png, info);
            if(png_get_channels(png, info) != 3 || png_get_bit_depth(png, info) != 8)
                throw std::runtime_error("Unsupported PNG color format for \"" +
                                         filename.string() + "\"");

            const png_size_t row_bytes = png_get_rowbytes(png, info);
            const png_size_t expected_row_bytes = static_cast<png_size_t>(width) * 3;
            if(row_bytes != expected_row_bytes)
                throw std::runtime_error("Unexpected PNG row layout for \"" +
                                         filename.string() + "\"");

            png_bytepp rows = png_reader.allocate_rows(static_cast<png_uint_32>(height),
                                                        row_bytes);
            png_read_image(png, rows);
            png_read_end(png, info);

            prepare_rgb_image(image, height, width, filename);

            for(int y = 0; y < height; ++y)
            {
                const png_bytep row = rows[y];
                float * red_row = image.logical_block_data(y);
                float * green_row = image.logical_block_data(height + y);
                float * blue_row = image.logical_block_data(2 * height + y);
                rgb8_to_planar_float(row, red_row, green_row, blue_row,
                                     static_cast<std::size_t>(width));
            }
        });
    }


    matrix
    png_get_image(const std::filesystem::path & filename)
    {
        matrix image;
        png_get_image(image, filename);
        return image;
    }

};
