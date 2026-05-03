#include "ikaros.h"

#include <algorithm>
#include <cmath>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/matrix.h>
#include <limits>

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

    void
    copy_to_dlib_gray(const matrix & input, dlib::array2d<unsigned char> & image)
    {
        int height = input.size_y();
        int width = input.size_x();
        image.set_size(height, width);

        if(input.rank() == 3 && input.shape(0) >= 3)
        {
            for(int y = 0; y < height; ++y)
                for(int x = 0; x < width; ++x)
                {
                    float r = input(0, y, x);
                    float g = input(1, y, x);
                    float b = input(2, y, x);
                    image[y][x] = to_byte(0.299f * r + 0.587f * g + 0.114f * b);
                }
            return;
        }

        for(int y = 0; y < height; ++y)
            for(int x = 0; x < width; ++x)
                image[y][x] = to_byte(input(y, x));
    }
}

class DlibFaceDetector : public Module
{
    matrix input_;
    matrix boxes_;
    matrix center_box_;
    matrix face_centers_;
    matrix count_;
    parameter max_faces_;
    parameter upsample_;
    dlib::frontal_face_detector detector_;
    dlib::array2d<unsigned char> image_;

public:
    void Init() override
    {
        Bind(input_, "INPUT");
        Bind(boxes_, "BOXES");
        Bind(center_box_, "CENTER_BOX");
        Bind(face_centers_, "FACE_CENTERS");
        Bind(count_, "COUNT");
        Bind(max_faces_, "max_faces");
        Bind(upsample_, "upsample");

        detector_ = dlib::get_frontal_face_detector();
    }

    void Tick() override
    {
        boxes_.clear();
        center_box_.clear();
        face_centers_.clear();
        count_.reset();

        copy_to_dlib_gray(input_, image_);

        for(int i = 0; i < static_cast<int>(upsample_); ++i)
            dlib::pyramid_up(image_);

        std::vector<dlib::rectangle> detections = detector_(image_);
        float scale = static_cast<float>(1 << std::max(0, static_cast<int>(upsample_)));
        int limit = std::min(static_cast<int>(max_faces_), static_cast<int>(detections.size()));
        float inv_width = input_.size_x() > 0 ? 1.0f / static_cast<float>(input_.size_x()) : 0.0f;
        float inv_height = input_.size_y() > 0 ? 1.0f / static_cast<float>(input_.size_y()) : 0.0f;

        int best_row = -1;
        float best_distance = std::numeric_limits<float>::max();

        for(int i = 0; i < limit; ++i)
        {
            dlib::rectangle r = detections[i];
            matrix row(5);
            row(0) = 2.0f * static_cast<float>(r.left()) / scale * inv_width - 1.0f;
            row(1) = 2.0f * static_cast<float>(r.top()) / scale * inv_height - 1.0f;
            row(2) = 2.0f * static_cast<float>(r.width()) / scale * inv_width;
            row(3) = 2.0f * static_cast<float>(r.height()) / scale * inv_height;
            row(4) = 1.0f;
            boxes_.append(row);

            float center_x = row(0) + 0.5f * row(2);
            float center_y = row(1) + 0.5f * row(3);
            matrix center_row(3);
            center_row(0) = center_x;
            center_row(1) = center_y;
            center_row(2) = row(2);
            face_centers_.append(center_row);

            float dx = center_x;
            float dy = center_y;
            float distance = dx * dx + dy * dy;
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

        count_(0) = static_cast<float>(boxes_.rows());
    }
};

INSTALL_CLASS(DlibFaceDetector)
