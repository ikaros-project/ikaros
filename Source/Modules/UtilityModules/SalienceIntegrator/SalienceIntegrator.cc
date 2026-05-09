#include "ikaros.h"

using namespace ikaros;

class SalienceIntegrator : public Module
{
    matrix input_;
    matrix output_;
    parameter factors_;
    parameter decay_;

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
        output_.scale(decay_);

        int index = 0;
        for(auto salience_map : input_)
            output_.multiply_and_accumulate(salience_map, factors_.get(index++, 1.0f));
    }
};

INSTALL_CLASS(SalienceIntegrator)
