#include "ikaros.h"

#include <cstdio>
#include <string>

using namespace ikaros;

class WhiteBalance: public Module
{
    parameter redTarget;
    parameter greenTarget;
    parameter blueTarget;

    parameter x0;
    parameter x1;
    parameter y0;
    parameter y1;

    parameter logX0;
    parameter logX1;
    parameter logY0;
    parameter logY1;

    matrix input;
    matrix output;

    bool warnedZeroReference = false;

    void Init()
    {
        Bind(redTarget, "red_target");
        Bind(greenTarget, "green_target");
        Bind(blueTarget, "blue_target");

        Bind(x0, "x0");
        Bind(x1, "x1");
        Bind(y0, "y0");
        Bind(y1, "y1");

        Bind(logX0, "log_x0");
        Bind(logX1, "log_x1");
        Bind(logY0, "log_y0");
        Bind(logY1, "log_y1");

        Bind(input, "INPUT");
        Bind(output, "OUTPUT");

        if(!IsColorImage())
            throw exception("WhiteBalance: INPUT must have shape [3, rows, cols].", path_);

        if(output.shape() != input.shape())
            throw exception("WhiteBalance: OUTPUT shape must match INPUT shape.", path_);

        ValidateRegion("reference", x0.as_int(), x1.as_int(), y0.as_int(), y1.as_int(), true);
        ValidateRegion("log", logX0.as_int(), logX1.as_int(), logY0.as_int(), logY1.as_int(), false);
    }

    void Tick()
    {
        const int left = x0.as_int();
        const int right = x1.as_int();
        const int top = y0.as_int();
        const int bottom = y1.as_int();

        const float redReference = AverageChannel(0, left, right, top, bottom);
        const float greenReference = AverageChannel(1, left, right, top, bottom);
        const float blueReference = AverageChannel(2, left, right, top, bottom);

        const float redGain = Gain(redTarget.as_float(), redReference);
        const float greenGain = Gain(greenTarget.as_float(), greenReference);
        const float blueGain = Gain(blueTarget.as_float(), blueReference);

        ApplyGains(redGain, greenGain, blueGain);
        LogRegion();
    }

    bool IsColorImage() const
    {
        return input.rank() == 3 && input.shape(0) == 3;
    }

    int Rows() const
    {
        return input.shape(1);
    }

    int Cols() const
    {
        return input.shape(2);
    }

    void ValidateRegion(const std::string & name, int left, int right, int top, int bottom, bool required)
    {
        if(!required && (left == right || top == bottom))
            return;

        if(left < 0 || top < 0 || right > Cols() || bottom > Rows())
            throw exception("WhiteBalance: " + name + " region is outside INPUT bounds.", path_);

        if(left >= right || top >= bottom)
            throw exception("WhiteBalance: " + name + " region must have positive width and height.", path_);
    }

    float AverageChannel(int channel, int left, int right, int top, int bottom)
    {
        const matrix plane = input[channel];
        float sum = 0.0f;

        for(int row = top; row < bottom; ++row)
            for(int col = left; col < right; ++col)
                sum += plane(row, col);

        return sum / float((right - left) * (bottom - top));
    }

    float Gain(float target, float reference)
    {
        if(reference != 0.0f)
            return target / reference;

        if(!warnedZeroReference)
        {
            Warning("WhiteBalance reference region has a zero channel average; using zero gain for that channel.");
            warnedZeroReference = true;
        }

        return 0.0f;
    }

    void ApplyGains(float redGain, float greenGain, float blueGain)
    {
        output[0].scale(input[0], redGain);
        output[1].scale(input[1], greenGain);
        output[2].scale(input[2], blueGain);
    }

    void LogRegion()
    {
        const int left = logX0.as_int();
        const int right = logX1.as_int();
        const int top = logY0.as_int();
        const int bottom = logY1.as_int();

        if(left == right || top == bottom)
            return;

        const float beforeRed = AverageChannel(0, left, right, top, bottom);
        const float beforeGreen = AverageChannel(1, left, right, top, bottom);
        const float beforeBlue = AverageChannel(2, left, right, top, bottom);

        const float afterRed = AverageOutputChannel(0, left, right, top, bottom);
        const float afterGreen = AverageOutputChannel(1, left, right, top, bottom);
        const float afterBlue = AverageOutputChannel(2, left, right, top, bottom);

        char message[160];
        std::snprintf(message, sizeof(message), "WhiteBalance: RGB before and after\t%.2f\t%.2f\t%.2f\t=>\t\t%.2f\t%.2f\t%.2f\n",
            beforeRed, beforeGreen, beforeBlue, afterRed, afterGreen, afterBlue);
        Notify(msg_debug, message);
    }

    float AverageOutputChannel(int channel, int left, int right, int top, int bottom)
    {
        const matrix plane = output[channel];
        float sum = 0.0f;

        for(int row = top; row < bottom; ++row)
            for(int col = left; col < right; ++col)
                sum += plane(row, col);

        return sum / float((right - left) * (bottom - top));
    }
};

INSTALL_CLASS(WhiteBalance)
