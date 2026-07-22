#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "ikaros.h"

#if defined(__APPLE__) && defined(IKAROS_MATRIX_ACCELERATE) && IKAROS_MATRIX_ACCELERATE
#include <Accelerate/../Frameworks/vImage.framework/Headers/Conversion.h>
#define IKAROS_IMAGE_CONVERSION_BENCHMARK_ACCELERATE 1
#else
#define IKAROS_IMAGE_CONVERSION_BENCHMARK_ACCELERATE 0
#endif

using namespace ikaros;

namespace
{
    template<typename Function>
    double
    measure_milliseconds_per_frame(int iterations, Function && function)
    {
        const auto start = std::chrono::steady_clock::now();
        for(int iteration = 0; iteration < iterations; ++iteration)
            function();
        const double milliseconds = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        return milliseconds / iterations;
    }


    unsigned char
    normalized_float_to_byte(float value) noexcept
    {
        if(!(value > 0))
            return 0;
        if(value >= 1)
            return 255;
        return static_cast<unsigned char>(value * 255.0f + 0.5f);
    }


    void
    convert_rgb8_to_planar_float_rows(const unsigned char * source, float * red,
                                      float * green, float * blue,
                                      int width, int height) noexcept
    {
        constexpr float scale = 1.0f / 255.0f;
        const std::size_t rowWidth = static_cast<std::size_t>(width);
        for(int row = 0; row < height; ++row)
        {
            const std::size_t rowOffset = static_cast<std::size_t>(row) * rowWidth;
            const unsigned char * sourcePixel = source + 3 * rowOffset;
            float * redPixel = red + rowOffset;
            float * greenPixel = green + rowOffset;
            float * bluePixel = blue + rowOffset;
            for(int column = 0; column < width; ++column)
            {
                redPixel[column] = sourcePixel[3 * column] * scale;
                greenPixel[column] = sourcePixel[3 * column + 1] * scale;
                bluePixel[column] = sourcePixel[3 * column + 2] * scale;
            }
        }
    }


#if IKAROS_IMAGE_CONVERSION_BENCHMARK_ACCELERATE
    void
    convert_planar_float_to_rgb8_rows(const float * red, const float * green,
                                      const float * blue, unsigned char * destination,
                                      int width, int height,
                                      std::vector<unsigned char> & planarStorage)
    {
        const std::size_t rowWidth = static_cast<std::size_t>(width);
        const std::size_t floatRowBytes = rowWidth * sizeof(float);
        planarStorage.resize(3 * rowWidth);

        for(int row = 0; row < height; ++row)
        {
            vImage_Buffer sourceRed
            {
                const_cast<float *>(red + static_cast<std::size_t>(row) * rowWidth),
                1,
                static_cast<vImagePixelCount>(width),
                floatRowBytes,
            };
            vImage_Buffer sourceGreen
            {
                const_cast<float *>(green + static_cast<std::size_t>(row) * rowWidth),
                1,
                static_cast<vImagePixelCount>(width),
                floatRowBytes,
            };
            vImage_Buffer sourceBlue
            {
                const_cast<float *>(blue + static_cast<std::size_t>(row) * rowWidth),
                1,
                static_cast<vImagePixelCount>(width),
                floatRowBytes,
            };
            vImage_Buffer planarRed{planarStorage.data(), 1,
                                    static_cast<vImagePixelCount>(width), rowWidth};
            vImage_Buffer planarGreen{planarStorage.data() + rowWidth, 1,
                                      static_cast<vImagePixelCount>(width), rowWidth};
            vImage_Buffer planarBlue{planarStorage.data() + 2 * rowWidth, 1,
                                     static_cast<vImagePixelCount>(width), rowWidth};
            vImage_Buffer result
            {
                destination + 3 * static_cast<std::size_t>(row) * rowWidth,
                1,
                static_cast<vImagePixelCount>(width),
                3 * rowWidth,
            };

            if(vImageConvert_PlanarFtoPlanar8(&sourceRed, &planarRed, 1, 0,
                                               kvImageDoNotTile) == kvImageNoError &&
               vImageConvert_PlanarFtoPlanar8(&sourceGreen, &planarGreen, 1, 0,
                                               kvImageDoNotTile) == kvImageNoError &&
               vImageConvert_PlanarFtoPlanar8(&sourceBlue, &planarBlue, 1, 0,
                                               kvImageDoNotTile) == kvImageNoError &&
               vImageConvert_Planar8toRGB888(&planarRed, &planarGreen, &planarBlue,
                                              &result, kvImageDoNotTile) == kvImageNoError)
                continue;

            unsigned char * resultPixel = static_cast<unsigned char *>(result.data);
            const float * redPixel = static_cast<float *>(sourceRed.data);
            const float * greenPixel = static_cast<float *>(sourceGreen.data);
            const float * bluePixel = static_cast<float *>(sourceBlue.data);
            for(int column = 0; column < width; ++column)
            {
                *resultPixel++ = normalized_float_to_byte(redPixel[column]);
                *resultPixel++ = normalized_float_to_byte(greenPixel[column]);
                *resultPixel++ = normalized_float_to_byte(bluePixel[column]);
            }
        }
    }


    void
    convert_planar_float_to_rgb8_blocks(const float * red, const float * green,
                                        const float * blue, unsigned char * destination,
                                        int width, int height, int blockRows,
                                        std::vector<unsigned char> & planarStorage)
    {
        const std::size_t rowWidth = static_cast<std::size_t>(width);
        const std::size_t floatRowBytes = rowWidth * sizeof(float);
        planarStorage.resize(3 * rowWidth * static_cast<std::size_t>(blockRows));

        for(int firstRow = 0; firstRow < height; firstRow += blockRows)
        {
            const int rows = std::min(blockRows, height - firstRow);
            const std::size_t blockPixels = rowWidth * static_cast<std::size_t>(rows);
            const std::size_t sourceOffset = rowWidth * static_cast<std::size_t>(firstRow);
            const std::size_t blockRowBytes = rowWidth;
            vImage_Buffer sourceRed
            {
                const_cast<float *>(red + sourceOffset),
                static_cast<vImagePixelCount>(rows),
                static_cast<vImagePixelCount>(width),
                floatRowBytes,
            };
            vImage_Buffer sourceGreen
            {
                const_cast<float *>(green + sourceOffset),
                static_cast<vImagePixelCount>(rows),
                static_cast<vImagePixelCount>(width),
                floatRowBytes,
            };
            vImage_Buffer sourceBlue
            {
                const_cast<float *>(blue + sourceOffset),
                static_cast<vImagePixelCount>(rows),
                static_cast<vImagePixelCount>(width),
                floatRowBytes,
            };
            vImage_Buffer planarRed{planarStorage.data(),
                                    static_cast<vImagePixelCount>(rows),
                                    static_cast<vImagePixelCount>(width), blockRowBytes};
            vImage_Buffer planarGreen{planarStorage.data() + blockPixels,
                                      static_cast<vImagePixelCount>(rows),
                                      static_cast<vImagePixelCount>(width), blockRowBytes};
            vImage_Buffer planarBlue{planarStorage.data() + 2 * blockPixels,
                                     static_cast<vImagePixelCount>(rows),
                                     static_cast<vImagePixelCount>(width), blockRowBytes};
            vImage_Buffer result
            {
                destination + 3 * sourceOffset,
                static_cast<vImagePixelCount>(rows),
                static_cast<vImagePixelCount>(width),
                3 * rowWidth,
            };

            if(vImageConvert_PlanarFtoPlanar8(&sourceRed, &planarRed, 1, 0,
                                               kvImageDoNotTile) != kvImageNoError ||
               vImageConvert_PlanarFtoPlanar8(&sourceGreen, &planarGreen, 1, 0,
                                               kvImageDoNotTile) != kvImageNoError ||
               vImageConvert_PlanarFtoPlanar8(&sourceBlue, &planarBlue, 1, 0,
                                               kvImageDoNotTile) != kvImageNoError ||
               vImageConvert_Planar8toRGB888(&planarRed, &planarGreen, &planarBlue,
                                              &result, kvImageDoNotTile) != kvImageNoError)
                throw std::runtime_error("Block RGB conversion failed");
        }
    }


    void
    convert_rgb8_to_planar_float_blocks(const unsigned char * source, float * red,
                                        float * green, float * blue, int width,
                                        int height, int blockRows,
                                        std::vector<unsigned char> & planarStorage)
    {
        const std::size_t rowWidth = static_cast<std::size_t>(width);
        const std::size_t floatRowBytes = rowWidth * sizeof(float);
        planarStorage.resize(3 * rowWidth * static_cast<std::size_t>(blockRows));

        for(int firstRow = 0; firstRow < height; firstRow += blockRows)
        {
            const int rows = std::min(blockRows, height - firstRow);
            const std::size_t blockPixels = rowWidth * static_cast<std::size_t>(rows);
            const std::size_t pixelOffset = rowWidth * static_cast<std::size_t>(firstRow);
            vImage_Buffer sourceBuffer
            {
                const_cast<unsigned char *>(source + 3 * pixelOffset),
                static_cast<vImagePixelCount>(rows),
                static_cast<vImagePixelCount>(width),
                3 * rowWidth,
            };
            vImage_Buffer planarRed{planarStorage.data(),
                                    static_cast<vImagePixelCount>(rows),
                                    static_cast<vImagePixelCount>(width), rowWidth};
            vImage_Buffer planarGreen{planarStorage.data() + blockPixels,
                                      static_cast<vImagePixelCount>(rows),
                                      static_cast<vImagePixelCount>(width), rowWidth};
            vImage_Buffer planarBlue{planarStorage.data() + 2 * blockPixels,
                                     static_cast<vImagePixelCount>(rows),
                                     static_cast<vImagePixelCount>(width), rowWidth};
            vImage_Buffer resultRed
            {
                red + pixelOffset,
                static_cast<vImagePixelCount>(rows),
                static_cast<vImagePixelCount>(width),
                floatRowBytes,
            };
            vImage_Buffer resultGreen
            {
                green + pixelOffset,
                static_cast<vImagePixelCount>(rows),
                static_cast<vImagePixelCount>(width),
                floatRowBytes,
            };
            vImage_Buffer resultBlue
            {
                blue + pixelOffset,
                static_cast<vImagePixelCount>(rows),
                static_cast<vImagePixelCount>(width),
                floatRowBytes,
            };

            if(vImageConvert_RGB888toPlanar8(&sourceBuffer, &planarRed, &planarGreen,
                                              &planarBlue,
                                              kvImageDoNotTile) != kvImageNoError ||
               vImageConvert_Planar8toPlanarF(&planarRed, &resultRed, 1, 0,
                                               kvImageDoNotTile) != kvImageNoError ||
               vImageConvert_Planar8toPlanarF(&planarGreen, &resultGreen, 1, 0,
                                               kvImageDoNotTile) != kvImageNoError ||
               vImageConvert_Planar8toPlanarF(&planarBlue, &resultBlue, 1, 0,
                                               kvImageDoNotTile) != kvImageNoError)
                throw std::runtime_error("Block RGB decoding conversion failed");
        }
    }
#endif
}


class ImageConversionBenchmarkModule : public Module
{
    parameter width;
    parameter height;
    parameter iterations;
    parameter blockRows;

public:
    void
    Init() override
    {
        Bind(width, "width");
        Bind(height, "height");
        Bind(iterations, "iterations");
        Bind(blockRows, "block_rows");

#if IKAROS_IMAGE_CONVERSION_BENCHMARK_ACCELERATE
        const int imageWidth = width.as_int();
        const int imageHeight = height.as_int();
        const int measuredIterations = iterations.as_int();
        const int requestedRowsPerBlock = blockRows.as_int();
        if(imageWidth <= 0 || imageHeight <= 0 || measuredIterations <= 0 ||
           requestedRowsPerBlock <= 0)
            throw std::invalid_argument("Image conversion benchmark values must be positive");
        if(static_cast<std::size_t>(imageHeight) >
           std::numeric_limits<std::size_t>::max() /
               static_cast<std::size_t>(imageWidth))
            throw std::length_error("Image conversion benchmark dimensions overflow");

        const std::size_t pixelCount = static_cast<std::size_t>(imageWidth) * imageHeight;
        if(pixelCount > std::numeric_limits<std::size_t>::max() / 3)
            throw std::length_error("Image conversion benchmark storage size overflow");
        const int rowsPerBlock = std::min(requestedRowsPerBlock, imageHeight);
        std::vector<float> sourcePlanar(3 * pixelCount);
        std::vector<unsigned char> sourceRgb(3 * pixelCount);
        for(std::size_t pixel = 0; pixel < pixelCount; ++pixel)
        {
            sourcePlanar[pixel] = static_cast<float>((17 * pixel + 3) & 255) / 255.0f;
            sourcePlanar[pixelCount + pixel] =
                static_cast<float>((29 * pixel + 5) & 255) / 255.0f;
            sourcePlanar[2 * pixelCount + pixel] =
                static_cast<float>((43 * pixel + 7) & 255) / 255.0f;
            sourceRgb[3 * pixel] = static_cast<unsigned char>((17 * pixel + 3) & 255);
            sourceRgb[3 * pixel + 1] = static_cast<unsigned char>((29 * pixel + 5) & 255);
            sourceRgb[3 * pixel + 2] = static_cast<unsigned char>((43 * pixel + 7) & 255);
        }

        std::vector<unsigned char> encodedRows(3 * pixelCount);
        std::vector<unsigned char> encodedBlocks(3 * pixelCount);
        std::vector<float> decodedScalar(3 * pixelCount);
        std::vector<float> decodedBlocks(3 * pixelCount);
        std::vector<unsigned char> rowStorage;
        std::vector<unsigned char> blockStorage;

        auto encodeRows = [&]()
        {
            convert_planar_float_to_rgb8_rows(
                sourcePlanar.data(), sourcePlanar.data() + pixelCount,
                sourcePlanar.data() + 2 * pixelCount, encodedRows.data(),
                imageWidth, imageHeight, rowStorage);
        };
        auto encodeBlocks = [&]()
        {
            convert_planar_float_to_rgb8_blocks(
                sourcePlanar.data(), sourcePlanar.data() + pixelCount,
                sourcePlanar.data() + 2 * pixelCount, encodedBlocks.data(),
                imageWidth, imageHeight, rowsPerBlock, blockStorage);
        };
        auto decodeScalar = [&]()
        {
            convert_rgb8_to_planar_float_rows(
                sourceRgb.data(), decodedScalar.data(), decodedScalar.data() + pixelCount,
                decodedScalar.data() + 2 * pixelCount, imageWidth, imageHeight);
        };
        auto decodeBlocks = [&]()
        {
            convert_rgb8_to_planar_float_blocks(
                sourceRgb.data(), decodedBlocks.data(), decodedBlocks.data() + pixelCount,
                decodedBlocks.data() + 2 * pixelCount, imageWidth, imageHeight,
                rowsPerBlock, blockStorage);
        };

        encodeRows();
        encodeBlocks();
        decodeScalar();
        decodeBlocks();
        if(encodedRows != encodedBlocks)
            throw std::runtime_error("Row and block encoders produced different pixels");
        if(!std::equal(decodedScalar.begin(), decodedScalar.end(), decodedBlocks.begin(),
                       [](float left, float right)
                       {
                           return std::fabs(left - right) <= 1e-7f;
                       }))
            throw std::runtime_error("Scalar and block decoders produced different pixels");

        const double encodeRowsMs =
            measure_milliseconds_per_frame(measuredIterations, encodeRows);
        const double encodeBlocksMs =
            measure_milliseconds_per_frame(measuredIterations, encodeBlocks);
        const double decodeScalarMs =
            measure_milliseconds_per_frame(measuredIterations, decodeScalar);
        const double decodeBlocksMs =
            measure_milliseconds_per_frame(measuredIterations, decodeBlocks);

        double checksum = 0;
        constexpr std::size_t checksumStride = 4093;
        for(std::size_t index = 0; index < encodedRows.size(); index += checksumStride)
            checksum += encodedRows[index] + encodedBlocks[index];
        for(std::size_t index = 0; index < decodedScalar.size(); index += checksumStride)
            checksum += decodedScalar[index] + decodedBlocks[index];

        std::ostringstream output;
        output << std::fixed << std::setprecision(6)
               << path_ << " IMAGE CONVERSION BENCHMARK"
               << " width=" << imageWidth
               << " height=" << imageHeight
               << " block_rows=" << rowsPerBlock
               << " encode_rows_ms=" << encodeRowsMs
               << " encode_blocks_ms=" << encodeBlocksMs
               << " decode_scalar_ms=" << decodeScalarMs
               << " decode_blocks_ms=" << decodeBlocksMs
               << " checksum=" << checksum;
        std::cout << output.str() << std::endl;
#else
        throw std::runtime_error(
            "Image conversion benchmark requires Apple Accelerate matrix support");
#endif
    }
};

INSTALL_CLASS(ImageConversionBenchmarkModule)
