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
#include "jerror.h"
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
    };


    static void
    jpeg_error_exit(j_common_ptr cinfo)
    {
        auto * error = reinterpret_cast<jpeg_error_context *>(cinfo->err);
        (*cinfo->err->format_message)(cinfo, error->message);
        longjmp(error->setjmp_buffer, 1);
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


    struct JpegDestination
    {
        jpeg_destination_mgr manager{};
        JOCTET * buffer = nullptr;
        std::size_t capacity = 0;
        std::size_t used = 0;
    };


    static void
    destination_init(j_compress_ptr cinfo)
    {
        auto * destination = reinterpret_cast<JpegDestination *>(cinfo->dest);
        destination->used = 0;
        destination->manager.next_output_byte = destination->buffer;
        destination->manager.free_in_buffer = destination->capacity;
    }


    static boolean
    destination_empty(j_compress_ptr cinfo)
    {
        auto * destination = reinterpret_cast<JpegDestination *>(cinfo->dest);
        destination->used = destination->capacity - destination->manager.free_in_buffer;
        if(destination->capacity > std::numeric_limits<std::size_t>::max() / 2)
        {
            ERREXIT(cinfo, JERR_OUT_OF_MEMORY);
            return FALSE;
        }

        const std::size_t new_capacity = destination->capacity * 2;
        auto * resized = static_cast<JOCTET *>(
            std::realloc(destination->buffer, new_capacity));
        if(resized == nullptr)
        {
            ERREXIT(cinfo, JERR_OUT_OF_MEMORY);
            return FALSE;
        }

        destination->buffer = resized;
        destination->capacity = new_capacity;
        destination->manager.next_output_byte = destination->buffer + destination->used;
        destination->manager.free_in_buffer = destination->capacity - destination->used;
        return TRUE;
    }


    static void
    destination_term(j_compress_ptr cinfo)
    {
        auto * destination = reinterpret_cast<JpegDestination *>(cinfo->dest);
        destination->used = destination->capacity - destination->manager.free_in_buffer;
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

            cinfo_.err = jpeg_std_error(&error_.pub);
            error_.pub.error_exit = jpeg_error_exit;

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
            if(pixel_count > (std::numeric_limits<std::size_t>::max() - 2048) /
                                 (2 * static_cast<std::size_t>(components)))
                throw std::length_error("JPEG output capacity overflow");
            destination_.capacity =
                2048 + pixel_count * 2 * static_cast<std::size_t>(components);
            destination_.buffer = static_cast<JOCTET *>(std::malloc(destination_.capacity));
            if(destination_.buffer == nullptr)
                throw std::bad_alloc();
            destination_.manager.init_destination = destination_init;
            destination_.manager.empty_output_buffer = destination_empty;
            destination_.manager.term_destination = destination_term;
            cinfo_.dest = &destination_.manager;

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
            jpeg_data result(reinterpret_cast<std::uint8_t *>(destination_.buffer),
                             destination_.used);
            destination_.buffer = nullptr;
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
            std::free(destination_.buffer);
            destination_ = {};
        }

        jpeg_compress_struct cinfo_{};
        jpeg_error_context error_{};
        JpegDestination destination_{};
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
                            const std::string & table, int quality)
    {
        if(image.rank() != 2)
            return {};
        const auto table_iterator = color_table.find(table);
        if(table_iterator == color_table.end())
            return {};
        validate_float_to_byte_range(minimum, maximum);

        const long width = image.size(1);
        const long height = image.size(0);
        const std::size_t row_width = static_cast<std::size_t>(width);
        std::vector<JSAMPLE> image_row(3 * row_width);
        std::vector<unsigned char> normalized_row(row_width);
        const LUT & colors = table_iterator->second;
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
                                         const auto & color = colors[normalized_row[x]];
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


    struct JpegHeader
    {
        int width;
        int height;
        int channels;
    };


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
        read(const std::filesystem::path & filename, Operation && operation)
        {
            file_.reset(std::fopen(filename.string().c_str(), "rb"));
            if(file_ == nullptr)
                throw std::runtime_error("Can't open " + filename.string());

            cinfo_.err = jpeg_std_error(&error_.pub);
            error_.pub.error_exit = jpeg_error_exit;

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


    static JpegHeader
    read_jpeg_header(const std::filesystem::path & filename)
    {
        JpegDecompressor decompressor;
        return decompressor.read(filename, [](const jpeg_decompress_struct & cinfo)
        {
            return JpegHeader
            {
                static_cast<int>(cinfo.image_width),
                static_cast<int>(cinfo.image_height),
                cinfo.num_components,
            };
        });
    }


    void
    jpeg_get_size(int & sizex, int & sizey, std::filesystem::path filename)
    {
        const JpegHeader header = read_jpeg_header(filename);
        sizex = header.width;
        sizey = header.height;
    }


    int
    jpeg_get_channels(std::filesystem::path filename)
    {
        return read_jpeg_header(filename).channels;
    }


    void
    jpeg_get_image(matrix & red, matrix & green, matrix & blue,
                   std::filesystem::path filename)
    {
        JpegDecompressor decompressor;
        decompressor.read(filename, [&](jpeg_decompress_struct & cinfo)
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

            red.resize(height, width);
            green.resize(height, width);
            blue.resize(height, width);

            JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)(
                reinterpret_cast<j_common_ptr>(&cinfo), JPOOL_IMAGE, row_stride, 1);

            int row = 0;
            while(cinfo.output_scanline < cinfo.output_height)
            {
                jpeg_read_scanlines(&cinfo, buffer, 1);
                for(int x = 0; x < width; ++x)
                {
                    red(row, x) = buffer[0][3 * x] / 255.0f;
                    green(row, x) = buffer[0][3 * x + 1] / 255.0f;
                    blue(row, x) = buffer[0][3 * x + 2] / 255.0f;
                }
                ++row;
            }

            jpeg_finish_decompress(&cinfo);
        });
    }

    //
    // PNG Images
    //

        void    
        png_get_size(int & sizex, int & sizey, std::filesystem::path filename)
        {
            FILE *fp = fopen(filename.c_str(), "rb");
            if (!fp) {
                throw std::runtime_error("Cannot open file: " + filename.string());
            }

            unsigned char header[8];
            fread(header, 1, 8, fp);
            if (png_sig_cmp(header, 0, 8)) {
                fclose(fp);
                throw std::runtime_error("Not a PNG file: " + filename.string());
            }

            png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png) {
                fclose(fp);
                throw std::runtime_error("Failed to create PNG read struct");
            }

            png_infop info = png_create_info_struct(png);
            if (!info) {
                png_destroy_read_struct(&png, nullptr, nullptr);
                fclose(fp);
                throw std::runtime_error("Failed to create PNG info struct");
            }

            if (setjmp(png_jmpbuf(png))) {
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                throw std::runtime_error("Error during PNG read");
            }

            png_init_io(png, fp);
            png_set_sig_bytes(png, 8);
            png_read_info(png, info);

            sizex = png_get_image_width(png, info);
            sizey = png_get_image_height(png, info);

            png_destroy_read_struct(&png, &info, nullptr);
            fclose(fp);
        }


        int     
        png_get_channels(std::filesystem::path filename)
        {
            FILE *fp = fopen(filename.c_str(), "rb");
            if (!fp) {
                throw std::runtime_error("Cannot open file: " + filename.string());
            }

            unsigned char header[8];
            fread(header, 1, 8, fp);
            if (png_sig_cmp(header, 0, 8)) {
                fclose(fp);
                throw std::runtime_error("Not a PNG file: " + filename.string());
            }

            png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            png_infop info = png_create_info_struct(png);

            if (setjmp(png_jmpbuf(png))) {
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                throw std::runtime_error("Error during PNG read");
            }

            png_init_io(png, fp);
            png_set_sig_bytes(png, 8);
            png_read_info(png, info);

            int channels = png_get_channels(png, info);

            png_destroy_read_struct(&png, &info, nullptr);
            fclose(fp);

            return channels;
        }



         void   
         png_get_image(matrix & red, matrix & green, matrix & blue, std::filesystem::path filename)
         {
 FILE *fp = fopen(filename.c_str(), "rb");
    if (!fp) {
        throw std::runtime_error("Cannot open file: " + filename.string());
    }

    unsigned char header[8];
    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8)) {
        fclose(fp);
        throw std::runtime_error("Not a PNG file: " + filename.string());
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop info = png_create_info_struct(png);

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        throw std::runtime_error("Error during PNG read");
    }

    png_init_io(png, fp);
    png_set_sig_bytes(png, 8);
    png_read_info(png, info);

    int width = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);
    int color_type = png_get_color_type(png, info);
    int bit_depth = png_get_bit_depth(png, info);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);
    if (bit_depth == 16)
        png_set_strip_16(png);
    if (color_type & PNG_COLOR_MASK_ALPHA || png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_strip_alpha(png);

    png_read_update_info(png, info);
    if(png_get_channels(png, info) != 3)
    {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        throw std::runtime_error("Unsupported PNG color format: " + filename.string());
    }

    // Allocate memory for the row pointers
    png_bytep *row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    if(row_pointers == nullptr)
    {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        throw std::runtime_error("Could not allocate PNG row pointers");
    }
    for(int y = 0; y < height; y++) {
        row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png, info));
        if(row_pointers[y] == nullptr)
        {
            for(int i = 0; i < y; i++)
                free(row_pointers[i]);
            free(row_pointers);
            png_destroy_read_struct(&png, &info, nullptr);
            fclose(fp);
            throw std::runtime_error("Could not allocate PNG row");
        }
    }

    png_read_image(png, row_pointers);

    // Resize matrices if needed
    red.resize(height, width);
    green.resize(height, width);
    blue.resize(height, width);

    // Copy data to matrices
    for(int y = 0; y < height; y++) {
        png_bytep row = row_pointers[y];
        for(int x = 0; x < width; x++) {
            png_bytep px = &(row[x * 3]);
            red[y][x] = px[0] / 255.0f;
            green[y][x] = px[1] / 255.0f;
            blue[y][x] = px[2] / 255.0f;
        }
    }

    // Cleanup
    for(int y = 0; y < height; y++) {
        free(row_pointers[y]);
    }
    free(row_pointers);

    png_destroy_read_struct(&png, &info, nullptr);
    fclose(fp);
         }

};
