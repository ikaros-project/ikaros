#include "ikaros.h"

#include <cmath>

using namespace ikaros;

namespace
{
    float sqr(float x)
    {
        return x * x;
    }
}

class ColorMatch: public Module
{
    parameter alpha;
    parameter sigma;
    parameter gain;
    parameter threshold;

    parameter target0;
    parameter target1;
    parameter target2;

    matrix input;
    matrix targetInput;
    matrix focus;
    matrix reinforcement;
    matrix output;

    float target0Value = 0.0f;
    float target1Value = 0.0f;
    float target2Value = 0.0f;

    bool warnedInvalidFocus = false;

    void Init()
    {
        Bind(alpha, "alpha");
        Bind(sigma, "sigma");
        Bind(gain, "gain");
        Bind(threshold, "threshold");

        Bind(target0, "target0");
        Bind(target1, "target1");
        Bind(target2, "target2");

        Bind(input, "INPUT");
        Bind(targetInput, "TARGETINPUT");
        Bind(focus, "FOCUS");
        Bind(reinforcement, "REINFORCEMENT");
        Bind(output, "OUTPUT");

        if(!IsColorImage(input))
            throw exception("ColorMatch: INPUT must have shape [3, rows, cols].", path_);

        if(output.rank() != 2 || output.rows() != Rows() || output.cols() != Cols())
            throw exception("ColorMatch: OUTPUT must have shape [rows, cols].", path_);

        if(targetInput.connected() && targetInput.shape() != input.shape())
            throw exception("ColorMatch: TARGETINPUT shape must match INPUT shape.", path_);

        if(focus.connected() && focus.size() < 2)
            throw exception("ColorMatch: FOCUS must contain x and y.", path_);

        if(reinforcement.connected() && reinforcement.size() < 1)
            throw exception("ColorMatch: REINFORCEMENT must contain at least one value.", path_);

        target0Value = target0.as_float();
        target1Value = target1.as_float();
        target2Value = target2.as_float();
    }

    void Tick()
    {
        NormalizeTarget();
        MatchImage();
        RetuneTarget();
    }

    bool IsColorImage(const matrix & m) const
    {
        return m.rank() == 3 && m.shape(0) == 3;
    }

    int Rows() const
    {
        return input.shape(1);
    }

    int Cols() const
    {
        return input.shape(2);
    }

    void NormalizeTarget()
    {
        const float s = target0Value + target1Value + target2Value;
        if(s == 0.0f)
            return;

        target0Value /= s;
        target1Value /= s;
        target2Value /= s;
    }

    void MatchImage()
    {
        const float * c0 = input[0].data();
        const float * c1 = input[1].data();
        const float * c2 = input[2].data();
        float * out = output.data();

        const float t = threshold.as_float();
        const float g = gain.as_float();
        const float s = sigma.as_float();
        const int pixelCount = Rows() * Cols();

        for(int i = 0; i < pixelCount; ++i)
        {
            const float cs = c0[i] + c1[i] + c2[i];
            if(cs <= t)
            {
                out[i] = 0.0f;
                continue;
            }

            const float d = std::sqrt(
                sqr(c0[i] / cs - target0Value) +
                sqr(c1[i] / cs - target1Value) +
                sqr(c2[i] / cs - target2Value));

            out[i] = g * std::exp(-s * d);
        }
    }

    void RetuneTarget()
    {
        if(!targetInput.connected() || !focus.connected() || !reinforcement.connected() || alpha.as_float() == 0.0f)
            return;

        const int x = int(focus(0));
        const int y = int(focus(1));

        if(x < 0 || x >= Cols() || y < 0 || y >= Rows())
        {
            if(!warnedInvalidFocus)
            {
                Warning("ColorMatch FOCUS is outside INPUT bounds; skipping retuning.");
                warnedInvalidFocus = true;
            }
            return;
        }

        const float d = alpha.as_float() * reinforcement(0);
        target0Value = (1.0f - d) * target0Value + d * targetInput(0, y, x);
        target1Value = (1.0f - d) * target1Value + d * targetInput(1, y, x);
        target2Value = (1.0f - d) * target2Value + d * targetInput(2, y, x);
    }
};

INSTALL_CLASS(ColorMatch)
