#include "ikaros.h"

#include <algorithm>

using namespace ikaros;

class SalienceIntegrator : public Module
{
    matrix input_;
    matrix output_;
    parameter factors_;
    parameter decay_;

    float
    factor(int index)
    {
        matrix & f = factors_;
        if(f.empty())
            return 1.0f;
        if(index < f.size())
            return f.data()[index];
        return 1.0f;
    }

public:
    void Init() override
    {
        Bind(input_, "INPUT");
        Bind(output_, "OUTPUT");
        Bind(factors_, "factors");
        Bind(decay_, "decay");
    }

    void Tick() override
    {
        output_.scale(static_cast<float>(decay_));

        if(input_.empty())
            return;

        if(input_.rank() == 2)
        {
            if(input_.rows() != output_.rows() || input_.cols() != output_.cols())
                Notify(msg_fatal_error, "INPUT and OUTPUT sizes do not match.");

            const float scale = factor(0);
            for(int y = 0; y < output_.rows(); ++y)
                for(int x = 0; x < output_.cols(); ++x)
                    output_(y, x) += scale * input_(y, x);
            return;
        }

        if(input_.rank() != 3)
            Notify(msg_fatal_error, "INPUT must be a single salience map or stacked salience maps.");

        if(input_.shape(1) != output_.rows() || input_.shape(2) != output_.cols())
            Notify(msg_fatal_error, "INPUT and OUTPUT sizes do not match.");

        for(int i = 0; i < input_.shape(0); ++i)
        {
            const float scale = factor(i);
            for(int y = 0; y < output_.rows(); ++y)
                for(int x = 0; x < output_.cols(); ++x)
                    output_(y, x) += scale * input_(i, y, x);
        }
    }
};

INSTALL_CLASS(SalienceIntegrator)
