#include "ikaros.h"

#include <algorithm>
#include <cmath>

using namespace ikaros;

namespace
{
    constexpr float lab_threshold = 0.008856f;

    float limit(float x, float a, float b)
    {
        return std::clamp(x, a, b);
    }

    float lab_f(float t)
    {
        if(t > lab_threshold)
            return std::cbrt(t);
        return 7.787f * t + 16.0f / 116.0f;
    }

    void transform_rgb2xyz(float red, float green, float blue, float & x, float & y, float & z)
    {
        x = 0.412453f * red + 0.357581f * green + 0.180423f * blue;
        y = 0.212670f * red + 0.715160f * green + 0.072169f * blue;
        z = 0.019334f * red + 0.119193f * green + 0.950227f * blue;
    }

    void transform_xyz2rgb(float x, float y, float z, float & red, float & green, float & blue)
    {
        red = 3.240479f * x - 1.537150f * y - 0.498535f * z;
        green = -0.969256f * x + 1.875992f * y + 0.041556f * z;
        blue = 0.055648f * x - 0.204043f * y + 1.057311f * z;
    }

    void transform_rgb2lab(float red, float green, float blue, float & l, float & a, float & b)
    {
        float x;
        float y;
        float z;
        transform_rgb2xyz(red, green, blue, x, y, z);

        x /= 0.950456f;
        z /= 1.088754f;

        if(y > lab_threshold)
            l = 116.0f * std::cbrt(y) - 16.0f;
        else
            l = 903.3f * y;

        if(l < 0.0f)
            l = 0.0f;

        a = 500.0f * (lab_f(x) - lab_f(y));
        b = 200.0f * (lab_f(y) - lab_f(z));
    }

    void transform_lab2rgb(float l, float a, float b, float & red, float & green, float & blue)
    {
        constexpr float t1 = 0.008856f;
        constexpr float t2 = 0.206893f;

        float fy = (l + 16.0f) / 116.0f;
        fy = fy * fy * fy;
        const bool yt = fy > t1;

        if(!yt)
            fy = l / 903.3f;

        float y = fy;

        if(yt)
            fy = std::cbrt(fy);
        else
            fy = 7.787f * fy + 16.0f / 116.0f;

        float fx = a / 500.0f + fy;
        float x = fx > t2 ? fx * fx * fx : (fx - 16.0f / 116.0f) / 7.787f;

        float fz = fy - b / 200.0f;
        float z = fz > t2 ? fz * fz * fz : (fz - 16.0f / 116.0f) / 7.787f;

        x *= 0.950456f;
        z *= 1.088754f;

        transform_xyz2rgb(x, y, z, red, green, blue);
        red = limit(red, 0.0f, 1.0f);
        green = limit(green, 0.0f, 1.0f);
        blue = limit(blue, 0.0f, 1.0f);
    }

    void transform_rgb2rgI(float red, float green, float blue, float & r, float & g, float & intensity)
    {
        intensity = red + green + blue;

        if(intensity == 0.0f)
        {
            r = 1.0f / 3.0f;
            g = 1.0f / 3.0f;
            return;
        }

        r = red / intensity;
        g = green / intensity;
    }
}

class ColorTransform: public Module
{
    parameter transform;
    parameter scale;

    matrix input;
    matrix output;

    void Init()
    {
        Bind(transform, "transform");
        Bind(scale, "scale");

        Bind(input, "INPUT");
        Bind(output, "OUTPUT");

        if(!IsColorImage())
            throw exception("ColorTransform: INPUT must have shape [3, rows, cols].", path_);

        if(output.shape() != input.shape())
            throw exception("ColorTransform: OUTPUT shape must match INPUT shape.", path_);
    }

    void Tick()
    {
        const float s = scale.as_float();
        if(s == 0.0f)
            throw exception("ColorTransform: scale must not be zero.", path_);

        const float invScale = 1.0f / s;
        switch(transform.as_int())
        {
            case 0:
                TransformImage(invScale, transform_rgb2lab);
                break;

            case 1:
                TransformLinear(invScale,
                    0.412453f, 0.357581f, 0.180423f,
                    0.212670f, 0.715160f, 0.072169f,
                    0.019334f, 0.119193f, 0.950227f);
                break;

            case 2:
                TransformImage(invScale, transform_lab2rgb);
                break;

            case 3:
                TransformLinear(invScale,
                    3.240479f, -1.537150f, -0.498535f,
                    -0.969256f, 1.875992f, 0.041556f,
                    0.055648f, -0.204043f, 1.057311f);
                break;

            case 4:
                TransformImage(invScale, transform_rgb2rgI);
                break;

            default:
                throw exception("ColorTransform: unknown transform.", path_);
        }
    }

    bool IsColorImage() const
    {
        return input.rank() == 3 && input.shape(0) == 3;
    }

    template<class TransformFunction>
    void TransformImage(float invScale, TransformFunction f)
    {
        const float * c0 = input[0].data();
        const float * c1 = input[1].data();
        const float * c2 = input[2].data();
        float * out0 = output[0].data();
        float * out1 = output[1].data();
        float * out2 = output[2].data();

        const int pixelCount = input.rows() * input.cols();
        for(int i = 0; i < pixelCount; ++i)
            f(c0[i] * invScale, c1[i] * invScale, c2[i] * invScale, out0[i], out1[i], out2[i]);
    }

    void TransformLinear(float invScale,
        float a00, float a01, float a02,
        float a10, float a11, float a12,
        float a20, float a21, float a22)
    {
        const float * c0 = input[0].data();
        const float * c1 = input[1].data();
        const float * c2 = input[2].data();
        float * out0 = output[0].data();
        float * out1 = output[1].data();
        float * out2 = output[2].data();

        const int pixelCount = input.rows() * input.cols();
        for(int i = 0; i < pixelCount; ++i)
        {
            const float v0 = c0[i] * invScale;
            const float v1 = c1[i] * invScale;
            const float v2 = c2[i] * invScale;

            out0[i] = a00 * v0 + a01 * v1 + a02 * v2;
            out1[i] = a10 * v0 + a11 * v1 + a12 * v2;
            out2[i] = a20 * v0 + a21 * v1 + a22 * v2;
        }
    }
};

INSTALL_CLASS(ColorTransform)
