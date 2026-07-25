#include "ikaros.h"

#include <cmath>

using namespace ikaros;

class Normalize: public Module
{
    parameter type;

    matrix input;
    matrix output;

    void Init()
    {
        Bind(type, "type");

        Bind(input, "INPUT");
        Bind(output, "OUTPUT");

        if(input.shape() != output.shape())
            throw exception("Normalize: OUTPUT shape must match INPUT shape.", path_);
    }

    void Tick()
    {
        switch(type.as_int())
        {
            case 0:
                NormalizeRange();
                break;

            case 1:
                NormalizeEuclidean();
                break;

            case 2:
                NormalizeCityBlock();
                break;

            case 3:
                NormalizeMax();
                break;

            default:
                throw exception("Normalize: type must be range, euclidean, cityblock, or max.", path_);
        }
    }

    void NormalizeRange()
    {
        if(input.size() == 0)
            return;

        const double minimum = input.min();
        const double maximum = input.max();
        const double range = maximum - minimum;

        if(range == 0.0)
        {
            output.set(0.0f);
            return;
        }

        output.copy(input);
        output.apply([minimum, range](float value) {
            return static_cast<float>(
                (static_cast<double>(value) - minimum) / range);
        });
    }

    void NormalizeEuclidean()
    {
        double squaredNorm = 0.0;
        for(int i = 0; i < input.size(); ++i)
        {
            const double value = input.data()[i];
            squaredNorm += value * value;
        }

        CopyOrZeroScaled(std::sqrt(squaredNorm));
    }

    void NormalizeCityBlock()
    {
        double norm = 0.0;
        for(int i = 0; i < input.size(); ++i)
            norm += std::fabs(static_cast<double>(input.data()[i]));

        CopyOrZeroScaled(norm);
    }

    void NormalizeMax()
    {
        if(input.size() == 0)
            return;

        CopyOrZeroScaled(input.max());
    }

    void CopyOrZeroScaled(double scale)
    {
        if(scale == 0.0)
        {
            output.set(0.0f);
            return;
        }

        output.copy(input);
        const float multiplier = static_cast<float>(1.0 / scale);
        if(std::isfinite(multiplier) && multiplier != 0.0f)
        {
            output.scale(multiplier);
            return;
        }

        output.apply([scale](float value) {
            return static_cast<float>(static_cast<double>(value) / scale);
        });
    }
};

INSTALL_CLASS(Normalize)
