#include "ikaros.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import <Vision/Vision.h>

using namespace ikaros;

namespace
{
    unsigned char
    to_byte(float v)
    {
        if(v <= 1.0f)
            v *= 255.0f;
        return static_cast<unsigned char>(std::clamp(std::lround(v), 0l, 255l));
    }

    bool
    copy_to_rgba(const matrix & input, std::vector<unsigned char> & pixels)
    {
        int width = input.size_x();
        int height = input.size_y();
        if(width <= 0 || height <= 0)
            return false;

        pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4);

        if(input.rank() == 3 && input.shape(0) >= 3)
        {
            for(int y = 0; y < height; ++y)
                for(int x = 0; x < width; ++x)
                {
                    std::size_t offset = (static_cast<std::size_t>(y) * width + x) * 4;
                    pixels[offset + 0] = to_byte(input(0, y, x));
                    pixels[offset + 1] = to_byte(input(1, y, x));
                    pixels[offset + 2] = to_byte(input(2, y, x));
                    pixels[offset + 3] = 255;
                }
            return true;
        }

        for(int y = 0; y < height; ++y)
            for(int x = 0; x < width; ++x)
            {
                unsigned char v = to_byte(input(y, x));
                std::size_t offset = (static_cast<std::size_t>(y) * width + x) * 4;
                pixels[offset + 0] = v;
                pixels[offset + 1] = v;
                pixels[offset + 2] = v;
                pixels[offset + 3] = 255;
            }

        return true;
    }

    CGImageRef
    create_cg_image(const matrix & input, std::vector<unsigned char> & pixels)
    {
        if(!copy_to_rgba(input, pixels))
            return nullptr;

        int width = input.size_x();
        int height = input.size_y();
        std::size_t bytes_per_row = static_cast<std::size_t>(width) * 4;
        CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
        CGDataProviderRef provider = CGDataProviderCreateWithData(nullptr,
                                                                  pixels.data(),
                                                                  pixels.size(),
                                                                  nullptr);
        CGImageRef image = CGImageCreate(width,
                                         height,
                                         8,
                                         32,
                                         bytes_per_row,
                                         color_space,
                                         kCGImageAlphaLast | kCGBitmapByteOrderDefault,
                                         provider,
                                         nullptr,
                                         false,
                                         kCGRenderingIntentDefault);
        CGDataProviderRelease(provider);
        CGColorSpaceRelease(color_space);
        return image;
    }

    bool
    can_downsample(const matrix & input)
    {
        return input.size_x() >= 2 &&
               input.size_y() >= 2 &&
               input.size_x() % 2 == 0 &&
               input.size_y() % 2 == 0;
    }

    void
    downsample_once(matrix & output, const matrix & input)
    {
        if(input.rank() == 2)
        {
            if(!output.is_uninitialized() &&
               (output.rank() != 2 || output.size_y() != input.size_y() / 2 || output.size_x() != input.size_x() / 2))
                output = matrix();
            output.downsample(input);
            return;
        }

        if(output.is_uninitialized() ||
           output.rank() != 3 ||
           output.size_z() != input.size_z() ||
           output.size_y() != input.size_y() / 2 ||
           output.size_x() != input.size_x() / 2)
            output = matrix(input.size_z(), input.size_y() / 2, input.size_x() / 2);

        for(int i = 0; i < input.size_z(); ++i)
            output[i].downsample(input[i]);
    }
}

class AppleVisionFaceDetector : public Module
{
    matrix input_;
    matrix boxes_;
    matrix center_box_;
    matrix face_centers_;
    matrix count_;
    parameter max_faces_;
    parameter downsample_;
    matrix downsampled_a_;
    matrix downsampled_b_;
    std::vector<unsigned char> pixels_;
    VNDetectFaceRectanglesRequest * request_ = nil;
    VNSequenceRequestHandler * sequence_handler_ = nil;
    NSArray<VNRequest *> * requests_ = nil;

public:
    ~AppleVisionFaceDetector() override
    {
        @autoreleasepool
        {
            [requests_ release];
            [sequence_handler_ release];
            [request_ release];
        }
    }

    void Init() override
    {
        Bind(input_, "INPUT");
        Bind(boxes_, "BOXES");
        Bind(center_box_, "CENTER_BOX");
        Bind(face_centers_, "FACE_CENTERS");
        Bind(count_, "COUNT");
        Bind(max_faces_, "max_faces");
        Bind(downsample_, "downsample");

        @autoreleasepool
        {
            request_ = [[VNDetectFaceRectanglesRequest alloc] init];
            sequence_handler_ = [[VNSequenceRequestHandler alloc] init];
            requests_ = [[NSArray alloc] initWithObjects:request_, nil];
        }
    }

    void Tick() override
    {
        boxes_.clear();
        center_box_.clear();
        face_centers_.clear();
        count_.reset();

        @autoreleasepool
        {
            const matrix * detection_input = &input_;
            int passes = std::max(0, static_cast<int>(downsample_));
            for(int i = 0; i < passes && can_downsample(*detection_input); ++i)
            {
                matrix & output = i % 2 == 0 ? downsampled_a_ : downsampled_b_;
                downsample_once(output, *detection_input);
                detection_input = &output;
            }

            CGImageRef image = create_cg_image(*detection_input, pixels_);
            if(!image)
                return;

            NSError * error = nil;
            BOOL ok = [sequence_handler_ performRequests:requests_ onCGImage:image error:&error];
            CGImageRelease(image);

            if(!ok)
            {
                NSString * message = error ? [error localizedDescription] : @"unknown Vision error";
                Notify(msg_warning, std::string("AppleVisionFaceDetector failed: ") + [message UTF8String]);
                return;
            }

            NSArray<VNFaceObservation *> * results = request_.results;
            int limit = std::min(static_cast<int>(max_faces_), static_cast<int>([results count]));
            int best_row = -1;
            float best_distance = std::numeric_limits<float>::max();

            for(int i = 0; i < limit; ++i)
            {
                VNFaceObservation * face = [results objectAtIndex:i];
                CGRect b = face.boundingBox;

                matrix row(5);
                row(0) = 2.0f * static_cast<float>(b.origin.x) - 1.0f;
                row(1) = 1.0f - 2.0f * static_cast<float>(b.origin.y + b.size.height);
                row(2) = 2.0f * static_cast<float>(b.size.width);
                row(3) = 2.0f * static_cast<float>(b.size.height);
                row(4) = static_cast<float>(face.confidence);
                boxes_.append(row);

                float center_x = row(0) + 0.5f * row(2);
                float center_y = row(1) + 0.5f * row(3);
                matrix center_row(3);
                center_row(0) = center_x;
                center_row(1) = center_y;
                center_row(2) = row(2);
                face_centers_.append(center_row);

                float distance = center_x * center_x + center_y * center_y;
                if(distance < best_distance)
                {
                    best_distance = distance;
                    best_row = boxes_.rows() - 1;
                }
            }

            if(best_row >= 0)
            {
                matrix row(5);
                row(0) = boxes_(best_row, 0);
                row(1) = boxes_(best_row, 1);
                row(2) = boxes_(best_row, 2);
                row(3) = boxes_(best_row, 3);
                row(4) = boxes_(best_row, 4);
                center_box_.append(row);
            }
        }

        count_(0) = static_cast<float>(boxes_.rows());
    }
};

INSTALL_CLASS(AppleVisionFaceDetector)
