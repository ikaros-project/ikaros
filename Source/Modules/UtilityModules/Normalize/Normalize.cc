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

        const float minimum = input.min();
        const float maximum = input.max();
        const float range = maximum - minimum;

        if(range == 0.0f)
        {
            output = 0.0f;
            return;
        }

        output.copy(input);
        output.apply([minimum, range](float x) { return (x - minimum) / range; });
    }

    void NormalizeEuclidean()
    {
        float norm = 0.0f;
        for(int i = 0; i < input.size(); ++i)
            norm += input.data()[i] * input.data()[i];

        norm = std::sqrt(norm);
        CopyOrZeroScaled(norm);
    }

    void NormalizeCityBlock()
    {
        float norm = 0.0f;
        for(int i = 0; i < input.size(); ++i)
            norm += std::fabs(input.data()[i]);

        CopyOrZeroScaled(norm);
    }

    void NormalizeMax()
    {
        if(input.size() == 0)
            return;

        CopyOrZeroScaled(input.max());
    }

    void CopyOrZeroScaled(float scale)
    {
        if(scale == 0.0f)
        {
            output = 0.0f;
            return;
        }

        output.copy(input);
        output.scale(1.0f / scale);
    }
};

INSTALL_CLASS(Normalize)
