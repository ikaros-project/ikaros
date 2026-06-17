#include "ikaros.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <thread>
#include <vector>

#if defined(__APPLE__)
#include <dispatch/dispatch.h>
#endif

using namespace ikaros;

class CameraRectification : public Module
{
    struct RemapEntry
    {
        int index00 = 0;
        int index10 = 0;
        int index01 = 0;
        int index11 = 0;
        float w00 = 0.0f;
        float w10 = 0.0f;
        float w01 = 0.0f;
        float w11 = 0.0f;
        bool valid = false;
    };

    matrix input;
    matrix camera_matrix;
    matrix distortion;
    matrix output;

    parameter border_value;

    std::vector<RemapEntry> map_;
    std::vector<float> last_camera_;
    std::vector<float> last_distortion_;
    int last_rows_ = 0;
    int last_cols_ = 0;
    bool warned_tilt_ = false;
    bool warned_zero_focal_ = false;

    static bool
    MatrixChanged(const matrix & m, std::vector<float> & cache)
    {
        std::vector<float> values;
        values.reserve(m.size());
        m.reduce([&values](float value)
        {
            values.push_back(value);
        });

        if (cache != values)
        {
            cache = std::move(values);
            return true;
        }

        return false;
    }

    static float
    Coeff(const matrix & coeffs, int index)
    {
        return index < coeffs.size() ? coeffs.data()[index] : 0.0f;
    }

    bool
    ValidateInputs()
    {
        if (input.rank() != 2 && input.rank() != 3)
        {
            Notify(msg_fatal_error, "CameraRectification expects INPUT to be grayscale [rows, cols] or RGB [3, rows, cols].");
            return false;
        }

        if (input.rank() == 3 && input.size_z() != 3)
        {
            Notify(msg_fatal_error, "CameraRectification expects color INPUT to have exactly 3 channels.");
            return false;
        }

        if (camera_matrix.rows() != 3 || camera_matrix.cols() != 3)
        {
            Notify(msg_fatal_error, "CameraRectification CAMERA_MATRIX must be a 3x3 matrix.");
            return false;
        }

        if (distortion.size() < 4)
        {
            Notify(msg_fatal_error, "CameraRectification DISTORTION must contain at least k1, k2, p1, p2.");
            return false;
        }

        if (camera_matrix(0, 0) == 0.0f || camera_matrix(1, 1) == 0.0f)
        {
            if (!warned_zero_focal_)
            {
                warned_zero_focal_ = true;
                Notify(msg_warning, "CameraRectification is waiting for non-zero CAMERA_MATRIX focal lengths.");
            }
            return false;
        }

        warned_zero_focal_ = false;

        return true;
    }

    void
    UpdateMapIfNeeded()
    {
        const int rows = input.rows();
        const int cols = input.cols();
        const bool size_changed = rows != last_rows_ || cols != last_cols_;
        const bool camera_changed = MatrixChanged(camera_matrix, last_camera_);
        const bool distortion_changed = MatrixChanged(distortion, last_distortion_);

        if (!size_changed && !camera_changed && !distortion_changed)
            return;

        last_rows_ = rows;
        last_cols_ = cols;
        map_.resize(static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols));

        const float fx = camera_matrix(0, 0);
        const float fy = camera_matrix(1, 1);
        const float cx = camera_matrix(0, 2);
        const float cy = camera_matrix(1, 2);

        const float k1 = Coeff(distortion, 0);
        const float k2 = Coeff(distortion, 1);
        const float p1 = Coeff(distortion, 2);
        const float p2 = Coeff(distortion, 3);
        const float k3 = Coeff(distortion, 4);
        const float k4 = Coeff(distortion, 5);
        const float k5 = Coeff(distortion, 6);
        const float k6 = Coeff(distortion, 7);
        const float s1 = Coeff(distortion, 8);
        const float s2 = Coeff(distortion, 9);
        const float s3 = Coeff(distortion, 10);
        const float s4 = Coeff(distortion, 11);

        if (!warned_tilt_ && distortion.size() > 12 && (Coeff(distortion, 12) != 0.0f || Coeff(distortion, 13) != 0.0f))
        {
            warned_tilt_ = true;
            Notify(msg_warning, "CameraRectification ignores OpenCV tilt coefficients tauX/tauY.");
        }

        for (int y = 0; y < rows; ++y)
        {
            for (int x = 0; x < cols; ++x)
            {
                const float xn = (static_cast<float>(x) - cx) / fx;
                const float yn = (static_cast<float>(y) - cy) / fy;
                const float r2 = xn * xn + yn * yn;
                const float r4 = r2 * r2;
                const float r6 = r4 * r2;
                const float numerator = 1.0f + k1 * r2 + k2 * r4 + k3 * r6;
                const float denominator = 1.0f + k4 * r2 + k5 * r4 + k6 * r6;
                const float radial = denominator != 0.0f ? numerator / denominator : numerator;
                const float xy2 = 2.0f * xn * yn;

                const float xd = xn * radial + p1 * xy2 + p2 * (r2 + 2.0f * xn * xn) + s1 * r2 + s2 * r4;
                const float yd = yn * radial + p1 * (r2 + 2.0f * yn * yn) + p2 * xy2 + s3 * r2 + s4 * r4;
                const float src_x = fx * xd + cx;
                const float src_y = fy * yd + cy;

                RemapEntry entry;
                if (src_x >= 0.0f && src_y >= 0.0f && src_x <= static_cast<float>(cols - 1) && src_y <= static_cast<float>(rows - 1))
                {
                    const int x0 = static_cast<int>(src_x);
                    const int y0 = static_cast<int>(src_y);
                    const int x1 = std::min(x0 + 1, cols - 1);
                    const int y1 = std::min(y0 + 1, rows - 1);
                    const float ax = src_x - static_cast<float>(x0);
                    const float ay = src_y - static_cast<float>(y0);

                    entry.index00 = y0 * cols + x0;
                    entry.index10 = y0 * cols + x1;
                    entry.index01 = y1 * cols + x0;
                    entry.index11 = y1 * cols + x1;
                    entry.w00 = (1.0f - ax) * (1.0f - ay);
                    entry.w10 = ax * (1.0f - ay);
                    entry.w01 = (1.0f - ax) * ay;
                    entry.w11 = ax * ay;
                    entry.valid = true;
                }

                map_[static_cast<std::size_t>(y) * cols + x] = entry;
            }
        }
    }

    void
    RectifyRows(int begin_row, int end_row)
    {
        const int cols = input.cols();
        const int channels = input.rank() == 2 ? 1 : input.size_z();
        const float fill = border_value;

        for (int channel = 0; channel < channels; ++channel)
        {
            const float * src = input.rank() == 2 ? input.data() : input[channel].data();
            float * dst = output.rank() == 2 ? output.data() : output[channel].data();

            for (int y = begin_row; y < end_row; ++y)
            {
                const int row_offset = y * cols;
                for (int x = 0; x < cols; ++x)
                {
                    const RemapEntry & e = map_[static_cast<std::size_t>(row_offset + x)];
                    dst[row_offset + x] = e.valid
                        ? src[e.index00] * e.w00 + src[e.index10] * e.w10 + src[e.index01] * e.w01 + src[e.index11] * e.w11
                        : fill;
                }
            }
        }
    }

    void
    Rectify()
    {
        const int rows = input.rows();
        const unsigned int hardware_threads = std::max(1u, std::thread::hardware_concurrency());
        const int chunks = std::min<int>(rows, std::max(1, static_cast<int>(hardware_threads) * 2));

#if defined(__APPLE__)
        struct DispatchContext
        {
            CameraRectification * rectifier;
            int rows;
            int chunks;
        } context{this, rows, chunks};

        dispatch_apply_f(static_cast<std::size_t>(chunks), dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), &context,
            [](void * raw_context, std::size_t chunk) {
                auto * context = static_cast<DispatchContext *>(raw_context);
                const int begin = context->rows * static_cast<int>(chunk) / context->chunks;
                const int end = context->rows * (static_cast<int>(chunk) + 1) / context->chunks;
                context->rectifier->RectifyRows(begin, end);
            });
#else
        if (chunks <= 1)
        {
            RectifyRows(0, rows);
            return;
        }

        std::vector<std::thread> threads;
        threads.reserve(static_cast<std::size_t>(chunks - 1));

        for (int chunk = 0; chunk < chunks; ++chunk)
        {
            const int begin = rows * chunk / chunks;
            const int end = rows * (chunk + 1) / chunks;
            if (chunk == chunks - 1)
                RectifyRows(begin, end);
            else
                threads.emplace_back([this, begin, end]() { RectifyRows(begin, end); });
        }

        for (auto & thread : threads)
            thread.join();
#endif
    }

public:
    void
    Init()
    {
        Bind(input, "INPUT");
        Bind(camera_matrix, "CAMERA_MATRIX");
        Bind(distortion, "DISTORTION");
        Bind(output, "OUTPUT");
        Bind(border_value, "border_value");
    }

    void
    Tick()
    {
        if (!ValidateInputs())
            return;

        if (output.rank() != input.rank() || output.rows() != input.rows() || output.cols() != input.cols())
        {
            if (input.rank() == 2)
                output.realloc(input.rows(), input.cols());
            else
                output.realloc(input.size_z(), input.rows(), input.cols());
        }

        UpdateMapIfNeeded();
        Rectify();
    }
};

INSTALL_CLASS(CameraRectification)
